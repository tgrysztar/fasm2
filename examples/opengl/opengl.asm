
format PE64 NX GUI 5.0
entry start

include 'win64a.inc'

WGL_CONTEXT_MAJOR_VERSION_ARB	:= 0x2091
WGL_CONTEXT_MINOR_VERSION_ARB	:= 0x2092

GL_COLOR_BUFFER_BIT	:= 0x00004000
GL_TRIANGLES		:= 0x0004
GL_TRIANGLE_STRIP	:= 0x0005
GL_FLOAT		:= 0x1406
GL_ARRAY_BUFFER 	:= 0x8892
GL_STATIC_DRAW		:= 0x88E4
GL_VERTEX_SHADER	:= 0x8B31
GL_FRAGMENT_SHADER	:= 0x8B30
GL_COMPILE_STATUS	:= 0x8B81
GL_LINK_STATUS		:= 0x8B82

iterate name,\
	glCreateShader,\
	glShaderSource,\
	glCompileShader,\
	glGetShaderiv,\
	glCreateProgram,\
	glAttachShader,\
	glLinkProgram,\
	glGetProgramiv,\
	glDeleteShader,\
	glGenVertexArrays,\
	glBindVertexArray,\
	glGenBuffers,\
	glBindBuffer,\
	glBufferData,\
	glVertexAttribPointer,\
	glEnableVertexAttribArray,\
	glUseProgram,\
	glGetUniformLocation,\
	glUniform1f,\
	glUniform3f

	define CONTEXT_AWARE_FUNCTION name

end iterate

section '.data' data readable writeable

  wc WNDCLASS style:0, lpfnWndProc:WindowProc, lpszClassName:_class

  attribs:
	dd WGL_CONTEXT_MAJOR_VERSION_ARB, 3
	dd WGL_CONTEXT_MINOR_VERSION_ARB, 3
	dd 0

  vertices:
	dd  0.6, 0.6,		0f, 0f, 1f
	dd  -0.6, 0.6,		1f, 0f, 1f
	dd  0.6, -0.6,		0f, 0f, 0f
	dd  -0.6, -0.6, 	1f, 0f, 0f

  thousand dd 1000.0

  align 8

  p_vs_src dq vs_src
  p_fs_src dq fs_src

  hdc dq ?
  hrc dq ?

  clock dq ?

  msg MSG
  rc RECT
  pfd PIXELFORMATDESCRIPTOR

  wglCreateContextAttribsARB dq ?

  irpv name, CONTEXT_AWARE_FUNCTION
	name dq ?
  end irpv

  vs_id dd ?
  fs_id dd ?
  program dd ?
  vao dd ?
  vbo dd ?

  uTimeLoc dd ?
  uResolutionLoc dd ?

section '.text' code readable executable

  ; setting fastcall.frame to non-negative value makes fastcall/invoke use it to track maximum necessary
  ; stack space (aligned to a multiple of 16 bytes), and not allocate it automatically
  fastcall.frame = 0

  start:
	sub	rsp,8+MAIN_FRAME
	invoke	GetModuleHandle,0
	mov	[wc.hInstance],rax
	invoke	LoadIcon,0,IDI_APPLICATION
	mov	[wc.hIcon],rax
	invoke	LoadCursor,0,IDC_ARROW
	mov	[wc.hCursor],rax
	invoke	RegisterClass,wc
	invoke	CreateWindowEx,0,_class,_title,WS_VISIBLE+WS_OVERLAPPEDWINDOW+WS_CLIPCHILDREN+WS_CLIPSIBLINGS,48,48,432,432,NULL,NULL,[wc.hInstance],NULL

  msg_loop:
	invoke	GetMessage,addr msg,NULL,0,0
	cmp	eax,1
	jb	end_loop
	jne	msg_loop
	invoke	TranslateMessage,addr msg
	invoke	DispatchMessage,addr msg
	jmp	msg_loop

  end_loop:
	invoke	ExitProcess,[msg.wParam]

  MAIN_FRAME := fastcall.frame

