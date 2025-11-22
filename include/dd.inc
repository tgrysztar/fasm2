
; x86 data directives with customizable backend.

iterate <word>, word, qword

	calminstruction word? value*
		emit	word, value
	end calminstruction

	macro calminstruction?.word? var*
		call	word, var
	end macro

end iterate

iterate <fword,dword>, dword,word, pword,dword, tword,qword

	calminstruction fword? value*
		emit	fword, value
	end calminstruction

	macro calminstruction?.fword? var*
		local	seg, off
		match	seg:off, var
		jyes	far
	    plain:
		call	fword, var
		jump	ok
	    far:
		call	dword, off
		call	word, seg
	    ok:
	end macro

end iterate

macro calminstruction?.byte? var*
	compute var, var
	check	var eqtype ''
	jyes	string
	emit	byte, var
	jump	ok
    string:
	emit	lengthof var, var
    ok:
end macro

define WCHAR? 2

calminstruction wchar? value*
	compute value, value
	check	value eqtype ''
	jno	plain
	local	n
	compute n, lengthof value
    string:
	check	n
	jno	done
	emit	WCHAR, value and 0FFh
	compute value, value shr 8
	compute n, n - 1
	jump	string
    plain:
	emit	WCHAR, value
    done:
end calminstruction

macro calminstruction?.wchar? var*
	arrange var, =WCHAR var
	assemble var
end macro

iterate <dw,word>, db,byte, dw,word, du,wchar, dd,dword, dp,pword, df,pword, dq,qword, dt,tword

	calminstruction dw? definitions*&
		local	value, n
	    unroll:
		transform definitions
		jyes	unroll
	    start:
		match	value=,definitions, definitions, ()
		jyes	recognize
		match	value, definitions
		arrange definitions,
	    recognize:
		match	n =dup? value, value, ()
		jyes	duplicate
		match	?, value
		jyes	reserve
		word	value
	    next:
		match	, definitions
		jno	start
		take	, definitions
		take	definitions, definitions
		jyes	next
		exit
	    reserve:
		emit	word
		jump	next
	    duplicate:
		match	(value), value
		check	n < 0
		jyes	negative
	    stack:
		check	n
		jno	next
		take	definitions, value
		arrange value, definitions
		compute n, n - 1
		jump	stack
	    negative:
		err	'the number of copies must be positive'
		jump	next
	end calminstruction

	calminstruction (label) dw? &definitions*&
		local	cmd
		arrange cmd, =label label : word
		assemble cmd
		arrange cmd, =dw definitions
		assemble cmd
	end calminstruction

end iterate

macro rf? count*
	rp count
end macro

struc (label) rf? count*
	label rp count
end struc
