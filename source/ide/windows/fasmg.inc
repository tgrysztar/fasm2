
; flat assembler g interface for Windows IDE
; Copyright (c) 2014-2024, Tomasz Grysztar.
; All rights reserved.

flat_assembler:

	mov	esi,[esp+4]
	mov	[source_path],esi

	mov	[maximum_number_of_errors],MAX_ERRORS

	xor	al,al
	call	assembly_init

	mov	eax,[maximum_number_of_passes]
	shl	eax,16
	invoke	PostMessage,[hwnd_progress],PBM_SETRANGE,0,eax
	invoke	PostMessage,[hwnd_progress],PBM_SETPOS,0,0
	invoke	GetTickCount
	mov	[timer],eax

  assemble:
	mov	esi,source_header
	mov	edx,[source_path]
	call	assembly_pass
	jc	assembly_done

	mov	ebx,[current_pass]
	invoke	PostMessage,[hwnd_progress],PBM_SETPOS,ebx,0
	cmp	ebx,[maximum_number_of_passes]
	jb	assemble

	call	show_display_data
	cmp	[first_error],0
	jne	assembly_failed

	call	assembly_shutdown
	mov	esi,_code_cannot_be_generated
	jmp	fatal_error

  assembly_done:
	call	show_display_data
	cmp	[first_error],0
	jne	assembly_failed

	mov	[executable_path],0
	mov	ebx,[source_path]
	mov	edi,[output_path]
	call	write_output_file
	jc	write_failed

	mov	eax,[current_pass]
	xor	edx,edx
	call	itoa
	call	display_error_string
	mov	esi,_passes
	cmp	[current_pass],1
	jne	display_passes_suffix
	mov	esi,_pass
      display_passes_suffix:
	xor	ecx,ecx
	call	display_error_string
	invoke	GetTickCount
	sub	eax,[timer]
	xor	edx,edx
	add	eax,50
	mov	ecx,1000
	div	ecx
	mov	ebx,eax
	mov	eax,edx
	xor	edx,edx
	mov	ecx,100
	div	ecx
	mov	[timer],eax
	xchg	eax,ebx
	or	ebx,eax
	jz	display_output_length
	xor	edx,edx
	call	itoa
	call	display_error_string
	mov	esi,_message_suffix
	mov	ecx,1
	call	display_error_string
	mov	eax,[timer]
	xor	edx,edx
	call	itoa
	call	display_error_string
	mov	esi,_seconds
	xor	ecx,ecx
	call	display_error_string
      display_output_length:
	call	get_output_length
	push	eax edx
	call	itoa
	call	display_error_string
	pop	edx eax
	mov	esi,_bytes
	cmp	eax,1
	jne	display_bytes_suffix
	test	edx,edx
	jnz	display_bytes_suffix
	mov	esi,_byte
      display_bytes_suffix:
	xor	ecx,ecx
	call	display_error_string
	mov	esi,_new_line
	xor	ecx,ecx
	call	display_error_string
      summary_done:

	call	assembly_shutdown
	xor	eax,eax
	jmp	shutdown

  assembly_failed:

	mov	ebx,[first_error]
  collect_error:
	call	show_error_message
	xor	esi,esi
	mov	ecx,1
	call	display_error_string
	mov	esi,[ebx+Error.preprocessed_data]
	mov	ecx,[ebx+Error.preprocessed_length]
	push	ebx
	call	show_preprocessed_line
	pop	ebx
	xor	esi,esi
	mov	ecx,1
	call	display_error_string
	mov	eax,[ebx+sizeof.Error+SourceContext.number_of_entries]
	test	eax,eax
	jz	display_error_message
	lea	esi,[ebx+sizeof.Error+sizeof.SourceContext]
	dec	eax
	imul	eax,sizeof.SourceEntry
	lea	edi,[esi+eax]
	push	esi edi
	lea	esi,[edi+SourceEntry.type]
	mov	ecx,1
	call	display_error_string
	pop	edi esi
      find_file_source:
	cmp	[edi+SourceEntry.type],SOURCE_FILE
	jne	try_next_source_entry
	cmp	[edi+SourceEntry.line_number],0
	jne	file_source_found
      try_next_source_entry:
	cmp	edi,esi
	je	no_file_source
	sub	edi,sizeof.SourceEntry
	jmp	find_file_source
      file_source_found:
	lea	esi,[edi+SourceEntry.line_number]
	mov	ecx,4
	push	edi
	call	display_error_string
	pop	edi
	mov	esi,[edi+SourceEntry.name]
	call	display_error_string
	jmp	error_collected
      no_file_source:
	xor	esi,esi
	mov	ecx,4
	call	display_error_string
      error_collected:
	xor	esi,esi
	mov	ecx,1
	call	display_error_string
	mov	ebx,[ebx+Error.next]
	test	ebx,ebx
	jnz	collect_error

	call	assembly_shutdown
	mov	eax,2
	jmp	shutdown

  write_failed:
	call	assembly_shutdown
	mov	esi,_write_failed
	jmp	fatal_error
  out_of_memory:
	call	assembly_shutdown
	mov	esi,_out_of_memory
  fatal_error:
	xor	ecx,ecx
	call	display_error_string
	mov	eax,1