proc WindowProc uses rbx rsi rdi, hwnd,wmsg,wparam,lparam
	frame
	mov	[hwnd],rcx
	cmp	edx,WM_CREATE
	je	wmcreate
	cmp	edx,WM_SIZE
	je	wmsize
	cmp	edx,WM_PAINT
	je	wmpaint
	cmp	edx,WM_KEYDOWN
	je	wmkeydown
	cmp	edx,WM_DESTROY
	je	wmdestroy
  defwndproc:
	invoke	DefWindowProc,rcx,rdx,r8,r9
	jmp	finish
  wmcreate:
	invoke	GetDC,rcx
	mov	[hdc],rax
	lea	rdi,[pfd]
	mov	rcx,sizeof.PIXELFORMATDESCRIPTOR shr 3
	xor	eax,eax
	rep	stosq
	mov	[pfd.nSize],sizeof.PIXELFORMATDESCRIPTOR
	mov	[pfd.nVersion],1
	mov	[pfd.dwFlags],PFD_SUPPORT_OPENGL+PFD_DOUBLEBUFFER+PFD_DRAW_TO_WINDOW
	mov	[pfd.iLayerType],PFD_MAIN_PLANE
	mov	[pfd.iPixelType],PFD_TYPE_RGBA
	mov	[pfd.cColorBits],32
	mov	[pfd.cAlphaBits],8
	mov	[pfd.cDepthBits],24
	mov	[pfd.cStencilBits],8
	invoke	ChoosePixelFormat,[hdc],addr pfd
	invoke	SetPixelFormat,[hdc],eax,addr pfd

	invoke	wglCreateContext,[hdc]
	test	rax,rax
	jz	context_not_created
	mov	[hrc],rax
	invoke	wglMakeCurrent,[hdc],[hrc]

	lea	rbx,[_wglCreateContextAttribsARB]	; name pointer in rbx for error handler
	invoke	wglGetProcAddress,rbx
	test	rax,rax
	jz	function_not_supported
	mov	[wglCreateContextAttribsARB],rax

	invoke	wglDeleteContext,[hrc]
	invoke	wglCreateContextAttribsARB,[hdc],0,attribs
	test	rax,rax
	jz	context_not_created
	mov	[hrc],rax
	invoke	wglMakeCurrent,[hdc],[hrc]

  irpv name, CONTEXT_AWARE_FUNCTION
	lea	rbx,[_#name]				; name pointer in rbx for error handler
	invoke	wglGetProcAddress,rbx
	test	rax,rax
	jz	function_not_supported
	mov	[name],rax
  end irpv

	invoke	glCreateShader,GL_VERTEX_SHADER
	mov	[vs_id],eax
	invoke	glShaderSource,[vs_id],1,addr p_vs_src,0
	invoke	glCompileShader,[vs_id]

	invoke	glCreateShader,GL_FRAGMENT_SHADER
	mov	[fs_id],eax
	invoke	glShaderSource,[fs_id],1,addr p_fs_src,0
	invoke	glCompileShader,[fs_id]

	invoke	glCreateProgram
	mov	[program],eax

	invoke	glAttachShader,[program],[vs_id]
	invoke	glAttachShader,[program],[fs_id]

	invoke	glLinkProgram,[program]

	invoke	glDeleteShader,[vs_id]
	invoke	glDeleteShader,[fs_id]

	invoke	glUseProgram,[program]

	invoke	glGetUniformLocation,[program],uTime
	mov	[uTimeLoc],eax
	invoke	glGetUniformLocation,[program],uResolution
	mov	[uResolutionLoc],eax

	invoke	glGenVertexArrays,1,addr vao
	invoke	glBindVertexArray,[vao]

	invoke	glGenBuffers,1,addr vbo
	invoke	glBindBuffer,GL_ARRAY_BUFFER,[vbo]

	invoke	glBufferData,GL_ARRAY_BUFFER,4*5*4,addr vertices,GL_STATIC_DRAW

	invoke	glVertexAttribPointer,\ ; position
			0,\		; index, layout(location=0)
			2,\		; size(vec2)
			GL_FLOAT,\	; type
			0,\		; normalized
			20,\		; stride(bytes)
			0		; offset(bytes)
	invoke	glEnableVertexAttribArray,0

	invoke	glVertexAttribPointer,\ ; color
			1,\		; index, layout(location=1)
			3,\		; size(vec3)
			GL_FLOAT,\	; type
			0,\		; normalized
			20,\		; stride(bytes)
			8		; offset(bytes), skip vec2
	invoke	glEnableVertexAttribArray,1

  wmsize:
	invoke	GetClientRect,[hwnd],addr rc
	invoke	glViewport,0,0,[rc.right],[rc.bottom]
	cvtsi2ss xmm1,[rc.right]
	cvtsi2ss xmm2,[rc.bottom]
	invoke	glUniform3f,[uResolutionLoc],float xmm1,float xmm2,float 1f
	xor	eax,eax
	jmp	finish
  wmpaint:
	invoke	GetTickCount
	mov	rcx,rax
	sub	rcx,[clock]
	cmp	rcx,10		; wait at least 10ms before drawing again
	jb	finish
	mov	[clock],rax
	and	eax,0FFFFFh
	cvtsi2ss xmm1,rax
	divss	xmm1,[thousand]
	invoke	glUniform1f,[uTimeLoc],float xmm1

	invoke	glClearColor,float dword 0.02,float dword 0.02,float dword 0.04,float dword 1.0
	invoke	glClear,GL_COLOR_BUFFER_BIT

	invoke	glDrawArrays,GL_TRIANGLE_STRIP,0,4

	invoke	SwapBuffers,[hdc]
	xor	eax,eax
	jmp	finish
  wmkeydown:
	cmp	r8d,VK_ESCAPE
	jne	defwndproc
  wmdestroy:
	invoke	wglMakeCurrent,0,0
	invoke	wglDeleteContext,[hrc]
  exit:
	invoke	ReleaseDC,[hwnd],[hdc]
	invoke	PostQuitMessage,0
	xor	eax,eax
  finish:
	ret
  function_not_supported:
	invoke	MessageBox,[hwnd],_function_not_supported,rbx,MB_ICONERROR+MB_OK
	jmp	exit
  context_not_created:
	invoke	MessageBox,[hwnd],_context_not_created,NULL,MB_ICONERROR+MB_OK
	jmp	exit
	endf
endp

section '.rdata' data readable

  _title db 'OpenGL 3.3',0
  _class db 'OPENGL3FASM2',0

  _function_not_supported db 'Function not supported.',0
  _context_not_created db 'Failed to create OpenGL context.',0

  _wglCreateContextAttribsARB db 'wglCreateContextAttribsARB',0

  irpv name, CONTEXT_AWARE_FUNCTION
	_#name db `name,0
  end irpv

  vs_src file 'vertex.glsl'
	 db 0

  fs_src file 'fragment.glsl'
	 db 0

  uTime db 'iTime',0
  uResolution db 'iResolution',0

  align 8

  data import

  library kernel,'KERNEL32.DLL',\
	  user,'USER32.DLL',\
	  gdi,'GDI32.DLL',\
	  opengl,'OPENGL32.DLL'

  import kernel,\
	 GetModuleHandle,'GetModuleHandleA',\
	 GetTickCount,'GetTickCount',\
	 ExitProcess,'ExitProcess'

  import user,\
	 MessageBox,'MessageBoxA',\
	 RegisterClass,'RegisterClassA',\
	 CreateWindowEx,'CreateWindowExA',\
	 DefWindowProc,'DefWindowProcA',\
	 GetMessage,'GetMessageA',\
	 TranslateMessage,'TranslateMessage',\
	 DispatchMessage,'DispatchMessageA',\
	 LoadCursor,'LoadCursorA',\
	 LoadIcon,'LoadIconA',\
	 GetClientRect,'GetClientRect',\
	 GetDC,'GetDC',\
	 ReleaseDC,'ReleaseDC',\
	 PostQuitMessage,'PostQuitMessage'

  import gdi,\
	 ChoosePixelFormat,'ChoosePixelFormat',\
	 SetPixelFormat,'SetPixelFormat',\
	 SwapBuffers,'SwapBuffers'

  import opengl,\
	 glClear,'glClear',\
	 glClearColor,'glClearColor',\
	 glViewport,'glViewport',\
	 glDrawArrays,'glDrawArrays',\
	 wglGetProcAddress,'wglGetProcAddress',\
	 wglCreateContext,'wglCreateContext',\
	 wglDeleteContext,'wglDeleteContext',\
	 wglMakeCurrent,'wglMakeCurrent'

  end data
