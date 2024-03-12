
define fix? fix?

fix... = 0

calminstruction (name) fix? &value&
	arrange name, fix.name
	publish name, value
	check fix...
	jyes done
	compute fix..., 1
	arrange name, fix.=enable
	assemble name
    done:
end calminstruction

macro fix?.enable? chain
	chain
	macro include?! file*, head
		include file, fix.enable head
		purge ?, include?
	end macro
	calminstruction ?! &line&
		local any
		match any =fix? any?, line
		jyes skip
		transform line, fix
	    skip:
		assemble line
	end calminstruction
end macro
