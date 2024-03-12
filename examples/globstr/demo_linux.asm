
; Global strings demonstration for Linux.

format ELF64 executable 3
entry start

include 'globstr.inc'
include 'macro/inline.inc'

	inlinemacro GLOB(value&)
		local data
		data GLOBSTR value,0
		return equ data
	end inlinemacro

segment readable executable

    start:
	lea	rsi,[GLOB("Hello, it's ")]
	call	display_string

	mov	rsi,[rsp+8]	; argv
	call	display_string

	lea	rsi,[GLOB(' speaking.',10)]
	call	display_string

	xor	edi,edi 	; exit code 0
	mov	eax,60		; sys_exit
	syscall

    display_string:		; rsi = address of ASCIIZ string
	xor	al,al
	mov	rdi,rsi
	or	rcx,-1
	repne	scasb
	neg	rcx
	sub	rcx,2
	mov	edi,1
	mov	eax,1		; sys_write
	mov	rdx,rcx
	syscall
	retn

segment readable

	GLOBSTR.here
