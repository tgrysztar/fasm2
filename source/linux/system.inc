
LINE_FEED = 0Ah

O_ACCMODE  = 0003o
O_RDONLY   = 0000o
O_WRONLY   = 0001o
O_RDWR	   = 0002o
O_CREAT    = 0100o
O_EXCL	   = 0200o
O_NOCTTY   = 0400o
O_TRUNC    = 1000o
O_APPEND   = 2000o
O_NONBLOCK = 4000o

S_ISUID    = 4000o
S_ISGID    = 2000o
S_ISVTX    = 1000o
S_IRUSR    = 0400o
S_IWUSR    = 0200o
S_IXUSR    = 0100o
S_IRGRP    = 0040o
S_IWGRP    = 0020o
S_IXGRP    = 0010o
S_IROTH    = 0004o
S_IWOTH    = 0002o
S_IXOTH    = 0001o

system_init:
	mov	eax,13			; sys_time
	mov	ebx,timestamp
	int	0x80
	retn

system_shutdown:
	retn

open:
; in: edx - path to file
; out: ebx = file handle, cf set on error
; preserves: esi, edi
	push	esi edi ebp
	call	adapt_path
	mov	eax,5			; sys_open
	mov	ecx,O_RDONLY
	xor	edx,edx
	int	0x80
	pop	ebp edi esi
	test	eax,eax
	js	interface_error
	mov	ebx,eax
	clc
	retn
    adapt_path:
	xor	ecx,ecx
	mov	ebx,path_buffer
      copy_path:
	mov	al,[edx+ecx]
	cmp	al,'\'
	jne	path_char_ok
	mov	al,'/'
      path_char_ok:
	cmp	ecx,1000h
	jae	out_of_memory
	mov	[ebx+ecx],al
	inc	ecx
	test	al,al
	jnz	copy_path
	retn
    interface_error:
	stc
	retn
create:
; in: edx - path to file, al = executable flag
; out: ebx = file handle, cf set on error
; preserves: esi, edi
	push	esi edi ebp
	mov	edi,S_IRUSR+S_IWUSR+S_IRGRP+S_IROTH
	test	al,al
	jz	file_attributes_ok
	or	edi,S_IXUSR+S_IXGRP+S_IXOTH
      file_attributes_ok:
	call	adapt_path
	mov	eax,5			; sys_open
	mov	ecx,O_CREAT+O_TRUNC+O_WRONLY
	mov	edx,edi
	int	0x80
	pop	ebp edi esi
	test	eax,eax
	js	interface_error
	mov	ebx,eax
	clc
	retn
write:
; in: ebx = file handle, edx - data, ecx = number of bytes
; out: cf set on error
; preserves: ebx, esi, edi
	push	ebx ecx esi edi ebp
	mov	eax,4			; sys_write
	xchg	ecx,edx
	int	0x80
	pop	ebp edi esi ecx ebx
	test	eax,eax
	js	interface_error
	cmp	eax,ecx
	jne	interface_error
	clc
	ret
read:
; in: ebx = file handle, edx - buffer, ecx = number of bytes
; out: cf set on error
; preserves: ebx, esi, edi
	push	ebx ecx esi edi ebp
	mov	eax,3			; sys_read
	xchg	ecx,edx
	int	0x80
	pop	ebp edi esi ecx ebx
	test	eax,eax
	js	interface_error
	cmp	eax,ecx
	jne	interface_error
	clc
	ret
close:
; in: ebx = file handle
; preserves: ebx, esi, edi
	push	ebx esi edi ebp
	mov	eax,6			; sys_close
	int	0x80
	pop	ebp edi esi ebx
	ret
