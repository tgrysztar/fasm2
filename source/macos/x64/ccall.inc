
; Adapted and tested by Jacob Young (jacobly.alt@gmail.com)

iterate reg, rdi, rsi, rdx, rcx, r8, r9
	arguments.%? equ reg
end iterate

macro ccall? proc*,args&
	local size
	mov	rbp,rsp
	and	rsp,0FFFFFFFFFFFFFFF0h
	match any, args
		iterate arg, args
			if sizeof (arg)
				lea	arguments.%,[arg]
			else if ~ arg eq arguments.%
				mov	arguments.%,arg
			end if
		end iterate
	end match
	call	proc
	mov	rsp,rbp
end macro
