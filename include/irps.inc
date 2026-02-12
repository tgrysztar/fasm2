
calminstruction (var) irps? seq&
	local head
    split:
	match head.name seq?, seq, .
	jyes next
	match head seq?, seq
	jno done
    next:
	publish :var, head
	jump split
    done:
end calminstruction

macro irps?! cursor, seq&
	local var
	var irps seq
	outscope irpv cursor, var
end macro

macro end?.irps?!
	end irpv
end macro