; NOTE
; CALL:      RCX, RDX, R8, R9
; VOLATILE:  RCX, RDX, R8, R9, RAX, R10, R11
; PRESERVED: RBX, RDI, RSI, RSP, RBP, R12, R13, R14, R15

.CODE
ALIGN 16
;=========================================================================================

; We don't need to set up the stack shadow space in a leaf function.

;; rcx -> uint8_t *vmem
;; rdx -> size_t   addr
;; r8d -> uint32_t data
;; r9d -> int      numBits (one byte)
openVCB_evil_assembly_bit_manipulation_routine_setVMem PROC PUBLIC

	; If one attempts to shift a 32-bit value by 32, it will be masked to 0 resulting
	; in a no-op. We do one of these shifts this in 64-bits instead to avoid that.
	shrx	eax, [rcx + rdx], r9d
	shlx	rax, rax, r9
	or	eax, r8d
	mov	[rcx + rdx], eax
	ret
	int	3

openVCB_evil_assembly_bit_manipulation_routine_setVMem ENDP

;---------------------------------------------------------------------------------------

ALIGN 16

;; rcx -> uint8_t *vmem
;; rdx -> size_t   addr
openVCB_evil_assembly_bit_manipulation_routine_getVMem PROC PUBLIC

	mov	eax, [rcx + rdx]
	ret
	int	3

openVCB_evil_assembly_bit_manipulation_routine_getVMem ENDP

;=========================================================================================
ALIGN 16
END
; vim: ft=masm
