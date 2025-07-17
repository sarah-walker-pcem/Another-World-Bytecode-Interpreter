	.text

	.set	XOS_SetCallBack, 0x2001b 
	.set	OS_AddCallBack, 0x000054 

	.global tickerv_handler
	.global callback_register_buffer
	.global callback_handler

	.global arcStubTimerCallback

tickerv_handler:
	STMDB	sp!, {r0, r1, lr}

	MOV	r0, pc
	ORR	r1, r0, #3
	TEQP	pc, r1			// Enter SVC mode so we can save R14_svc

	STR	lr, [sp, #-4]!
	SWI	XOS_SetCallBack
	LDR	lr, [sp], #4

	TEQP	pc, r0			// Back to IRQ mode

	LDMIA	sp!, {r0, r1, lr}
	MOVS	pc, lr


callback_register_buffer:
	.rept 16
	.word 0
	.endr

in_callback:
	.word 0

callback_stack_registers:
	.word temp_stack
	.word temp_stack + 4096

callback_handler:
	LDR	r0, in_callback
	CMP	r0, #0
	BNE	skip_callback

	MOV	r0, #1
	STR	r0, in_callback

	ADR 	r14, callback_register_buffer
	LDR	r12, [r14, #13*4]	// R13_usr

	ADD	r11, r14, #8*4
	LDMIA	r11, {r0-r7}
	STMDB	r12!, {r6-r7}		// Skip R13
	STMDB	r12!, {r0-r4}
	LDMIA	r14, {r0-r7}
	STMDB	r12!, {r0-r7}		// R0-R15 (sans R13) now on user stack
	STR	r12, old_sp

	// Switch to temporary stack for the next call
	ADR	sl, callback_stack_registers
	LDMIA	sl, {sl, sp}^

	TEQP	pc, #0			// Switch to user mode!


	BL	arcStubTimerCallback

	LDR	sp, old_sp		// Restore old stack

	MOV	r0, #0
	STR	r0, in_callback

	LDMIA	sp!, {r0-r12}
	LDMIA	sp!, {r14, r15}^




	LDMIA	r14!, {r0-r7}
	STMDB	r12!, {r0-r7}
	LDMIA	r14!, {r0-r4}	//r8-r12
	
skip_callback:
	LDMIA	r14, {r0-r14}^
	MOV	r0, r0
	LDR	r14, [r14, #15*4]
	MOVS	pc, r14


old_sp:
	.word 0

temp_stack:
	.rept 1024
	.word 0
	.endr

