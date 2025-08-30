	.text

	.set	XOS_SetCallBack, 0x2001b 
	.set	OS_AddCallBack, 0x000054 

	.global tickerv_handler
	.global callback_register_buffer
	.global callback_handler

	.global arcStubTimerCallback

tickerv_handler:
	STMDB	sp!, {r0, r1, lr}

	CMP	pc, pc
	MRSEQ	r0, cpsr
	BICEQ	r1, r0, #0x1f
	ORREQ	r1, r1, #0x13
	MOVNE	r0, pc
	ORRNE	r1, r0, #3
	MSREQ	cpsr_c, r1
	TEQNEP	pc, r1			// Enter SVC mode so we can save R14_svc
	MOV	r0, r0

	STR	lr, [sp, #-4]!
	SWI	XOS_SetCallBack
	LDR	lr, [sp], #4

	CMP	pc, pc
	MSREQ	cpsr_c, r0
	TEQNEP	pc, r0			// Back to IRQ mode
	MOV	r0, r0

	LDMIA	sp!, {r0, r1, lr}
	MOV	pc, lr


callback_register_buffer:
	.rept 17
	.word 0
	.endr

in_callback:
	.word 0

callback_stack_registers:
	.word temp_stack
	.word temp_stack + 4096

callback_handler:
	CMP	pc, pc
	BEQ	callback_handler32

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
	MOV	r0, r0

	TEQP	pc, #0			// Switch to user mode!


	BL	arcStubTimerCallback

	LDR	sp, old_sp		// Restore old stack

	MOV	r0, #0
	STR	r0, in_callback

	LDMIA	sp!, {r0-r12}
	LDMIA	sp!, {r14, r15}^


skip_callback:
	LDMIA	r14, {r0-r14}^
	MOV	r0, r0
	LDR	r14, [r14, #15*4]
	MOVS	pc, r14




callback_handler32:
	LDR	r0, in_callback
	CMP	r0, #0
	BNE	skip_callback32

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
	LDR	r0, [r14, #16*4]	// SPSR
	STR	r0, [r12, #-4]!

	STR	r12, old_sp

	// Switch to temporary stack for the next call
	ADR	sl, callback_stack_registers
	LDMIA	sl, {sl, sp}^
	MOV	r0, r0

	LDR	r14, [r14, #16*4]
	MSR	spsr, r14
	MOVS	pc, pc			// Switch to user mode!
	MOV	r0, r0


	BL	arcStubTimerCallback

	LDR	sp, old_sp		// Restore old stack

	MOV	r0, #0
	STR	r0, in_callback

	LDR	r0, [sp], #4
	MSR	cpsr_f, r0
	LDMIA	sp!, {r0-r12, r14, r15}


	
skip_callback32:
	ADR 	r14, callback_register_buffer

	LDR	r0, [r14, #16*4]
	MSR	spsr, r0
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