shutdown:
	push	eax
	invoke	PostMessage,[hwnd_compiler],WM_COMMAND,IDOK,0
	call	[ExitThread]

LINE_FEED equ 13,10

system_init:
	invoke	HeapCreate,0,20000h,0
	mov	[memory],eax
	test	eax,eax
	jz	out_of_memory
	invoke	GetSystemTime,systemtime
	invoke	SystemTimeToFileTime,systemtime,filetime
	mov	ebx,[filetime.dwLowDateTime]
	mov	eax,[filetime.dwHighDateTime]
	sub	ebx,116444736000000000 and 0FFFFFFFFh
	sbb	eax,116444736000000000 shr 32
	xor	edx,edx
	mov	ecx,10000000
	div	ecx
	mov	dword [timestamp+4],eax
	mov	eax,ebx
	div	ecx
	mov	dword [timestamp],eax
	retn

system_shutdown:
	cmp	[memory],0
	je	memory_released
	invoke	HeapDestroy,[memory]
    memory_released:
	retn

malloc:
malloc_fixed:
malloc_growable:
; in: ecx = requested size
; out: eax - allocated block, ecx = allocated size, on error jumps to out_of_memory (does not return)
; preserves: ebx, esi, edi
; note:
;  use of malloc_fixed hints that block will be kept as is until the end of assembly
;  use of malloc_growable hints that block is likely to be resized
	invoke	HeapAlloc,[memory],0,ecx
	test	eax,eax
	jz	out_of_memory
    memory_allocated:
	push	eax
	invoke	HeapSize,[memory],0,eax
	mov	ecx,eax
	pop	eax
	cmp	ecx,-1
	je	out_of_memory
	retn
realloc:
; in: eax - memory block, ecx = requested size
; out: eax - resized block, ecx = allocated size, on error jumps to out_of_memory (does not return)
; preserves: ebx, esi, edi
	invoke	HeapReAlloc,[memory],0,eax,ecx
	test	eax,eax
	jnz	memory_allocated
	jmp	out_of_memory
mfree:
; in: eax - memory block
; out: cf set on error
; preserves: ebx, esi, edi
; note: eax may have value 0 or -1, it should be treated as invalid input then
	test	eax,eax
	jz	interface_error
	cmp	eax,-1
	je	interface_error
	invoke	HeapFree,[memory],0,eax
	test	eax,eax
	jz	interface_error
	clc
	retn
    interface_error:
	stc
	retn

open:
; in: edx - path to file
; out: ebx = file handle, cf set on error
; preserves: esi, edi
	mov	ebx,edx
	invoke	WaitForSingleObject,[mutex],-1
	invoke	CreateFile,ebx,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0
	cmp	eax,-1
	je	release_mutex
	mov	ebx,eax
	clc
	retn
create:
; in: edx - path to file
; out: ebx = file handle, cf set on error
; preserves: esi, edi
	cmp	[executable_path],0
	jne	path_catching_done
	push	esi edi
	mov	esi,edx
	mov	edi,executable_path
    catch_path:
	lodsb
	stosb
	test	al,al
	jnz	catch_path
	pop	edi esi
    path_catching_done:
	mov	ebx,edx
	invoke	WaitForSingleObject,[mutex],-1
	invoke	CreateFile,ebx,GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0
	cmp	eax,-1
	je	release_mutex
	mov	ebx,eax
	clc
	retn
write:
; in: ebx = file handle, edx - data, ecx = number of bytes
; out: cf set on error
; preserves: ebx, esi, edi
	push	ecx
	invoke	WriteFile,ebx,edx,ecx,systmp,0
	pop	ecx
	test	eax,eax
	jz	interface_error
	cmp	ecx,[systmp]
	jne	interface_error
	clc
	retn
read:
; in: ebx = file handle, edx - buffer, ecx = number of bytes
; out: cf set on error
; preserves: ebx, esi, edi
	push	ecx
	invoke	ReadFile,ebx,edx,ecx,systmp,0
	pop	ecx
	test	eax,eax
	jz	interface_error
	cmp	ecx,[systmp]
	jne	interface_error
	clc
	retn
