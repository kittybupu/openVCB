bits 64
default REL

; SYSV
; USERSPACE: RDI, RSI, RDX, RCX, R8, R9
; VOLATILE:  RDI, RSI, RDX, RCX, R8, R9, RAX, R10, R11
; PRESERVED: RSP, RBP, RBX, R12, R13, R14, R15
; KERNEL:    RDI, RSI, RDX, R10, R8, R9
;
; WIN32
; USERSPACE: RCX, RDX, R8, R9
; VOLATILE:  RCX, RDX, R8, R9, RAX, R10, R11
; PRESERVED: RBX, RDI, RSI, RSP, RBP, R12, R13, R14, R15
; KERNEL:    N/A
;=========================================================================================

GLOBAL openVCB_evil_assembly_bit_manipulation_routine_setVMem
GLOBAL openVCB_evil_assembly_bit_manipulation_routine_getVMem

SECTION .text
ALIGN 16, int3
;=========================================================================================

;; rdi -> uint8_t *vmem
;; rsi -> size_t   addr
;; edx -> uint32_t data
;; ecx -> int      numBits (one byte)
openVCB_evil_assembly_bit_manipulation_routine_setVMem:
	shrx	eax, [rdi + rsi], ecx
	shlx	rax, rax, rcx
	or	eax, edx
	mov	[rdi + rsi], eax
	ret
	int3

;---------------------------------------------------------------------------------------

ALIGN 16, int3

;; rdi -> uint8_t *vmem
;; rsi -> size_t   addr
openVCB_evil_assembly_bit_manipulation_routine_getVMem:
	mov	eax, [rdi + rsi]
	ret
	int3

;=========================================================================================
ALIGN 16, int3
; vim: ft=nasm
