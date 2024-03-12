
; Global strings demonstration for Windows.

include 'win64ax.inc'
include 'globstr.inc'

GLOBSTR.reuse := 1

.data
	GLOBSTR.here
.code

    start:
	; Inline macro allows to put string data in any expression
	; (getting the address in return):
	inlinemacro GLOB(value)
		local data
		data GLOBSTR value,0
		return equ data
	end inlinemacro

	lea	rdx,[GLOB('Hello!')]
	invoke	MessageBox, HWND_DESKTOP, rdx, GLOB('Example'), MB_OK

	; Alternatively, replacing the "fastcall?.inline_string" macro changes
	; how the string arguments to "fastcall" and "invoke" are handled:
	macro fastcall?.inline_string var
		local data
		data GLOBSTR var,0
		redefine var data
	end macro
	; For 32-bit case it would be the "stdcall?.push_string" macro,
	; provided here for completness:
	macro stdcall?.push_string value&
		local data
		data GLOBSTR value,0
		push data
	end macro

	invoke	MessageBox, HWND_DESKTOP, 'And bye!', 'Example', MB_OK

	invoke	ExitProcess,0

.end start
