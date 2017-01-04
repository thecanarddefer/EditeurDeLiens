	.cpu arm7tdmi
	.fpu softvfp
	.eabi_attribute 20, 1
	.eabi_attribute 21, 1
	.eabi_attribute 23, 3
	.eabi_attribute 24, 1
	.eabi_attribute 25, 1
	.eabi_attribute 26, 1
	.eabi_attribute 30, 6
	.eabi_attribute 34, 0
	.eabi_attribute 18, 4
	.file	"test1.c"
	.section	.rodata
	.align	2
.LC0:
	.ascii	"Hello les L3 !\000"
	.text
	.align	2
	.global	main
	.type	main, %function
main:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 1, uses_anonymous_args = 0
	stmfd	sp!, {fp, lr}
	add	fp, sp, #4
	sub	sp, sp, #8
	str	r0, [fp, #-8]
	str	r1, [fp, #-12]
	ldr	r0, .L3
	bl	puts
	mov	r3, #0
	mov	r0, r3
	sub	sp, fp, #4
	@ sp needed
	ldmfd	sp!, {fp, pc}
.L4:
	.align	2
.L3:
	.word	.LC0
	.size	main, .-main
	.ident	"GCC: (4.8.4-1+11-1) 4.8.4 20141219 (release)"