lseek:
; in: ebx = file handle, cl = method, edx:eax = offset
; out: edx:eax = new offset from the beginning of file, cf set on error
; preserves: ebx, esi, edi
	test	edx,edx
	jnz	interface_error
	push	esi edi ebx ebp
	movzx	edi,cl
	mov	ecx,edx
	mov	edx,eax
	mov	eax,140 		; sys_llseek
	mov	esi,loff
	int	0x80
	pop	ebp ebx edi esi
	test	eax,eax
	js	interface_error
	mov	eax,dword [loff]
	mov	edx,dword [loff+4]
	clc
	ret

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
	push	ebx esi ebp
	test	ecx,ecx
	jnz	write_string_to_stdout
	xor	al,al
	mov	edi,esi
	or	ecx,-1
	repne	scasb
	neg	ecx
	sub	ecx,2
   write_string_to_stdout:
	mov	eax,4			; sys_write
	mov	ebx,1
	mov	edx,ecx
	mov	ecx,esi
	int	0x80
	pop	ebp esi ebx
	retn

display_error_string:
; in:
;  esi - string
;  ecx = string length, zero for ASCIIZ string
; preserves: ebx, esi
	push	ebx esi ebp
	test	ecx,ecx
	jnz	write_string_to_stderr
	xor	al,al
	mov	edi,esi
	or	ecx,-1
	repne	scasb
	neg	ecx
	sub	ecx,2
   write_string_to_stderr:
	mov	eax,4			; sys_write
	mov	ebx,2
	mov	edx,ecx
	mov	ecx,esi
	int	0x80
	pop	ebp esi ebx
	retn

get_environment_variable:
; in:
;  esi - name
;  edi - buffer for value
;  ecx = size of buffer
; out:
;  eax = length of value
; preserves: ebx, esi, edi
	push	ebx ecx
	mov	edx,[env]
      scan_environment:
	mov	ebx,[edx]
	test	ebx,ebx
	jz	no_environment_variable
	xor	ecx,ecx
      compare_character:
	mov	al,[ebx+ecx]
	mov	ah,[esi+ecx]
	inc	ecx
	cmp	al,'='
	je	end_of_variable_name
	test	ah,ah
	jz	next_variable
	sub	ah,al
	je	compare_character
	cmp	ah,20h
	jne	next_variable
	cmp	al,'A'
	jb	next_variable
	cmp	al,'Z'
	jna	compare_character
      next_variable:
	add	edx,4
	jmp	scan_environment
      end_of_variable_name:
	test	ah,ah
	jnz	next_variable
	add	ebx,ecx
	pop	ecx
	xor	eax,eax
      copy_environment_variable:
	mov	dl,[ebx+eax]
	cmp	eax,ecx
	jae	next_environment_variable_character
	mov	[edi+eax],dl
      next_environment_variable_character:
	inc	eax
	test	dl,dl
	jnz	copy_environment_variable
      environment_variable_ok:
	pop	ebx
	ret
      no_environment_variable:
	pop	ecx
	mov	eax,1
	jecxz	environment_variable_ok
	and	byte [edi],0
	pop	ebx
	ret

VALLOC_MINIMUM_SIZE = 100000h

valloc:
; in: ecx = requested minimum size
; out: eax - allocated block, ecx = allocated size, zero if failed
; preserves: ebx, esi, edi
	cmp	ecx,VALLOC_MINIMUM_SIZE
	jbe	valloc_size_minimum
	dec	ecx
	and	ecx,(-1) shl 12
	add	ecx,1 shl 12
	jmp	valloc_size_ready
    valloc_size_minimum:
	mov	ecx,VALLOC_MINIMUM_SIZE
    valloc_size_ready:
	push	ebx edi
	mov	ebx,mmap_args
	mov	edi,ebx
	xor	eax,eax
	stosd
	mov	eax,ecx
	stosd
	mov	eax,3			; PROT_READ + PROT_WRITE
	stosd
	mov	eax,22h 		; MAP_PRIVATE + MAP_ANONYMOUS
	stosd
	or	eax,-1
	stosd
	xor	eax,eax
	stosd
	mov	eax,0x5A		; old_mmap
	int	0x80
	cmp	eax,-1
	jne	valloc_ok
	xor	ecx,ecx
    valloc_ok:
	pop	edi ebx
	retn
