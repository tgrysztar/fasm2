
% = 0

calminstruction times?: statement
	local	number, instruction, limit
	match	number= statement, statement, ()
	jno	incomplete_statement
	take	limit, number
	compute limit, limit
	take	instruction, statement
	compute number, 1
	take	%, number
	check	limit < 0
	jyes	negative_number
    loop:
	check	% > limit
	jyes	done
	assemble instruction
	compute %, % + 1
	jump	loop
    incomplete_statement:
	err	'incomplete statement'
	exit
    negative_number:
	err	'the number of times must be positive'
    done:
	take	, %
	take	, limit
	take	, instruction
end calminstruction