close:
; in: ebx = file handle
; preserves: ebx, esi, edi
	invoke	CloseHandle,ebx
    release_mutex:
	invoke	ReleaseMutex,[mutex]	; this relies on the fact that engine never opens more than one file at once
	stc
	retn
lseek:
; in: ebx = file handle, cl = method, edx:eax = offset
; out: edx:eax = new offset from the beginning of file, cf set on error
; preserves: ebx, esi, edi
	movzx	ecx,cl
	mov	[systmp],edx
	invoke	SetFilePointer,ebx,eax,systmp,ecx
	cmp	eax,-1
	jne	lseek_ok
	invoke	GetLastError
	test	eax,eax
	jnz	interface_error
	not	eax
    lseek_ok:
	mov	edx,[systmp]
	clc
	retn

get_timestamp:
; out: edx:eax = timestamp
; preserves: ebx, ecx, esi, edi
; note: during the passes of a single assembly this function should always return the same value
	mov	eax,dword [timestamp]
	mov	edx,dword [timestamp+4]
	retn

display_string:
; in:
;  esi - string
;  ecx = string length, zero for ASCIIZ string
; preserves: ebx, esi
	push	ebx
	mov	ebx,stdout
	jmp	write_string
display_error_string:
; in:
;  esi - string
;  ecx = string length, zero for ASCIIZ string
; preserves: ebx, esi
	push	ebx esi
	mov	ebx,stderr
   write_string:
	test	ecx,ecx
	jnz	write_string_to_buffer
	xor	al,al
	mov	edi,esi
	or	ecx,-1
	repne	scasb
	neg	ecx
	sub	ecx,2
   write_string_to_buffer:
	mov	edi,[ebx]
	push	ecx
	invoke	HeapSize,[hheap],0,dword [ebx]
	mov	edx,eax
	sub	eax,4
	sub	eax,[edi]
	mov	ecx,[esp]
	sub	ecx,eax
	jb	string_buffer_ready	; do not fit tight, leave room for terminating zero
	add	edx,ecx
	inc	edx
	invoke	HeapReAlloc,[hheap],0,dword [ebx],edx
	test	eax,eax
	jz	shutdown
	mov	[ebx],eax
	mov	edi,eax
    string_buffer_ready:
	pop	ecx
	mov	eax,[edi]
	add	[edi],ecx
	lea	edi,[edi+4+eax]
	xor	al,al
	test	esi,esi
	jz	zero_string
	rep	movsb
	stosb
	pop	esi ebx
	retn
    zero_string:
	rep	stosb
	pop	esi ebx
	retn

get_environment_variable:
; in:
;  esi - name
;  edi - buffer for value
;  ecx = size of buffer
; out:
;  eax = length of value
; preserves: ebx, esi, edi
	push	ecx
	invoke	GetEnvironmentVariable,esi,string_buffer,1000h
	invoke	GetPrivateProfileString,_section_environment,esi,string_buffer,string_buffer,1000h,ini_path
	mov	ecx,[esp]
	push	esi edi
	mov	esi,string_buffer
    copy_env_val:
	lodsb
	jecxz	next_env_char
	mov	[edi],al
	dec	ecx
      next_env_char:
	inc	edi
	test	al,al
	jnz	copy_env_val
	mov	eax,edi
	pop	edi esi ecx
	sub	eax,edi
	retn

  include '../../assembler.inc'
  include '../../symbols.inc'
  include '../../expressions.inc'
  include '../../conditions.inc'
  include '../../floats.inc'
  include '../../directives.inc'
  include '../../calm.inc'
  include '../../errors.inc'
  include '../../map.inc'
  include '../../reader.inc'
  include '../../output.inc'
  include '../../console.inc'

section '.rdata' data readable

  include '../../tables.inc'
  include '../../messages.inc'

  _pass db ' pass, ',0
  _passes db ' passes, ',0
  _dot db '.'
  _seconds db ' seconds, ',0
  _byte db ' byte.',0
  _bytes db ' bytes.',0

  _write_failed db 'Failed to write the output file.',0
  _out_of_memory db 'Not enough memory to complete the assembly.',0
  _code_cannot_be_generated db 'Could not generate code within the allowed number of passes.',0

  version_string db VERSION,0

section '.bss' readable writeable

  include '../../variables.inc'

  source_string dd ?
  source_path dd ?
  output_path dd ?

  maximum_number_of_passes dd ?

  timestamp dq ?
  systemtime SYSTEMTIME
  filetime FILETIME

  memory dd ?
  systmp dd ?

  timer dd ?

  stdout dd ?
  stderr dd ?
