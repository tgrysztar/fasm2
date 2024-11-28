
macro imports? items&

	align 4

	iterate item, items

		if % = 1

			repeat %%
				indx %
				match name.=DLL?, item
					name.ImportLookupTableRva	dd RVA ImportLookupTable.name
					name.TimeDateStamp		dd 0
					name.ForwarderChain		dd 0
					name.NameRva			dd RVA ImportLibraryName.name
					name.ImportAddressTableRva	dd RVA ImportAddressTable.name
				else if % = 1
					err 'please start with a name of a DLL'
				end if
			end repeat
					if PE.MAGIC <> 0x20B
									dd 0,0,0,0
					else
									dd 0,0,0
									align 8
					end if

			repeat %%
				indx %
				match name.=DLL?, item
					if PE.MAGIC <> 0x20B
									dd 0
					else
									dq 0
					end if
					ImportLookupTable.name:
				else
					if PE.MAGIC <> 0x20B
									dd RVA ImportLookup.item
					else
									dq RVA ImportLookup.item
					end if
				end match
			end repeat

			repeat %%
				indx %
				match name.=DLL?, item
					if PE.MAGIC <> 0x20B
									dd 0
					else
									dq 0
					end if
					ImportAddressTable.name:
				else
					if PE.MAGIC <> 0x20B
						item			dd RVA ImportLookup.item
					else
						item			dq RVA ImportLookup.item
					end if
				end match
			end repeat
					if PE.MAGIC <> 0x20B
									dd 0
					else
									dq 0
					end if

			indx 1
		end if

		match name.=DLL?, item
					ImportLibraryName.name		db `item,0
		else
									align 2
					ImportLookup.item:
									dw 0
									db `item,0
		end match

	end iterate

end macro
