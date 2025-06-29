	.text

	.global mixer_arc_callback

	.global	mixer_channels
	.global sound_config_data

	.set	MixerChannelActive, 0
	.set	MixerChannelVolume, 1
	.set	MixerChunkData, 4
	.set	MixerChunkLen,  8
	.set	MixerChunkLoopPos,  10
	.set	MixerChunkLoopLen,  12
	.set	MixerChannelChunkPos, 16
	.set	MixerChannelChunkInc, 20

mixer_channels_p:
	.word mixer_channels

sound_config_data_p:
	.word sound_config_data

@ r10 = DMA buffer end (+1)
@ r11 = DMA buffer inc
@ r12 = DMA buffer base

mixer_arc_callback:
	STMFD	sp!, {r0-r12, lr}

	LDR	r7, sound_config_data_p
	LDR	r7, [r7, #12]           	// old channel handler header
	LDR	r7, [r7, #8]            	// linear -> log table
	LDR	r8, mixer_channels_p
	LDR	r8, [r8]

	MOV	r9, #4   			// Channel count

mixer_channel_loop:
	STR	r12, [sp, #-4]!	

	LDRB	r0, [r8, #MixerChannelActive]   // active
	CMP	r0, #0
	BEQ	mixer_channel_zero_loop


	LDRB	r2, [r8, #MixerChannelVolume]	// volume
	LDRB	r2, [r7, r2, LSL #6]		// log volume
	BIC	r2, r2, #1
	RSB	r2, r2, #0xfe			// log attenuation

	LDR	r5, [r8, #MixerChunkData]    	// data
	LDR	r6, [r8, #MixerChunkLen]	// length
	MOV	r6, r6, LSL #16
	MOV	r6, r6, LSR #16
	LDR	r3, [r8, #MixerChannelChunkPos]	// chunkPos
	LDR	r4, [r8, #MixerChannelChunkInc]	// chunkInc

	LDR	r1, [r8, #MixerChunkLoopLen]	// loopLen
	MOV	r1, r1, LSL #16
	MOV	r1, r1, LSR #16
	CMP	r1, #0
	BNE	mixer_channel_looped


mixer_channel_inner_loop:
	LDRB	r0, [r5, r3, LSR #8]
	LDRB	r0, [r7, r0, LSL #5]
	SUBS	r0, r0, r2			// Apply volume and clamp
	MOVMI	r0, #0
	STRB	r0, [r12], r11

	ADD	r3, r3, r4
	CMP	r6, r3, LSR #8			// hit the end?
	BLT	mixer_channel_sample_end	


	CMP	r12, r10
	BLT	mixer_channel_inner_loop


mixer_channel_loop_continue:
	STR	r3, [r8, #MixerChannelChunkPos]	// chunkPos
	LDR	r12, [sp], #4
	ADD	r12, r12, #1
	ADD	r8, r8,	#24
	SUBS	r9, r9, #1
	BNE	mixer_channel_loop

	LDMFD	sp!, {r0-r12, pc}


mixer_channel_sample_end:
	MOV	r0, #0
	STRB	r0, [r8]			// mark as inactive
	B	mixer_channel_zero_loop_continue


mixer_channel_zero_loop:
	STRB	r0, [r12], r11

mixer_channel_zero_loop_continue:
	CMP	r12, r10
	BLT	mixer_channel_zero_loop

	B	mixer_channel_loop_continue


mixer_channel_looped:
	LDR	r6, [r8, #MixerChunkLoopPos]	// loopPos
	MOV	r6, r6, LSL #16
	MOV	r6, r6, LSR #16
	ADD	r6, r1, r6              	// loopPos + loopLen

mixer_channel_looped_loop:
	LDRB	r0, [r5, r3, LSR #8]
	LDRB	r0, [r7, r0, LSL #5]
	SUBS	r0, r0, r2			// Apply volume and clamp
	MOVMI	r0, #0
	STRB	r0, [r12], r11

	ADD	r3, r3, r4
	CMP	r6, r3, LSR #8			// hit the end?
	SUBLT	r3, r3, r1, LSL #8		// subtract loopLen

	CMP	r12, r10
	BLT	mixer_channel_looped_loop

	B	mixer_channel_loop_continue

