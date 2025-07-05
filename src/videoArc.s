	.text

	.global fillPolygonSegment

//	fillPolygonSegment(h, cpt1, cpt2, color, step1, step2, _hliney, _curPagePtr1, _pages[0])

// a1 = h
// a2 = cpt1
// a3 = cpt2
// a4 = color
// [sp] = step1
// [sp+4] = step2
// [sp+8] = _hliney
// [sp+12] = write page
// [sp+16] = read page
fillPolygonSegment:
	CMP	a1, #0			// Skip if height is 0
	MOVEQ	pc, lr

	STMFD	sp!, {v1, v2, v3, v4, v5, v6, ip, lr}

	LDR	v6, [sp, #(32 + 8)]	// _hliney
	CMP	v6, #0
	BGE	0f

	// Top clip

	ADD	a1, a1, v6		// Remove clipped lines
	CMP	a1, #0			// Any lines left?
	LDMLEFD	sp!, {v1, v2, v3, v4, v5, v6, ip, pc}

	// Adjust cpt1, cpt2
	ADD	v5, sp, #32
	LDMIA	v5, {v4, v5}		// step1, step2
	RSB	v6, v6, #0
	MLA	a2, v4, v6, a2
	MLA	a3, v5, v6, a3
	MOV	v6, #0

0:
	ADD	v5, v6, a1		// End line
	RSBS	v5, v5, #200		// Lines from end of segment to end of screen
	ADDMI	a1, a1, v5		// Clip if necessary
	CMP	a1, #0			// Any lines left?
	LDMLEFD	sp!, {v1, v2, v3, v4, v5, v6, ip, pc}

	LDR	ip, [sp, #(32 + 12)]	// Write page
	ADD	ip, ip, v6, LSL #7
	ADD	ip, ip, v6, LSL #5	// Pointer to start of first line

	CMP	a4, #0x10
	BCC	fillPolygonSegmentN
	BHI	fillPolygonSegmentP

	// Fall through

fillPolygonSegmentBlend:
	// Fill a4 with colour
	MOV	a4, #0x88
	ORR	a4, a4, a4, LSL #8
	ORR	a4, a4, a4, LSL #16

0:
	STMDB	sp!, {a1, a2, a3}
	MOV	a2, a2, ASR #16
	MOV	a3, a3, ASR #16

	CMP	a2, #320	// Full line clip
	BGE	9f
	CMP	a3, #0
	BLT	9f

	CMP	a2, #0
	MOVLT	a2, #0
	CMP	a3, #320
	MOVGE	a3, #320
	SUBGE	a3, a3, #1


	// Plot line

	// Sort coordinates
	CMP	a2, a3
	MOVGT	lr, a2
	MOVGT	a2, a3
	MOVGT	a3, lr

	ADD	a3, a3, #1

	ADD	a1, ip, a2, LSR #1
	BIC	a1, a1, #3		// Word aligned start address

	EOR	v1, a2, a3
	CMP	v1, #7
	BLS	fillPolygonSegmentBlendSameWord	// Line starts and ends in the same word


	// Align to first word
	ANDS	v3, a2, #7
	BEQ	1f

	LDR	v1, [a1]
	ADR	v2, first_word_table
	LDR	v3, [v2, v3, LSL #2]
	AND	v2, a4, v3
	ORR	v1, v1, v2
	STR	v1, [a1], #4

	ADD	a2, a2, #7
	BIC	a2, a2, #7
	
1:
	MOV	v2, a4

	SUB	v1, a3, a2		// Width
	CMP	v1, #32
	BCC	3f

	MOV	v3, a4
	MOV	v4, a4

2:
	LDMIA	a1, {v2, v3, v4, v5}
	ORR	v2, v2, a4
	ORR	v3, v3, a4
	ORR	v4, v4, a4
	ORR	v5, v5, a4
	STMIA	a1!, {v2, v3, v4, v5}
	SUBS	v1, v1, #32
	CMP	v1, #32
	BCS	2b

3:
	CMP	v1, #16
	SUBCS	v1, #16
	LDMCSIA a1, {v2, v3}
	ORRCS	v2, v2, a4
	ORRCS	v3, v3, a4
	STMCSIA	a1!, {v2, v3}

	CMP	v1, #8
	SUBCS	v1, #8
	LDRCS	v2, [a1]
	ORRCS	v2, v2, a4
	STRCS	v2, [a1], #4

	// Final word
	CMP	v1, #0
	BEQ	9f

	LDR	v2, [a1]
	ADR	a2, last_word_table
	LDR	a2, [a2, v1, LSL #2]
	AND	a2, a4, a2
	ORR	v2, v2, a2
	STR	v2, [a1]

	B	9f


fillPolygonSegmentBlendSameWord:
	LDR	v1, [a1]
	AND	a2, a2, #7
	AND	a3, a3, #7
	ORR	a2, a2, a3, LSL #3
	ADR	a3, same_word_table
	LDR	a2, [a3, a2, LSL #2]
	AND	a2, a4, a2
	ORR	v1, v1, a2
	STR	v1, [a1]	



9:
	LDMIA	sp!, {a1, a2, a3}	// restore cpt1, cpt2, h

	ADD	v5, sp, #32
	LDMIA	v5, {v4, v5} 		// step1 & 2

	ADD	a2, a2, v4
	ADD	a3, a3, v5
	ADD	ip, ip, #160

	SUBS	a1, a1, #1
	BNE	0b

	LDMFD	sp!, {v1, v2, v3, v4, v5, v6, ip, pc}



fillPolygonSegmentN:
	// Fill a4 with colour
	ORR	a4, a4, a4, LSL #4
	ORR	a4, a4, a4, LSL #8
	ORR	a4, a4, a4, LSL #16

0:
	STMDB	sp!, {a1, a2, a3}
	MOV	a2, a2, ASR #16
	MOV	a3, a3, ASR #16

	CMP	a2, #320	// Full line clip
	BGE	9f
	CMP	a3, #0
	BLT	9f

	CMP	a2, #0
	MOVLT	a2, #0
	CMP	a3, #320
	MOVGE	a3, #320
	SUBGE	a3, a3, #1


	// Plot line

	// Sort coordinates
	CMP	a2, a3
	MOVGT	lr, a2
	MOVGT	a2, a3
	MOVGT	a3, lr

	ADD	a3, a3, #1

	ADD	a1, ip, a2, LSR #1
	BIC	a1, a1, #3		// Word aligned start address

	EOR	v1, a2, a3
	CMP	v1, #7
	BLS	fillPolygonSegmentNSameWord	// Line starts and ends in the same word


	// Align to first word
	ANDS	v3, a2, #7
	BEQ	1f

	LDR	v1, [a1]
	ADR	v2, first_word_table
	LDR	v3, [v2, v3, LSL #2]
	BIC	v1, v1, v3
	AND	v2, a4, v3
	ORR	v1, v1, v2
	STR	v1, [a1], #4

	ADD	a2, a2, #7
	BIC	a2, a2, #7
	
1:
	MOV	v2, a4

	SUB	v1, a3, a2		// Width
	CMP	v1, #32
	BCC	3f

	MOV	v3, a4
	MOV	v4, a4

2:
	STMIA	a1!, {a4, v2, v3, v4}
	SUBS	v1, v1, #32
	CMP	v1, #32
	BCS	2b

3:
	CMP	v1, #16
	SUBCS	v1, #16
	STMCSIA	a1!, {a4, v2}

	CMP	v1, #8
	SUBCS	v1, #8
	STRCS	a4, [a1], #4

	// Final word
	CMP	v1, #0
	BEQ	9f

	LDR	v2, [a1]
	ADR	a2, last_word_table
	LDR	a2, [a2, v1, LSL #2]
	BIC	v2, v2, a2
	AND	a2, a4, a2
	ORR	v2, v2, a2
	STR	v2, [a1]

	B	9f


fillPolygonSegmentNSameWord:
	LDR	v1, [a1]
	AND	a2, a2, #7
	AND	a3, a3, #7
	ORR	a2, a2, a3, LSL #3
	ADR	a3, same_word_table
	LDR	a2, [a3, a2, LSL #2]
	BIC	v1, v1, a2
	AND	a2, a4, a2
	ORR	v1, v1, a2
	STR	v1, [a1]	



9:
	LDMIA	sp!, {a1, a2, a3}	// restore cpt1, cpt2, h

	ADD	v5, sp, #32
	LDMIA	v5, {v4, v5} 		// step1 & 2

	ADD	a2, a2, v4
	ADD	a3, a3, v5
	ADD	ip, ip, #160

	SUBS	a1, a1, #1
	BNE	0b

	LDMFD	sp!, {v1, v2, v3, v4, v5, v6, ip, pc}


fillPolygonSegmentP:
	LDR	v4, [sp, #(32+16)]	// Read page
	ADD	v4, v4, v6, LSL #7
	ADD	v4, v4, v6, LSL #5	// Pointer to start of first line

0:
	STMDB	sp!, {a1, a2, a3, v4}
	MOV	a2, a2, ASR #16
	MOV	a3, a3, ASR #16

	CMP	a2, #320	// Full line clip
	BGE	9f
	CMP	a3, #0
	BLT	9f

	CMP	a2, #0
	MOVLT	a2, #0
	CMP	a3, #320
	MOVGE	a3, #320
	SUBGE	a3, a3, #1


	// Plot line

	// Sort coordinates
	CMP	a2, a3
	MOVGT	lr, a2
	MOVGT	a2, a3
	MOVGT	a3, lr

	ADD	a3, a3, #1

	ADD	a1, ip, a2, LSR #1
	BIC	a1, a1, #3		// Word aligned start address
	ADD	v4, v4, a2, LSR #1
	BIC	v4, v4, #3

	EOR	v1, a2, a3
	CMP	v1, #7
	BLS	fillPolygonSegmentPSameWord	// Line starts and ends in the same word


	// Align to first word
	ANDS	v3, a2, #7
	BEQ	1f

	LDR	v1, [a1]
	LDR	a4, [v4], #4
	ADR	v2, first_word_table
	LDR	v3, [v2, v3, LSL #2]
	BIC	v1, v1, v3
	AND	v2, a4, v3
	ORR	v1, v1, v2
	STR	v1, [a1], #4

	ADD	a2, a2, #7
	BIC	a2, a2, #7
	
1:
	SUB	v1, a3, a2		// Width
	CMP	v1, #32
	BCC	3f

2:
	LDMIA	v4!, {a4, v2, v3, v5}
	STMIA	a1!, {a4, v2, v3, v5}
	SUBS	v1, v1, #32
	CMP	v1, #32
	BCS	2b

3:
	CMP	v1, #16
	SUBCS	v1, #16
	LDMCSIA v4!, {a4, v2}
	STMCSIA	a1!, {a4, v2}

	CMP	v1, #8
	SUBCS	v1, #8
	LDRCS	a4, [v4], #4
	STRCS	a4, [a1], #4

	// Final word
	CMP	v1, #0
	BEQ	9f

	LDR	v2, [a1]
	LDR	a4, [v4]
	ADR	a2, last_word_table
	LDR	a2, [a2, v1, LSL #2]
	BIC	v2, v2, a2
	AND	a2, a4, a2
	ORR	v2, v2, a2
	STR	v2, [a1]

	B	9f


fillPolygonSegmentPSameWord:
	LDR	v1, [a1]
	LDR	a4, [v4]
	AND	a2, a2, #7
	AND	a3, a3, #7
	ORR	a2, a2, a3, LSL #3
	ADR	a3, same_word_table
	LDR	a2, [a3, a2, LSL #2]
	BIC	v1, v1, a2
	AND	a2, a4, a2
	ORR	v1, v1, a2
	STR	v1, [a1]	



9:
	LDMIA	sp!, {a1, a2, a3, v4}	// restore cpt1, cpt2, h, read pointer

	ADD	v5, sp, #32
	LDMIA	v5, {v3, v5} 		// step1 & 2

	ADD	a2, a2, v3
	ADD	a3, a3, v5
	ADD	v4, v4, #160
	ADD	ip, ip, #160

	SUBS	a1, a1, #1
	BNE	0b

	LDMFD	sp!, {v1, v2, v3, v4, v5, v6, ip, pc}

first_word_table:
	.word	0xffffffff, 0xfffffff0, 0xffffff00, 0xfffff000, 0xffff0000, 0xfff00000, 0xff000000, 0xf0000000
last_word_table:
	.word	0x00000000, 0x0000000f, 0x000000ff, 0x00000fff, 0x0000ffff, 0x000fffff, 0x00ffffff, 0x0fffffff
same_word_table:
	.word	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
	.word	0x0000000f, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
	.word	0x000000ff, 0x000000f0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
	.word	0x00000fff, 0x00000ff0, 0x00000f00, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
	.word	0x0000ffff, 0x0000fff0, 0x0000ff00, 0x0000f000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
	.word	0x000fffff, 0x000ffff0, 0x000fff00, 0x000ff000, 0x000f0000, 0x00000000, 0x00000000, 0x00000000
	.word	0x00ffffff, 0x00fffff0, 0x00ffff00, 0x00fff000, 0x00ff0000, 0x00f00000, 0x00000000, 0x00000000
	.word	0x0fffffff, 0x0ffffff0, 0x0fffff00, 0x0ffff000, 0x0fff0000, 0x0ff00000, 0x0f000000, 0x00000000

