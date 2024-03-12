
define GLOBSTR GLOBSTR

virtual at GLOBSTR.baseaddr
	GLOBSTR.buffer::
end virtual

postpone
	virtual GLOBSTR.buffer
		load GLOBSTR.data:$-$$ from $$
	end virtual
	if ~ defined GLOBSTR.where
		display 'Global string data has been placed automatically at the end of the executable.',13,10
		display 'To place it elsewhere, use "GLOBSTR.here" macro in the intended location.',13,10
		GLOBSTR.baseaddr db GLOBSTR.data
	end if
end postpone

macro GLOBSTR.here
	GLOBSTR.baseaddr db GLOBSTR.data
	GLOBSTR.where := GLOBSTR.baseaddr
end macro

if defined GLOBSTR.reuse & GLOBSTR.reuse

	virtual at 0
		GLOBSTR.chr_skips:: dd 256 dup ?
	end virtual

	struc GLOBSTR data&
		local buffer,size,a,b,c,p,skip,found
		virtual at 0
			buffer:: db data
			size := $
		end virtual
		repeat 256
			store size:dword at GLOBSTR.chr_skips:(%-1)*4
		end repeat
		repeat size-1
			load a:byte from buffer:%-1
			store size-%:dword at GLOBSTR.chr_skips:a*4
		end repeat
		virtual GLOBSTR.buffer
			found = -1
			p = 0
			while p + size <= $-$$
				c = size
				while c > 0
					load a:byte from $$+p+c-1
					load b:byte from buffer:c-1
					if a <> b
						c = -1
						break
					end if
					c = c - 1
				end while
				if c = 0
					found = p
					break
				else
					load a:byte from $$+p+size-1
					load skip:dword from GLOBSTR.chr_skips:a*4
					p = p + skip
				end if
			end while
			if found >= 0
				label . at $$+found
			else
				. db data
			end if
		end virtual
	end struc

else

	struc GLOBSTR data&
		virtual GLOBSTR.buffer
			. db data
		end virtual
	end struc

end if