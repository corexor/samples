
define x86

element x86.reg
element x86.r8	: x86.reg + 1
element x86.r16 : x86.reg + 2
element x86.r32 : x86.reg + 4

element al? : x86.r8 + 0
element cl? : x86.r8 + 1
element dl? : x86.r8 + 2
element bl? : x86.r8 + 3
element ah? : x86.r8 + 4
element ch? : x86.r8 + 5
element dh? : x86.r8 + 6
element bh? : x86.r8 + 7

element ax? : x86.r16 + 0
element cx? : x86.r16 + 1
element dx? : x86.r16 + 2
element bx? : x86.r16 + 3
element sp? : x86.r16 + 4
element bp? : x86.r16 + 5
element si? : x86.r16 + 6
element di? : x86.r16 + 7

element eax? : x86.r32 + 0
element ecx? : x86.r32 + 1
element edx? : x86.r32 + 2
element ebx? : x86.r32 + 3
element esp? : x86.r32 + 4
element ebp? : x86.r32 + 5
element esi? : x86.r32 + 6
element edi? : x86.r32 + 7

element x86.sreg

element es? : x86.sreg + 0
element cs? : x86.sreg + 1
element ss? : x86.sreg + 2
element ds? : x86.sreg + 3
element fs? : x86.sreg + 4
element gs? : x86.sreg + 5

element x86.creg

element x86.crx : x86.creg + 0
element x86.drx : x86.creg + 1
element x86.trx : x86.creg + 4

repeat 8, i:0
	element cr#i? : x86.crx + i
	element dr#i? : x86.drx + i
	element tr#i? : x86.trx + i
end repeat

x86.mode = 16

macro use16?
	x86.mode = 16
end macro

macro use32?
	x86.mode = 32
end macro

define @dest
define @src
define @aux

macro x86.parse_operand ns,op
	match =byte? value, op
		ns.size = 1
		x86.parse_operand_value ns,value
	else match =word? value, op
		ns.size = 2
		x86.parse_operand_value ns,value
	else match =dword? value, op
		ns.size = 4
		x86.parse_operand_value ns,value
	else match =pword? value, op
		ns.size = 6
		x86.parse_operand_value ns,value
	else match =qword? value, op
		ns.size = 8
		x86.parse_operand_value ns,value
	else
		ns.size = 0
		x86.parse_operand_value ns,op
	end match
end macro

macro x86.parse_operand_value ns,op
	ns.segment_prefix = 0
	match [addr], op
		ns.type = 'mem'
		match seg:offs, addr
			ns.segment = +seg
			if ns.segment relativeto ns.segment element 1 & ns.segment scale 0 = 0 & ns.segment metadata 1 relativeto x86.sreg
				ns.segment = ns.segment metadata 1 - x86.sreg
				if ns.segment < 4
					ns.segment_prefix = 26h + ns.segment shl 3
				else
					ns.segment_prefix = 64h + ns.segment-4
				end if
			else
				err 'invalid operand'
			end if
			x86.parse_address ns,offs
		else
			x86.parse_address ns,addr
		end match
	else match seg:offs, op
		ns.type = 'far'
		if ns.size & ns.size <> 4 & ns.size <> 6
			err 'operand sizes do not match'
		end if
		ns.segment = +seg
		ns.offset = +offs
	else
		ns.type = 'imm'
		ns.imm = +op
		if defined op
			ns.unresolved = 0
		else
			ns.unresolved = 1
		end if
		ns.displacement_size = 0
		if ns.imm relativeto ns.imm element 1 & ns.imm scale 0 = 0
			if (ns.imm metadata 1) metadata 1 relativeto x86.reg
				ns.type = 'reg'
				ns.mode = x86.mode
				ns.mod = 11b
				ns.rm = ns.imm metadata 1 - (ns.imm metadata 1) element 1
				if ns.size & ns.size <> (ns.imm metadata 1) metadata 1 - x86.reg
					err 'operand sizes do not match'
				else
					ns.size = (ns.imm metadata 1) metadata 1 - x86.reg
				end if
			else if ns.imm metadata 1 relativeto x86.sreg
				ns.type = 'sreg'
				ns.rm = ns.imm metadata 1 - x86.sreg
				if ns.size <> 0 & ns.size <> 2 & ns.size <> 4
					err 'invalid operand size'
				end if
			end if
		end if
	end match
end macro

macro x86.parse_address ns,addr
	ns.mode = 0
	match =word? addr16, addr
		ns.mode = 16
		ns.address = +addr16
		if ns.size = 0
			ns.size = sizeof (addr16)
		end if
	else match =dword? addr32, addr
		ns.mode = 32
		ns.address = +addr32
		if ns.size = 0
			ns.size = sizeof (addr32)
		end if
	else
		ns.address = +addr
		if ns.size = 0
			ns.size = sizeof (addr)
		end if
	end match
	ns.address_registers = 0
	repeat elementsof ns.address
		if ns.address metadata % relativeto x86.r16 | ns.address metadata % relativeto x86.r32
			ns.address_registers = ns.address_registers + ns.address element % * ns.address scale %
		end if
	end repeat
	ns.displacement = ns.address - ns.address_registers
	if ns.mode = 0
		ns.mode = x86.mode
		if ns.mode = 16 & ns.displacement relativeto 0 & ns.displacement >= 10000h & ns.address_registers scale 1 = 0
			ns.mode = 32
		end if
	else
		if (ns.mode = 16 & ns.address_registers metadata 1 relativeto x86.r32) | (ns.mode = 32 & ns.address_registers metadata 1 relativeto x86.r16)
			err 'invalid address'
		end if
	end if
	if ns.address_registers relativeto 0
		ns.mod = 0
		if ns.mode = 16
			ns.rm = 6
			ns.displacement_size = 2
		else
			ns.rm = 5
			ns.displacement_size = 4
		end if
	else
		if ns.address_registers metadata 1 relativeto x86.r32
			ns.mode = 32
			ns.index_only = 0
			if ns.address_registers scale 2 = 0
				ns.scale = ns.address_registers scale 1
				ns.base = ns.address_registers metadata 1 - x86.r32
				if ns.scale = 1
					ns.rm = ns.base
					if ns.rm = 4
						ns.sib = 4 shl 3 + 4
					end if
				else if ns.base <> 4 & ns.scale = 2
					ns.rm = 4
					ns.sib = ns.base shl 3 + ns.base
				else if ns.base <> 4 & (ns.scale = 4 | ns.scale = 8)
					ns.rm = 4
					ns.sib = (bsf ns.scale) shl 6 + ns.base shl 3 + 5
					ns.index_only = 1
				else if ns.base <> 4 & (ns.scale = 3 | ns.scale = 5 | ns.scale = 9)
					ns.rm = 4
					ns.sib = (bsf (ns.scale-1)) shl 6 + ns.base shl 3 + ns.base
				else
					err 'invalid address'
				end if
			else if ns.address_registers scale 3 = 0 & ns.address_registers metadata 2 relativeto x86.r32
				if ns.address_registers scale 1 = 1
					ns.base = ns.address_registers metadata 1 - x86.r32
					ns.index = ns.address_registers metadata 2 - x86.r32
					ns.scale = ns.address_registers scale 2
				else if ns.address_registers scale 2 = 1
					ns.base = ns.address_registers metadata 2 - x86.r32
					ns.index = ns.address_registers metadata 1 - x86.r32
					ns.scale = ns.address_registers scale 1
				else
					err 'invalid address'
				end if
				if ns.index = 4
					if ns.scale = 1
						ns.index = ns.base
						ns.base = 4
					else
						err 'invalid address'
					end if
				end if
				ns.rm = 4
				ns.sib = (bsf ns.scale) shl 6 + ns.index shl 3 + ns.base
			else
				err 'invalid address'
			end if
		else if ns.address_registers metadata 1 relativeto x86.r16
			ns.mode = 16
			if ns.address_registers relativeto bx+si
				ns.rm = 0
			else if ns.address_registers relativeto bx+di
				ns.rm = 1
			else if ns.address_registers relativeto bp+si
				ns.rm = 2
			else if ns.address_registers relativeto bp+di
				ns.rm = 3
			else if ns.address_registers relativeto si
				ns.rm = 4
			else if ns.address_registers relativeto di
				ns.rm = 5
			else if ns.address_registers relativeto bp
				ns.rm = 6
			else if ns.address_registers relativeto bx
				ns.rm = 7
			else
				err 'invalid address'
			end if
		else
			err 'invalid address'
		end if
		if ns.mode = 16
			ns.displacement_size = 2
			ns.mod = 2
			if ns.displacement relativeto 0
				if ns.displacement = 0 & ns.rm <> 6
					ns.displacement_size = 0
					ns.mod = 0
				else if ns.displacement<80h & ns.displacement>=-80h
					ns.displacement_size = 1
					ns.mod = 1
				else if ns.displacement-10000h>=-80h & ns.displacement<10000h
					ns.displacement = ns.displacement-10000h
					ns.displacement_size = 1
					ns.mod = 1
				end if
			end if
		else
			ns.displacement_size = 4
			ns.mod = 2
			if ns.index_only
				ns.mod = 0
			else if ns.displacement relativeto 0
				if ns.displacement = 0 & ns.rm <> 5 & (ns.rm <> 4 | ns.sib and 111b <> 5)
					ns.displacement_size = 0
					ns.mod = 0
				else if ns.displacement<80h & ns.displacement>=-80h
					ns.displacement_size = 1
					ns.mod = 1
				else if ns.displacement-100000000h>=-80h & ns.displacement<100000000h
					ns.displacement = ns.displacement-100000000h
					ns.displacement_size = 1
					ns.mod = 1
				end if
			end if
		end if
	end if
end macro

macro x86.store_instruction opcode*,rm_operand*,reg*,imm_size:0,imm
	if rm_operand.segment_prefix
		if rm_operand.mode = 16 & ( rm_operand.rm = 2 | rm_operand.rm = 3 | ( rm_operand.mod > 0 & rm_operand.rm = 6 ) )
			if rm_operand.segment_prefix <> 36h
				db rm_operand.segment_prefix
			end if
		else if rm_operand.mode = 32 & ( ( rm_operand.mod > 0 & rm_operand.rm = 5 ) | ( rm_operand.rm = 4 & rm_operand.sib and 111b = 4 ) | ( rm_operand.mod > 0 & rm_operand.rm = 4 & rm_operand.sib and 111b = 5 ) )
			if rm_operand.segment_prefix <> 36h
				db rm_operand.segment_prefix
			end if
		else if rm_operand.segment_prefix <> 3Eh
			db rm_operand.segment_prefix
		end if
	end if
	if rm_operand.mod <> 11b & rm_operand.mode <> x86.mode
		db 67h
	end if
	db opcode, rm_operand.mod shl 6 + reg shl 3 + rm_operand.rm
	if rm_operand.mode = 32 & rm_operand.mod <> 11b & rm_operand.rm = 4
		db rm_operand.sib
	end if
	if rm_operand.displacement_size = 1
		db rm_operand.displacement
	else if rm_operand.displacement_size = 2
		dw rm_operand.displacement
	else if rm_operand.displacement_size = 4
		dd rm_operand.displacement
	end if
	if imm_size = 1
		db imm
	else if imm_size = 2
		dw imm
	else if imm_size = 4
		dd imm
	end if
end macro

macro x86.store_operand_prefix size
	if (size = 2 & x86.mode = 32) | (size = 4 & x86.mode = 16)
		db 66h
	else if size <> 0 & size <> 2 & size <> 4
		err 'invalid operand size'
	end if
end macro

macro x86.basic_instruction basecode,dest,src
	x86.parse_operand @dest,dest
	x86.parse_operand @src,src
	local size
	if @dest.size = 0 & @src.size = 0
		err 'operand size not specified'
	else if @dest.size<>0 & @src.size<>0 & @dest.size<>@src.size
		err 'operand sizes do not match'
	else
		size = @dest.size or @src.size
		if size > 4
			err 'invalid operand size'
		end if
	end if
	if @src.type = 'reg' & ( @dest.type = 'reg' | @dest.type = 'mem' )
		if defined size & size > 1
			x86.store_operand_prefix size
			x86.store_instruction basecode+1,@dest,@src.rm
		else
			x86.store_instruction basecode,@dest,@src.rm
		end if
	else if @src.type = 'mem' & @dest.type = 'reg'
		if defined size & size > 1
			x86.store_operand_prefix size
			x86.store_instruction basecode+3,@src,@dest.rm
		else
			x86.store_instruction basecode+2,@src,@dest.rm
		end if
	else if @src.type = 'imm' & ( @dest.type = 'reg' | @dest.type = 'mem' )
		if defined size & size > 1
			x86.store_operand_prefix size
			if @src.imm eqtype 0 & @src.imm relativeto 0 & @src.imm<80h & @src.imm>=-80h
				x86.store_instruction 83h,@dest,basecode shr 3,1,@src.imm
			else if size = 2 & @src.imm eqtype 0 & @src.imm relativeto 0 & @src.imm-10000h>=-80h & @src.imm<10000h
				@src.imm = @src.imm-10000h
				x86.store_instruction 83h,@dest,basecode shr 3,1,@src.imm
			else if size = 4 & @src.imm eqtype 0 & @src.imm relativeto 0 & @src.imm-100000000h>=-80h & @src.imm<100000000h
				@src.imm = @src.imm-100000000h
				x86.store_instruction 83h,@dest,basecode shr 3,1,@src.imm
			else if @dest.type = 'reg' & @dest.rm = 0
				db basecode+5
				if size = 2
					dw @src.imm
				else
					dd @src.imm
				end if
			else
				x86.store_instruction 81h,@dest,basecode shr 3,size,@src.imm
			end if
		else
			if @dest.type = 'reg' & @dest.rm = 0
				db basecode+4
				db @src.imm
			else
				x86.store_instruction 80h,@dest,basecode shr 3,1,@src.imm
			end if
		end if
	else
		err 'invalid combination of operands'
	end if
end macro

macro x86.single_operand_instruction postbyte,dest
	x86.parse_operand @dest,dest
	if @dest.size = 0
		err 'operand size not specified'
	else if @dest.size > 4
		err 'invalid operand size'
	end if
	if @dest.type = 'mem' | @dest.type = 'reg'
		if @dest.size > 1
			x86.store_operand_prefix @dest.size
			x86.store_instruction 0F7h,@dest,postbyte
		else
			x86.store_instruction 0F6h,@dest,postbyte
		end if
	else
		err 'invalid operand'
	end if
end macro

macro add? dest*,src*
	x86.basic_instruction 0,dest,src
end macro

macro or? dest*,src*
	x86.basic_instruction 8,dest,src
end macro

macro adc? dest*,src*
	x86.basic_instruction 10h,dest,src
end macro

macro sbb? dest*,src*
	x86.basic_instruction 18h,dest,src
end macro

macro and? dest*,src*
	x86.basic_instruction 20h,dest,src
end macro

macro sub? dest*,src*
	x86.basic_instruction 28h,dest,src
end macro

macro xor? dest*,src*
	x86.basic_instruction 30h,dest,src
end macro

macro cmp? dest*,src*
	x86.basic_instruction 38h,dest,src
end macro

macro not? src*
	x86.single_operand_instruction 2,src
end macro

macro neg? src*
	x86.single_operand_instruction 3,src
end macro

macro mul? src*
	x86.single_operand_instruction 4,src
end macro

macro div? src*
	x86.single_operand_instruction 6,src
end macro

macro idiv? src*
	x86.single_operand_instruction 7,src
end macro

macro mov? dest*,src*
	x86.parse_operand @dest,dest
	x86.parse_operand @src,src
	local size,ext
	if @dest.size = 0 & @src.size = 0 & @src.type <> 'sreg' & @dest.type <> 'sreg'
		err 'operand size not specified'
	else if @dest.size<>0 & @src.size<>0 & @dest.size<>@src.size
		err 'operand sizes do not match'
	else
		size = @dest.size or @src.size
		if size > 4
			err 'invalid operand size'
		end if
	end if
	if @src.type = 'reg' & @dest.type = 'mem' & @src.rm = 0 & ( (@dest.mode = 16 & @dest.mod = 0 & @dest.rm = 6) | (@dest.mode = 32 & @dest.mod = 0 & @dest.rm = 5) )
		if @dest.segment_prefix & @dest.segment_prefix <> 3Eh
			db @dest.segment_prefix
		end if
		if defined size & size > 1
			x86.store_operand_prefix size
			db 0A3h
		else
			db 0A2h
		end if
		if @dest.mode <> x86.mode
			db 67h
		end if
		if @dest.mode = 16
			dw @dest.displacement
		else
			dd @dest.displacement
		end if
	else if @src.type = 'mem' & @dest.type = 'reg' & @dest.rm = 0 & ( (@src.mode = 16 & @src.mod = 0 & @src.rm = 6) | (@src.mode = 32 & @src.mod = 0 & @src.rm = 5) )
		if @src.segment_prefix & @src.segment_prefix <> 3Eh
			db @src.segment_prefix
		end if
		if defined size & size > 1
			x86.store_operand_prefix size
			db 0A1h
		else
			db 0A0h
		end if
		if @src.mode <> x86.mode
			db 67h
		end if
		if @src.mode = 16
			dw @src.displacement
		else
			dd @src.displacement
		end if
	else if @src.type = 'reg' & ( @dest.type = 'reg' | @dest.type = 'mem' )
		if defined size & size > 1
			x86.store_operand_prefix size
			x86.store_instruction 89h,@dest,@src.rm
		else
			x86.store_instruction 88h,@dest,@src.rm
		end if
	else if @src.type = 'mem' & @dest.type = 'reg'
		if defined size & size > 1
			x86.store_operand_prefix size
			x86.store_instruction 8Bh,@src,@dest.rm
		else
			x86.store_instruction 8Ah,@src,@dest.rm
		end if
	else if @src.type = 'imm' & @dest.type = 'mem'
		if defined size & size > 1
			x86.store_operand_prefix size
			x86.store_instruction 0C7h,@dest,0,size,@src.imm
		else
			x86.store_instruction 0C6h,@dest,0,1,@src.imm
		end if
	else if @src.type = 'imm' & @dest.type = 'reg'
		if @src.imm metadata 1 metadata 1 relativeto x86.creg & @src.imm relativeto @src.imm element 1
			if @dest.size = 4
				ext = 20h + @src.imm metadata 1 metadata 1 - x86.creg
				@src.rm = @src.imm metadata 1 - @src.imm metadata 1 element 1
				x86.store_instruction <0Fh,ext>,@dest,@src.rm
			else
				err 'invalid operand size'
			end if
		else
			if defined size & size > 1
				x86.store_operand_prefix size
				db 0B8h + @dest.rm
				if size = 2
					dw @src.imm
				else
					dd @src.imm
				end if
			else
				db 0B0h + @dest.rm
				db @src.imm
			end if
		end if
	else if @src.type = 'reg' & @dest.type = 'imm'
		if @dest.imm metadata 1 metadata 1 relativeto x86.creg & @dest.imm relativeto @dest.imm element 1
			if @src.size = 4
				ext = 22h + @dest.imm metadata 1 metadata 1 - x86.creg
				@dest.rm = @dest.imm metadata 1 - @dest.imm metadata 1 element 1
				x86.store_instruction <0Fh,ext>,@src,@dest.rm
			else
				err 'invalid operand size'
			end if
		else
			err 'invalid combination of operands'
		end if
	else if @src.type = 'sreg' & @dest.type = 'reg'
		if defined size & size > 1
			x86.store_operand_prefix size
			x86.store_instruction 8Ch,@dest,@src.rm
		else
			err 'invalid operand size'
		end if
	else if @src.type = 'sreg' & @dest.type = 'mem'
		if defined size & size = 2
			x86.store_instruction 8Ch,@dest,@src.rm
		else
			err 'invalid operand size'
		end if
	else if @dest.type = 'sreg' & @dest.rm <> 1 & ( @src.type = 'reg' | @src.type = 'mem' )
		if defined size & size <> 1
			x86.store_instruction 8Eh,@src,@dest.rm
		else
			err 'invalid operand size'
		end if
	else
		err 'invalid combination of operands'
	end if
end macro

macro test? dest*,src*
	x86.parse_operand @dest,dest
	x86.parse_operand @src,src
	local size
	if @dest.size = 0 & @src.size = 0
		err 'operand size not specified'
	else if @dest.size <> 0 & @src.size <> 0 & @dest.size <> @src.size
		err 'operand sizes do not match'
	else
		size = @dest.size or @src.size
		if size > 4
			err 'invalid operand size'
		end if
	end if
	if @src.type = 'reg' & ( @dest.type = 'reg' | @dest.type = 'mem' )
		if defined size & size > 1
			x86.store_operand_prefix size
			x86.store_instruction 85h,@dest,@src.rm
		else
			x86.store_instruction 84h,@dest,@src.rm
		end if
	else if @src.type = 'mem' & @dest.type = 'reg'
		if defined size & size > 1
			x86.store_operand_prefix size
			x86.store_instruction 85h,@src,@dest.rm
		else
			x86.store_instruction 84h,@src,@dest.rm
		end if
	else if @src.type = 'imm' & ( @dest.type = 'reg' | @dest.type = 'mem' )
		if defined size & size > 1
			x86.store_operand_prefix size
			if @dest.type = 'reg' & @dest.rm = 0
				db 0A9h
				if size = 2
					dw @src.imm
				else
					dd @src.imm
				end if
			else
				x86.store_instruction 0F7h,@dest,0,size,@src.imm
			end if
		else
			if @dest.type = 'reg' & @dest.rm = 0
				db 0A8h
				db @src.imm
			else
				x86.store_instruction 0F6h,@dest,0,1,@src.imm
			end if
		end if
	else
		err 'invalid combination of operands'
	end if
end macro

macro xchg? dest*,src*
	x86.parse_operand @dest,dest
	x86.parse_operand @src,src
	local size
	if @dest.size = 0 & @src.size = 0
		err 'operand size not specified'
	else if @dest.size <> 0 & @src.size <> 0 & @dest.size <> @src.size
		err 'operand sizes do not match'
	else
		size = @dest.size or @src.size
		if size > 4
			err 'invalid operand size'
		end if
	end if
	if @src.type = 'reg' & @dest.type = 'reg'
		if defined size & size > 1
			x86.store_operand_prefix size
			if @dest.rm = 0
				db 90h + @src.rm
			else if @src.rm = 0
				db 90h + @dest.rm
			else
				x86.store_instruction 87h,@src,@dest.rm
			end if
		else
			x86.store_instruction 86h,@src,@dest.rm
		end if
	else if @src.type = 'reg' & @dest.type = 'mem'
		if defined size & size > 1
			x86.store_operand_prefix size
			x86.store_instruction 87h,@dest,@src.rm
		else
			x86.store_instruction 86h,@dest,@src.rm
		end if
	else if @src.type = 'mem' & @dest.type = 'reg'
		if defined size & size > 1
			x86.store_operand_prefix size
			x86.store_instruction 87h,@src,@dest.rm
		else
			x86.store_instruction 86h,@src,@dest.rm
		end if
	else
		err 'invalid combination of operands'
	end if
end macro

iterate <instr,postbyte>, inc,0 ,dec,1
	macro instr? dest*
		x86.parse_operand @dest,dest
		if @dest.size = 0
			err 'operand size not specified'
		else if @dest.size > 4
			err 'invalid operand size'
		end if
		if @dest.type = 'mem'
			if @dest.size > 1
				x86.store_operand_prefix @dest.size
				x86.store_instruction 0FFh,@dest,postbyte
			else
				x86.store_instruction 0FEh,@dest,postbyte
			end if
		else if @dest.type = 'reg'
			if @dest.size > 1
				x86.store_operand_prefix @dest.size
				db 40h + @dest.rm + postbyte shl 3
			else
				x86.store_instruction 0FEh,@dest,postbyte
			end if
		else
			err 'invalid operand'
		end if
	end macro
end iterate

macro imul? dest*,src1,src2
	match , src1 src2
		x86.single_operand_instruction 5,dest
	else
		x86.parse_operand @dest,dest
		x86.parse_operand @src,src1
		match , src2
			if @dest.size = 0 & @src.size = 0
				err 'operand size not specified'
			else if @dest.size <> 0 & @src.size <> 0 & @dest.size <> @src.size
				err 'operand sizes do not match'
			else
				size = @dest.size or @src.size
				if size <> 2 & size <> 4
					err 'invalid operand size'
				end if
			end if
			x86.store_operand_prefix size
			if @dest.type = 'reg' & (@src.type = 'reg' | @src.type = 'mem')
				x86.store_instruction <0Fh,0AFh>,@src,@dest.rm
			else if @src.type = 'imm' & @dest.type = 'reg'
				if @src.imm eqtype 0 & @src.imm relativeto 0 & @src.imm<80h & @src.imm>=-80h
					x86.store_instruction 6Bh,@dest,@dest.rm,1,@src.imm
				else if size = 2 & @src.imm eqtype 0 & @src.imm relativeto 0 & @src.imm-10000h>=-80h & @src.imm<10000h
					@src.imm = @src.imm-10000h
					x86.store_instruction 6Bh,@dest,@dest.rm,1,@src.imm
				else if size = 4 & @src.imm eqtype 0 & @src.imm relativeto 0 & @src.imm-100000000h>=-80h & @src.imm<100000000h
					@src.imm = @src.imm-100000000h
					x86.store_instruction 6Bh,@dest,@dest.rm,1,@src.imm
				else
					x86.store_instruction 69h,@dest,@dest.rm,size,@src.imm
				end if
			else
				err 'invalid operand'
			end if
		else
			x86.parse_operand @aux,src2
			if @dest.size = 0 & @src.size = 0 & @aux.size = 0
				err 'operand size not specified'
			else if @dest.size <> 0 & @src.size <> 0 & @dest.size <> @src.size
				err 'operand sizes do not match'
			else
				size = @dest.size or @src.size
				if @aux.size <> 0 & @aux.size <> size
					err 'operand sizes do not match'
				else if size <> 2 & size <> 4
					err 'invalid operand size'
				end if
			end if
			if @aux.type = 'imm' & ( @src.type = 'mem' | @src.type = 'reg' ) & @dest.type = 'reg'
				x86.store_operand_prefix size
				if @aux.imm relativeto 0 & @aux.imm<80h & @aux.imm>=-80h
					x86.store_instruction 6Bh,@src,@dest.rm,1,@aux.imm
				else if size = 2 & @aux.imm relativeto 0 & @aux.imm-10000h>=-80h & @aux.imm<10000h
					@aux.imm = @aux.imm-10000h
					x86.store_instruction 6Bh,@src,@dest.rm,1,@aux.imm
				else if size = 4 & @aux.imm relativeto 0 & @aux.imm-100000000h>=-80h & @aux.imm<100000000h
					@aux.imm = @aux.imm-100000000h
					x86.store_instruction 6Bh,@src,@dest.rm,1,@aux.imm
				else
					x86.store_instruction 69h,@src,@dest.rm,size,@aux.imm
				end if
			else
				err 'invalid operand'
			end if
		end match
	end match
end macro

macro x86.push_instruction operand_size,src
	x86.parse_operand @src,src
	if @src.size = 0
		@src.size = operand_size
	else if (operand_size <> 0 & @src.size <> operand_size) | (operand_size = 0 & @src.size <> 2 & @src.size <> 4)
		err 'invalid operand size'
	end if
	if @src.type = 'mem'
		x86.store_operand_prefix @src.size
		x86.store_instruction 0FFh,@src,110b
	else if @src.type = 'reg'
		x86.store_operand_prefix @src.size
		db 50h + @src.rm
	else if @src.type = 'sreg'
		x86.store_operand_prefix @src.size
		if @src.rm < 4
			db 6 + @src.rm shl 3
		else
			db 0Fh,0A0h + (@src.rm-4) shl 3
		end if
	else if @src.type = 'imm'
		x86.store_operand_prefix @src.size
		if @src.imm eqtype 0 & @src.imm relativeto 0 & @src.imm<80h & @src.imm>=-80h
			db 6Ah
			db @src.imm
		else
			db 68h
			if @src.size = 2 | (@src.size = 0 & x86.mode = 16)
				dw @src.imm
			else
				dd @src.imm
			end if
		end if
	else
		err 'invalid operand'
	end if
end macro

macro x86.pop_instruction operand_size,dest
	x86.parse_operand @dest,dest
	if @dest.size = 0
		@dest.size = operand_size
	else if (operand_size <> 0 & @dest.size <> operand_size) | (operand_size = 0 & @dest.size <> 2 & @dest.size <> 4)
		err 'invalid operand size'
	end if
	if @dest.type = 'mem'
		x86.store_operand_prefix @dest.size
		x86.store_instruction 8Fh,@dest,0
	else if @dest.type = 'reg'
		x86.store_operand_prefix @dest.size
		db 58h + @dest.rm
	else if @dest.type = 'sreg'
		x86.store_operand_prefix @dest.size
		if @dest.rm < 4
			db 7 + @dest.rm shl 3
		else
			db 0Fh,0A1h + (@dest.rm-4) shl 3
		end if
	else
		err 'invalid operand'
	end if
end macro

macro push? src*
	x86.push_instruction 0,src
end macro

macro pushw? src*
	x86.push_instruction 2,src
end macro

macro pushd? src*
	x86.push_instruction 4,src
end macro

macro pop? dest*
	x86.pop_instruction 0,dest
end macro

macro popw? dest*
	x86.pop_instruction 2,dest
end macro

macro popd? dest*
	x86.pop_instruction 4,dest
end macro

iterate instr, push,pushw,pushd,pop,popw,popd
	macro instr? sequence
		local seq,el
		define el
		define seq sequence
		while 1
			match car= cdr, seq
				match pfx, el
					instr pfx car
					redefine el
				else
					match =word?, car
						redefine el car
					else match =dword?, car
						redefine el car
					else
						instr car
					end match
				end match
				redefine seq cdr
			else
				match pfx, el
					instr pfx seq
				else
					instr seq
				end match
				break
			end match
		end while
	end macro
end iterate

macro retn? operand
	match imm, operand
		db 0C2h
		dw imm
	else
		db 0C3h
	end match
end macro

macro retnw? operand
	x86.store_operand_prefix 2
	match imm, operand
		db 0C2h
		dw imm
	else
		db 0C3h
	end match
end macro

macro retnd? operand
	x86.store_operand_prefix 4
	match imm, operand
		db 0C2h
		dw imm
	else
		db 0C3h
	end match
end macro

macro retf? operand
	match imm, operand
		db 0CAh
		dw imm
	else
		db 0CBh
	end match
end macro

macro retfw? operand
	x86.store_operand_prefix 2
	match imm, operand
		db 0CAh
		dw imm
	else
		db 0CBh
	end match
end macro

macro retfd? operand
	x86.store_operand_prefix 4
	match imm, operand
		db 0CAh
		dw imm
	else
		db 0CBh
	end match
end macro

macro ret? operand
	retn operand
end macro

macro retw? operand
	retnw operand
end macro

macro retd? operand
	retnd operand
end macro

macro lea? dest*,src*
	x86.parse_operand @dest,dest
	x86.parse_operand @src,src
	if @dest.size <> 2 & @dest.size <> 4
		err 'invalid operand size'
	end if
	if @src.type = 'mem' & @dest.type = 'reg'
		x86.store_operand_prefix @dest.size
		x86.store_instruction 8Dh,@src,@dest.rm
	else
		err 'invalid combination of operands'
	end if
end macro

iterate instr, les,lds
	repeat 1 basecode:0C4h+%-1
		macro instr? dest*,src*
			x86.parse_operand @dest,dest
			x86.parse_operand @src,src
			if (@dest.size = 2 & (@src.size <> 0 & @src.size <> 4)) | (@dest.size = 4 & (@src.size <> 0 & @src.size <> 6))
				err 'invalid operand size'
			end if
			if @src.type = 'mem' & @dest.type = 'reg'
				x86.store_operand_prefix @dest.size
				x86.store_instruction basecode,@src,@dest.rm
			else
				err 'invalid combination of operands'
			end if
		end macro
	end repeat
end iterate

macro x86.shift_instruction opcode,dest,cnt
	x86.parse_operand @dest,dest
	x86.parse_operand @src,cnt
	if @dest.size = 0
		err 'operand size not specified'
	else if @dest.size > 4
		err 'invalid operand size'
	end if
	if @src.size <> 0 & @src.size <> 1
		err 'invalid operand size'
	end if
	if @src.type = 'reg' & @src.size = 1 & @src.rm = 1 & ( @dest.type = 'reg' | @dest.type = 'mem' )
		if @dest.size > 1
			x86.store_operand_prefix @dest.size
			x86.store_instruction 0D3h,@dest,opcode
		else
			x86.store_instruction 0D2h,@dest,opcode
		end if
	else if @src.type = 'imm' & ( @dest.type = 'reg' | @dest.type = 'mem' )
		if @dest.size >1
			x86.store_operand_prefix @dest.size
			if @src.imm = 1
				x86.store_instruction 0D1h,@dest,opcode
			else
				x86.store_instruction 0C1h,@dest,opcode,1,@src.imm
			end if
		else
			if @src.imm = 1
				x86.store_instruction 0D0h,@dest,opcode
			else
				x86.store_instruction 0C0h,@dest,opcode,1,@src.imm
			end if
		end if
	else
		err 'invalid combination of operands'
	end if
end macro

macro rol? dest*,cnt*
	x86.shift_instruction 0,dest,cnt
end macro

macro ror? dest*,cnt*
	x86.shift_instruction 1,dest,cnt
end macro

macro rcl? dest*,cnt*
	x86.shift_instruction 2,dest,cnt
end macro

macro rcr? dest*,cnt*
	x86.shift_instruction 3,dest,cnt
end macro

macro shl? dest*,cnt*
	x86.shift_instruction 4,dest,cnt
end macro

macro sal? dest*,cnt*
	x86.shift_instruction 4,dest,cnt
end macro

macro shr? dest*,cnt*
	x86.shift_instruction 5,dest,cnt
end macro

macro sar? dest*,cnt*
	x86.shift_instruction 7,dest,cnt
end macro

macro x86.parse_jump_operand ns,op
	match =far? dest, op
		x86.parse_operand ns,dest
		ns.jump_type = 'far'
	else match =near? dest, op
		x86.parse_operand ns,dest
		ns.jump_type = 'near'
	else match =short? dest, op
		x86.parse_operand ns,dest
		ns.jump_type = 'short'
	else
		x86.parse_operand ns,op
		ns.jump_type = ''
	end match
end macro

macro call? dest*
	x86.parse_jump_operand @dest,dest
	if @dest.type = 'far'
		if @dest.jump_type & @dest.jump_type <> 'far'
			err 'invalid operand'
		end if
		if @dest.size = 0
			db 9Ah
			if x86.mode = 16
				dw @dest.offset,@dest.segment
			else
				dd @dest.offset
				dw @dest.segment
			end if
		else if @dest.size = 4 | @dest.size = 6
			x86.store_operand_prefix (@dest.size-2)
			db 9Ah
			if @dest.size = 4
				dw @dest.offset,@dest.segment
			else
				dd @dest.offset
				dw @dest.segment
			end if
		else
			err 'invalid operand size'
		end if
	else if @dest.type = 'mem' | @dest.type = 'reg'
		if @dest.size = 6
			if @dest.jump_type & @dest.jump_type <> 'far'
				err 'invalid operand'
			end if
			x86.store_operand_prefix 4
			x86.store_instruction 0FFh,@dest,11b
		else if @dest.size = 4
			if @dest.jump_type | @dest.type = 'reg'
				if @dest.jump_type = 'far'
					x86.store_operand_prefix 2
					x86.store_instruction 0FFh,@dest,11b
				else
					x86.store_operand_prefix 4
					x86.store_instruction 0FFh,@dest,10b
				end if
			else
				if x86.mode = 16
					x86.store_operand_prefix 2
					x86.store_instruction 0FFh,@dest,11b
				else
					x86.store_operand_prefix 4
					x86.store_instruction 0FFh,@dest,10b
				end if
			end if
		else if @dest.size = 2
			if @dest.jump_type & @dest.jump_type <> 'near'
				err 'invalid operand'
			end if
			x86.store_operand_prefix 2
			x86.store_instruction 0FFh,@dest,10b
		else if @dest.size = 0
			if @dest.jump_type = 'far'
				x86.store_instruction 0FFh,@dest,11b
			else if @dest.jump_type = 'near'
				x86.store_instruction 0FFh,@dest,10b
			else
				err 'operand size not specified'
			end if
		else
			err 'invalid operand'
		end if
	else if @dest.type = 'imm'
		if @dest.jump_type & @dest.jump_type <> 'near'
			err 'invalid operand'
		end if
		if @dest.size = 2 | (@dest.size = 0 & x86.mode = 16)
			if @dest.imm relativeto 0 & (@dest.imm < 0 | @dest.imm >= 10000h)
				err 'value out of range'
			end if
			x86.store_operand_prefix 2
			db 0E8h
			dw @dest.imm-($+2)
		else
			if @dest.imm relativeto 0 & (@dest.imm < 0 | @dest.imm >= 100000000h)
				err 'value out of range'
			end if
			x86.store_operand_prefix 4
			db 0E8h
			dd @dest.imm-($+4)
		end if
	else
		err 'invalid operand'
	end if
end macro

macro jmp? dest*
	x86.parse_jump_operand @dest,dest
	if @dest.type = 'far'
		if @dest.jump_type & @dest.jump_type <> 'far'
			err 'invalid operand'
		end if
		if @dest.size = 0
			db 0EAh
			if x86.mode = 16
				dw @dest.offset,@dest.segment
			else
				dd @dest.offset
				dw @dest.segment
			end if
		else if @dest.size = 4 | @dest.size = 6
			x86.store_operand_prefix (@dest.size-2)
			db 0EAh
			if @dest.size = 4
				dw @dest.offset,@dest.segment
			else
				dd @dest.offset
				dw @dest.segment
			end if
		else
			err 'invalid operand size'
		end if
	else if @dest.type = 'mem' | @dest.type = 'reg'
		if @dest.size = 6
			if @dest.jump_type & @dest.jump_type <> 'far'
				err 'invalid operand'
			end if
			x86.store_operand_prefix 4
			x86.store_instruction 0FFh,@dest,101b
		else if @dest.size = 4
			if @dest.jump_type | @dest.type = 'reg'
				if @dest.jump_type = 'far'
					x86.store_operand_prefix 2
					x86.store_instruction 0FFh,@dest,101b
				else
					x86.store_operand_prefix 4
					x86.store_instruction 0FFh,@dest,100b
				end if
			else
				if x86.mode = 16
					x86.store_operand_prefix 2
					x86.store_instruction 0FFh,@dest,101b
				else
					x86.store_operand_prefix 4
					x86.store_instruction 0FFh,@dest,100b
				end if
			end if
		else if @dest.size = 2
			if @dest.jump_type & @dest.jump_type <> 'near'
				err 'invalid operand'
			end if
			x86.store_operand_prefix 2
			x86.store_instruction 0FFh,@dest,100b
		else if @dest.size = 0
			if @dest.jump_type = 'far'
				x86.store_instruction 0FFh,@dest,101b
			else if @dest.jump_type = 'near'
				x86.store_instruction 0FFh,@dest,100b
			else
				err 'operand size not specified'
			end if
		else
			err 'invalid operand size'
		end if
	else if @dest.type = 'imm'
		if @dest.size = 2 | (@dest.size = 0 & x86.mode = 16)
			if @dest.imm relativeto 0 & (@dest.imm < 0 | @dest.imm >= 10000h)
				err 'value out of range'
			end if
			if @dest.jump_type = 'near'
				x86.store_operand_prefix 2
				db 0E9h
				dw @dest.imm-($+2)
			else if @dest.jump_type = 'short'
				db 0EBh
				if @dest.imm-($+1)<80h & @dest.imm-($+1)>=-80h
					db @dest.imm-($+1)
				else
					err 'relative jump out of range'
					db ?
				end if
			else if ~ @dest.jump_type
				if ( @dest.imm relativeto 0 & @dest.imm-($+2)<80h & @dest.imm-($+2)>=-80h ) | @dest.unresolved
					db 0EBh
					db @dest.imm-($+1)
				else
					x86.store_operand_prefix 2
					db 0E9h
					dw @dest.imm-($+2)
				end if
			else
				err 'invalid operand'
			end if
		else
			if @dest.imm relativeto 0 & (@dest.imm < 0 | @dest.imm >= 100000000h)
				err 'value out of range'
			end if
			if @dest.jump_type = 'near'
				x86.store_operand_prefix 4
				db 0E9h
				dw @dest.imm-($+4)
			else if @dest.jump_type = 'short'
				db 0EBh
				if @dest.imm-($+1)<80h & @dest.imm-($+1)>=-80h
					db @dest.imm-($+1)
				else
					err 'relative jump out of range'
					db ?
				end if
			else if ~ @dest.jump_type
				if ( @dest.imm relativeto 0 & @dest.imm-($+2)<80h & @dest.imm-($+2)>=-80h ) | @dest.unresolved
					db 0EBh
					db @dest.imm-($+1)
				else
					x86.store_operand_prefix 4
					db 0E9h
					dd @dest.imm-($+4)
				end if
			else
				err 'invalid operand'
			end if
		end if
	else
		err 'invalid operand'
	end if
end macro

macro x86.conditional_jump opcode,dest
	x86.parse_jump_operand @dest,dest
	if @dest.type = 'imm' & @dest.jump_type <> 'far'
		if @dest.imm relativeto 0 & ( @dest.imm < 0 | (x86.mode = 16 & @dest.imm >= 10000h) | (x86.mode = 32 & @dest.imm >= 100000000h) )
			err 'value out of range'
		end if
		if ( @dest.jump_type <> 'near' & @dest.imm relativeto 0 & @dest.imm-($+2)<80h & @dest.imm-($+2)>=-80h ) | @dest.unresolved
			db opcode
			db @dest.imm-($+1)
		else if @dest.jump_type = 'short'
			err 'relative jump out of range'
		else
			if x86.mode = 16
				db 0Fh,10h+opcode
				dw @dest.imm-($+2)
			else
				db 0Fh,10h+opcode
				dd @dest.imm-($+4)
			end if
		end if
	else
		err 'invalid operand'
	end if
end macro

macro jo? dest*
	x86.conditional_jump 70h,dest
end macro

macro jno? dest*
	x86.conditional_jump 71h,dest
end macro

macro jc? dest*
	x86.conditional_jump 72h,dest
end macro

macro jb? dest*
	x86.conditional_jump 72h,dest
end macro

macro jnae? dest*
	x86.conditional_jump 72h,dest
end macro

macro jnc? dest*
	x86.conditional_jump 73h,dest
end macro

macro jnb? dest*
	x86.conditional_jump 73h,dest
end macro

macro jae? dest*
	x86.conditional_jump 73h,dest
end macro

macro jz? dest*
	x86.conditional_jump 74h,dest
end macro

macro je? dest*
	x86.conditional_jump 74h,dest
end macro

macro jnz? dest*
	x86.conditional_jump 75h,dest
end macro

macro jne? dest*
	x86.conditional_jump 75h,dest
end macro

macro jna? dest*
	x86.conditional_jump 76h,dest
end macro

macro jbe? dest*
	x86.conditional_jump 76h,dest
end macro

macro ja? dest*
	x86.conditional_jump 77h,dest
end macro

macro jnbe? dest*
	x86.conditional_jump 77h,dest
end macro

macro js? dest*
	x86.conditional_jump 78h,dest
end macro

macro jns? dest*
	x86.conditional_jump 79h,dest
end macro

macro jp? dest*
	x86.conditional_jump 7Ah,dest
end macro

macro jpe? dest*
	x86.conditional_jump 7Ah,dest
end macro

macro jnp? dest*
	x86.conditional_jump 7Bh,dest
end macro

macro jpo? dest*
	x86.conditional_jump 7Bh,dest
end macro

macro jl? dest*
	x86.conditional_jump 7Ch,dest
end macro

macro jnge? dest*
	x86.conditional_jump 7Ch,dest
end macro

macro jnl? dest*
	x86.conditional_jump 7Dh,dest
end macro

macro jge? dest*
	x86.conditional_jump 7Dh,dest
end macro

macro jng? dest*
	x86.conditional_jump 7Eh,dest
end macro

macro jle? dest*
	x86.conditional_jump 7Eh,dest
end macro

macro jg? dest*
	x86.conditional_jump 7Fh,dest
end macro

macro jnle? dest*
	x86.conditional_jump 7Fh,dest
end macro

macro x86.short_jump opcode,dest
	x86.parse_jump_operand @dest,dest
	if @dest.type = 'imm' & ( @dest.jump_type = 'short' | ~ @dest.jump_type )
		if @dest.imm relativeto 0 & ( @dest.imm < 0 | (x86.mode = 16 & @dest.imm >= 10000h) | (x86.mode = 32 & @dest.imm >= 100000000h) )
			err 'value out of range'
		end if
		db opcode
		if @dest.imm-($+1)<80h & @dest.imm-($+1)>=-80h
			db @dest.imm-($+1)
		else
			err 'relative jump out of range'
			db ?
		end if
	else
		err 'invalid operand'
	end if
end macro

macro loopnz? dest*
	x86.short_jump 0E0h,dest
end macro

macro loopne? dest*
	x86.short_jump 0E0h,dest
end macro

macro loopz? dest*
	x86.short_jump 0E1h,dest
end macro

macro loope? dest*
	x86.short_jump 0E1h,dest
end macro

macro loop? dest*
	x86.short_jump 0E2h,dest
end macro

macro loopnzw? dest*
	x86.store_operand_prefix 2
	x86.short_jump 0E0h,dest
end macro

macro loopnew? dest*
	x86.store_operand_prefix 2
	x86.short_jump 0E0h,dest
end macro

macro loopzw? dest*
	x86.store_operand_prefix 2
	x86.short_jump 0E1h,dest
end macro

macro loopew? dest*
	x86.store_operand_prefix 2
	x86.short_jump 0E1h,dest
end macro

macro loopw? dest*
	x86.store_operand_prefix 2
	x86.short_jump 0E2h,dest
end macro

macro loopnzd? dest*
	x86.store_operand_prefix 4
	x86.short_jump 0E0h,dest
end macro

macro loopned? dest*
	x86.store_operand_prefix 4
	x86.short_jump 0E0h,dest
end macro

macro loopzd? dest*
	x86.store_operand_prefix 4
	x86.short_jump 0E1h,dest
end macro

macro looped? dest*
	x86.store_operand_prefix 4
	x86.short_jump 0E1h,dest
end macro

macro loopd? dest*
	x86.store_operand_prefix 4
	x86.short_jump 0E2h,dest
end macro

macro jcxz? dest*
	x86.store_operand_prefix 2
	x86.short_jump 0E3h,dest
end macro

macro jecxz? dest*
	x86.store_operand_prefix 4
	x86.short_jump 0E3h,dest
end macro

macro daa?
	db 27h
end macro

macro das?
	db 2Fh
end macro

macro aaa?
	db 37h
end macro

macro aas?
	db 3Fh
end macro

macro nop?
	db 90h
end macro

macro cbw?
	x86.store_operand_prefix 2
	db 98h
end macro

macro cwde?
	x86.store_operand_prefix 4
	db 98h
end macro

macro cwd?
	x86.store_operand_prefix 2
	db 99h
end macro

macro cdq?
	x86.store_operand_prefix 4
	db 99h
end macro

macro int3?
	db 0CCh
end macro

macro int? number*
	db 0CDh,number
end macro

macro into?
	db 0CEh
end macro

macro iret?
	db 0CFh
end macro

macro iretw?
	x86.store_operand_prefix 2
	db 0CFh
end macro

macro iretd?
	x86.store_operand_prefix 4
	db 0CFh
end macro

macro aam? number:10
	db 0D4h,number
end macro

macro aad? number:10
	db 0D5h,number
end macro

macro salc?
	db 0D6h
end macro

macro lock? instr&
	db 0F0h
	instr
end macro

macro repnz? instr&
	db 0F2h
	instr
end macro

macro repne? instr&
	db 0F2h
	instr
end macro

macro rep? instr&
	db 0F3h
	instr
end macro

macro repz? instr&
	db 0F3h
	instr
end macro

macro repe? instr&
	db 0F3h
	instr
end macro

macro hlt?
	db 0F4h
end macro

macro cmc?
	db 0F5h
end macro

macro clc?
	db 0F8h
end macro

macro stc?
	db 0F9h
end macro

macro cli?
	db 0FAh
end macro

macro sti?
	db 0FBh
end macro

macro cld?
	db 0FCh
end macro

macro std?
	db 0FDh
end macro

macro pushf?
	db 9Ch
end macro

macro popf?
	db 9Dh
end macro

macro pushfw?
	x86.store_operand_prefix 2
	db 9Ch
end macro

macro popfw?
	x86.store_operand_prefix 2
	db 9Dh
end macro

macro pushfd?
	x86.store_operand_prefix 4
	db 9Ch
end macro

macro popfd?
	x86.store_operand_prefix 4
	db 9Dh
end macro

macro pusha?
	db 60h
end macro

macro popa?
	db 61h
end macro

macro pushaw?
	x86.store_operand_prefix 2
	db 60h
end macro

macro popaw?
	x86.store_operand_prefix 2
	db 61h
end macro

macro pushad?
	x86.store_operand_prefix 4
	db 60h
end macro

macro popad?
	x86.store_operand_prefix 4
	db 61h
end macro

macro sahf?
	db 9Eh
end macro

macro lahf?
	db 9Fh
end macro

macro movsb?
	db 0A4h
end macro

macro movsw?
	x86.store_operand_prefix 2
	db 0A5h
end macro

macro movsd?
	x86.store_operand_prefix 4
	db 0A5h
end macro

macro cmpsb?
	db 0A6h
end macro

macro cmpsw?
	x86.store_operand_prefix 2
	db 0A7h
end macro

macro cmpsd?
	x86.store_operand_prefix 4
	db 0A7h
end macro

macro stosb?
	db 0AAh
end macro

macro stosw?
	x86.store_operand_prefix 2
	db 0ABh
end macro

macro stosd?
	x86.store_operand_prefix 4
	db 0ABh
end macro

macro lodsb?
	db 0ACh
end macro

macro lodsw?
	x86.store_operand_prefix 2
	db 0ADh
end macro

macro lodsd?
	x86.store_operand_prefix 4
	db 0ADh
end macro

macro scasb?
	db 0AEh
end macro

macro scasw?
	x86.store_operand_prefix 2
	db 0AFh
end macro

macro scasd?
	x86.store_operand_prefix 4
	db 0AFh
end macro

macro insb?
	db 06Ch
end macro

macro insw?
	x86.store_operand_prefix 2
	db 06Dh
end macro

macro insd?
	x86.store_operand_prefix 4
	db 06Dh
end macro

macro outsb?
	db 06Eh
end macro

macro outsw?
	x86.store_operand_prefix 2
	db 06Fh
end macro

macro outsd?
	x86.store_operand_prefix 4
	db 06Fh
end macro

macro xlatb?
	db 0D7h
end macro

macro movs? dest*,src*
	x86.parse_operand @dest,dest
	x86.parse_operand @src,src
	local size
	if @dest.size = 0 & @src.size = 0
		err 'operand size not specified'
	else if @dest.size <> 0 & @src.size <> 0 & @dest.size <> @src.size
		err 'operand sizes do not match'
	else
		size = @dest.size or @src.size
		if size > 4
			err 'invalid operand size'
		end if
	end if
	if @src.type = 'mem' & @src.mod = 0 & @dest.type = 'mem' & @dest.mod = 0 & ( (@src.mode = 16 & @src.rm = 4 & @dest.mode = 16 & @dest.rm = 5) | (@src.mode = 32 & @src.rm = 6 & @dest.mode = 32 & @dest.rm = 7) ) & ( @dest.segment_prefix = 0 | @dest.segment_prefix = 26h )
		if @src.segment_prefix & @src.segment_prefix <> 3Eh
			db @src.segment_prefix
		end if
		if defined size & size > 1
			x86.store_operand_prefix size
			if @dest.mode <> x86.mode
				db 67h
			end if
			db 0A5h
		else
			if @dest.mode <> x86.mode
				db 67h
			end if
			db 0A4h
		end if
	else
		err 'invalid operand'
	end if
end macro

macro cmps? src*,dest*
	x86.parse_operand @dest,dest
	x86.parse_operand @src,src
	local size
	if @dest.size = 0 & @src.size = 0
		err 'operand size not specified'
	else if @dest.size <> 0 & @src.size <> 0 & @dest.size <> @src.size
		err 'operand sizes do not match'
	else
		size = @dest.size or @src.size
		if size > 4
			err 'invalid operand size'
		end if
	end if
	if @src.type = 'mem' & @src.mod = 0 & @dest.type = 'mem' & @dest.mod = 0 & ( (@src.mode = 16 & @src.rm = 4 & @dest.mode = 16 & @dest.rm = 5) | (@src.mode = 32 & @src.rm = 6 & @dest.mode = 32 & @dest.rm = 7) ) & ( @dest.segment_prefix = 0 | @dest.segment_prefix = 26h )
		if @src.segment_prefix & @src.segment_prefix <> 3Eh
			db @src.segment_prefix
		end if
		if defined size & size > 1
			x86.store_operand_prefix size
			if @dest.mode <> x86.mode
				db 67h
			end if
			db 0A7h
		else
			if @dest.mode <> x86.mode
				db 67h
			end if
			db 0A6h
		end if
	else
		err 'invalid operand'
	end if
end macro

macro stos? dest*
	x86.parse_operand @dest,dest
	if @dest.size = 0
		err 'operand size not specified'
	else if @dest.size > 4
		err 'invalid operand size'
	end if
	if @dest.type = 'mem' & @dest.mod = 0 & ( (@dest.mode = 16 & @dest.rm = 5) | (@dest.mode = 32 & @dest.rm = 7) ) & ( @dest.segment_prefix = 0 | @dest.segment_prefix = 26h )
		if @dest.size > 1
			x86.store_operand_prefix @dest.size
			if @dest.mode <> x86.mode
				db 67h
			end if
			db 0ABh
		else
			if @dest.mode <> x86.mode
				db 67h
			end if
			db 0AAh
		end if
	else
		err 'invalid operand'
	end if
end macro

macro lods? src*
	x86.parse_operand @src,src
	if @src.size = 0
		err 'operand size not specified'
	else if @src.size > 4
		err 'invalid operand size'
	end if
	if @src.type = 'mem' & @src.mod = 0 & ( (@src.mode = 16 & @src.rm = 4) | (@src.mode = 32 & @src.rm = 6) )
		if @src.segment_prefix & @src.segment_prefix <> 3Eh
			db @src.segment_prefix
		end if
		if @src.size > 1
			x86.store_operand_prefix @src.size
			if @src.mode <> x86.mode
				db 67h
			end if
			db 0ADh
		else
			if @src.mode <> x86.mode
				db 67h
			end if
			db 0ACh
		end if
	else
		err 'invalid operand'
	end if
end macro

macro scas? dest*
	x86.parse_operand @dest,dest
	if @dest.size = 0
		err 'operand size not specified'
	else if @dest.size > 4
		err 'invalid operand size'
	end if
	if @dest.type = 'mem' & @dest.mod = 0 & ( (@dest.mode = 16 & @dest.rm = 5) | (@dest.mode = 32 & @dest.rm = 7) ) & ( @dest.segment_prefix = 0 | @dest.segment_prefix = 26h )
		if @dest.size > 1
			x86.store_operand_prefix @dest.size
			if @dest.mode <> x86.mode
				db 67h
			end if
			db 0AFh
		else
			if @dest.mode <> x86.mode
				db 67h
			end if
			db 0AEh
		end if
	else
		err 'invalid operand'
	end if
end macro

macro ins? dest*,src*
	x86.parse_operand @dest,dest
	x86.parse_operand @src,src
	if @dest.size = 0
		err 'operand size not specified'
	else if @dest.size > 4
		err 'invalid operand size'
	end if
	if @src.type = 'reg' & @src.size = 2 & @src.rm = 2 & @dest.type = 'mem' & @dest.mod = 0 & ( (@dest.mode = 16 & @dest.rm = 5) | (@dest.mode = 32 & @dest.rm = 7) ) & ( @dest.segment_prefix = 0 | @dest.segment_prefix = 26h )
		if @dest.size > 1
			x86.store_operand_prefix @dest.size
			if @dest.mode <> x86.mode
				db 67h
			end if
			db 06Dh
		else
			if @dest.mode <> x86.mode
				db 67h
			end if
			db 06Ch
		end if
	else
		err 'invalid operand'
	end if
end macro

macro outs? dest*,src*
	x86.parse_operand @dest,dest
	x86.parse_operand @src,src
	if @src.size = 0
		err 'operand size not specified'
	else if @src.size > 4
		err 'invalid operand size'
	end if
	if @dest.type = 'reg' & @dest.size = 2 & @dest.rm = 2 & @src.type = 'mem' & @src.mod = 0 & ( (@src.mode = 16 & @src.rm = 4) | (@src.mode = 32 & @src.rm = 6) )
		if @src.segment_prefix & @src.segment_prefix <> 3Eh
			db @src.segment_prefix
		end if
		if @src.size > 1
			x86.store_operand_prefix @src.size
			if @src.mode <> x86.mode
				db 67h
			end if
			db 06Fh
		else
			if @src.mode <> x86.mode
				db 67h
			end if
			db 06Eh
		end if
	else
		err 'invalid operand'
	end if
end macro

macro xlat? src*
	x86.parse_operand @src,src
	if @src.size > 1
		err 'invalid operand size'
	end if
	if @src.type = 'mem' & @src.mod = 0 & ( (@src.mode = 16 & @src.rm = 7) | (@src.mode = 32 & @src.rm = 3) )
		if @src.segment_prefix & @src.segment_prefix <> 3Eh
			db @src.segment_prefix
		end if
		if @src.mode <> x86.mode
			db 67h
		end if
		db 0D7h
	else
		err 'invalid operand'
	end if
end macro

macro in? dest*,src*
	x86.parse_operand @dest,dest
	x86.parse_operand @src,src
	if @dest.size = 0
		err 'operand size not specified'
	else if @dest.size > 4
		err 'invalid operand size'
	end if
	if @src.type = 'reg' & @src.size = 2 & @src.rm = 2 & @dest.type = 'reg' & @dest.rm = 0
		if @dest.size > 1
			x86.store_operand_prefix @dest.size
			db 0EDh
		else
			db 0ECh
		end if
	else if @src.type = 'imm' & @dest.type = 'reg' & @dest.rm = 0
		if @dest.size > 1
			x86.store_operand_prefix @dest.size
			db 0E5h,@src.imm
		else
			db 0E4h,@src.imm
		end if
	else
		err 'invalid combination of operands'
	end if
end macro

macro out? dest*,src*
	x86.parse_operand @dest,dest
	x86.parse_operand @src,src
	if @src.size = 0
		err 'operand size not specified'
	else if @src.size > 4
		err 'invalid operand size'
	end if
	if @dest.type = 'reg' & @dest.size = 2 & @dest.rm = 2 & @src.type = 'reg' & @src.rm = 0
		if @src.size > 1
			x86.store_operand_prefix @src.size
			db 0EFh
		else
			db 0EEh
		end if
	else if @dest.type = 'imm' & @src.type = 'reg' & @src.rm = 0
		if @src.size > 1
			x86.store_operand_prefix @src.size
			db 0E7h,@dest.imm
		else
			db 0E6h,@dest.imm
		end if
	else
		err 'invalid combination of operands'
	end if
end macro

macro bound? dest*,src*
	x86.parse_operand @dest,dest
	x86.parse_operand @src,src
	local size
	if @dest.size <> 0 & @src.size <> 0 & @dest.size <> @src.size
		err 'operand sizes do not match'
	else
		size = @dest.size or @src.size
		if size <> 2 & size <> 4
			err 'invalid operand size'
		end if
	end if
	if @src.type = 'mem' & @dest.type = 'reg'
		x86.store_operand_prefix size
		x86.store_instruction 62h,@src,@dest.rm
	else
		err 'invalid combination of operands'
	end if
end macro

macro enter? alloc*,nesting*
	x86.parse_operand @src,alloc
	x86.parse_operand @aux,nesting
	if (@src.size <> 0 & @src.size <> 2) | (@aux.size <> 0 & @aux.size <> 1)
		err 'invalid operand size'
	end if
	if @src.type = 'imm' & @aux.type = 'imm'
		db 0C8h
		dw @src.imm
		db @aux.imm
	else
		err 'invalid operand'
	end if
end macro

macro leave?
	db 0C9h
end macro

macro arpl? dest*,src*
	x86.parse_operand @dest,dest
	x86.parse_operand @src,src
	local size
	if @dest.size <> 0 & @src.size <> 0 & @dest.size <> @src.size
		err 'operand sizes do not match'
	else
		size = @dest.size or @src.size
		if size <> 2 & size <> 4
			err 'invalid operand size'
		end if
	end if
	if @src.type = 'reg' & (@dest.type = 'mem' | @dest.type = 'reg')
		x86.store_operand_prefix size
		x86.store_instruction <63h>,@dest,@src.rm
	else
		err 'invalid combination of operands'
	end if
end macro

macro x86.setcc_instruction ext,dest
	x86.parse_operand @dest,dest
	if @dest.size > 1
		err 'invalid operand size'
	end if
	x86.store_instruction <0Fh,ext>,@dest,0
end macro

macro seto? dest*
	x86.setcc_instruction 90h,dest
end macro

macro setno? dest*
	x86.setcc_instruction 91h,dest
end macro

macro setc? dest*
	x86.setcc_instruction 92h,dest
end macro

macro setb? dest*
	x86.setcc_instruction 92h,dest
end macro

macro setnae? dest*
	x86.setcc_instruction 92h,dest
end macro

macro setnc? dest*
	x86.setcc_instruction 93h,dest
end macro

macro setnb? dest*
	x86.setcc_instruction 93h,dest
end macro

macro setae? dest*
	x86.setcc_instruction 93h,dest
end macro

macro setz? dest*
	x86.setcc_instruction 94h,dest
end macro

macro sete? dest*
	x86.setcc_instruction 94h,dest
end macro

macro setnz? dest*
	x86.setcc_instruction 95h,dest
end macro

macro setne? dest*
	x86.setcc_instruction 95h,dest
end macro

macro setna? dest*
	x86.setcc_instruction 96h,dest
end macro

macro setbe? dest*
	x86.setcc_instruction 96h,dest
end macro

macro seta? dest*
	x86.setcc_instruction 97h,dest
end macro

macro setnbe? dest*
	x86.setcc_instruction 97h,dest
end macro

macro sets? dest*
	x86.setcc_instruction 98h,dest
end macro

macro setns? dest*
	x86.setcc_instruction 99h,dest
end macro

macro setp? dest*
	x86.setcc_instruction 9Ah,dest
end macro

macro setpe? dest*
	x86.setcc_instruction 9Ah,dest
end macro

macro setnp? dest*
	x86.setcc_instruction 9Bh,dest
end macro

macro setpo? dest*
	x86.setcc_instruction 9Bh,dest
end macro

macro setl? dest*
	x86.setcc_instruction 9Ch,dest
end macro

macro setnge? dest*
	x86.setcc_instruction 9Ch,dest
end macro

macro setnl? dest*
	x86.setcc_instruction 9Dh,dest
end macro

macro setge? dest*
	x86.setcc_instruction 9Dh,dest
end macro

macro setng? dest*
	x86.setcc_instruction 9Eh,dest
end macro

macro setle? dest*
	x86.setcc_instruction 9Eh,dest
end macro

macro setg? dest*
	x86.setcc_instruction 9Fh,dest
end macro

macro setnle? dest*
	x86.setcc_instruction 9Fh,dest
end macro

iterate <instr,ext,postbyte>, lldt,0,2, ltr,0,3, verr,0,4, verw,0,5, lmsw,1,6
	macro instr? dest*
		x86.parse_operand @dest,dest
		if @dest.size <> 0 & @dest.size <> 2
			err 'invalid operand size'
		end if
		if @dest.type = 'reg' | @dest.type = 'mem'
			x86.store_instruction <0Fh,ext>,@dest,postbyte
		else
			err 'invalid operand'
		end if
	end macro
end iterate

iterate <instr,ext,postbyte>, sldt,0,0, str,0,1, smsw,1,4
	macro instr? dest*
		x86.parse_operand @dest,dest
		if @dest.type = 'reg'
			if @dest.size <> 0 & @dest.size <> 2 & @dest.size <> 4
				err 'invalid operand size'
			end if
			x86.store_operand_prefix @dest.size
			x86.store_instruction <0Fh,ext>,@dest,postbyte
		else if @dest.type = 'mem'
			if @dest.size <> 0 & @dest.size <> 2
				err 'invalid operand size'
			end if
			x86.store_instruction <0Fh,ext>,@dest,postbyte
		else
			err 'invalid operand'
		end if
	end macro
end iterate

iterate <instr,postbyte>, lgdt,2, lidt,3, sgdt,0, sidt,1
	macro instr? dest*
		x86.parse_operand @dest,dest
		if @dest.size <> 0 & @dest.size <> 5 & @dest.size <> 6
			err 'invalid operand size'
		end if
		if @dest.type = 'mem'
			if @dest.size = 6
				x86.store_operand_prefix 4
			else if @dest.size = 5
				x86.store_operand_prefix 2
			end if
			x86.store_instruction <0Fh,1>,@dest,postbyte
		else
			err 'invalid operand'
		end if
	end macro
end iterate

iterate <instr,ext>, lar,2, lsl,3
	macro instr? dest*,src*
		x86.parse_operand @dest,dest
		x86.parse_operand @src,src
		local size
		if @dest.size <> 0 & @src.size <> 0 & @dest.size <> @src.size
			err 'operand sizes do not match'
		else
			size = @dest.size or @src.size
			if size <> 2 & size <> 4
				err 'invalid operand size'
			end if
		end if
		if @dest.type = 'reg' & (@src.type = 'mem' | @src.type = 'reg')
			x86.store_operand_prefix size
			x86.store_instruction <0Fh,ext>,@src,@dest.rm
		else
			err 'invalid combination of operands'
		end if
	end macro
end iterate

macro clts?
	db 0Fh,06h
end macro

macro loadall?
	db 0Fh,07h
end macro

iterate <instr,ext>, movzx,0B6h, movsx,0BEh
	macro instr? dest*,src*
		x86.parse_operand @dest,dest
		x86.parse_operand @src,src
		if @dest.size <= @src.size
			err 'operand sizes do not match'
		end if
		if @dest.type = 'reg' & (@src.type = 'mem' | @src.type = 'reg')
			if @src.size = 2
				x86.store_operand_prefix @dest.size
				x86.store_instruction <0Fh,ext+1>,@src,@dest.rm
			else if @src.size = 1
				x86.store_operand_prefix @dest.size
				x86.store_instruction <0Fh,ext>,@src,@dest.rm
			else
				err 'invalid operand size'
			end if
		else
			err 'invalid combination of operands'
		end if
	end macro
end iterate

iterate <instr,postbyte>, bt,4, bts,5, btr,6, btc,7
	macro instr? dest*,src*
		x86.parse_operand @dest,dest
		x86.parse_operand @src,src
		local size
		if @src.type = 'reg' & (@dest.type = 'mem' | @dest.type = 'reg')
			if @dest.size <> 0 & @src.size <> 0 & @dest.size <> @src.size
				err 'operand sizes do not match'
			else
				size = @dest.size or @src.size
				if size <> 2 & size <> 4
					err 'invalid operand size'
				end if
			end if
			x86.store_operand_prefix size
			x86.store_instruction <0Fh,0A3h+(postbyte-4) shl 3>,@dest,@src.rm
		else if @src.type = 'imm' & (@dest.type = 'mem' | @dest.type = 'reg')
			if @src.size <> 0 & @src.size <> 1
				err 'invalid operand size'
			end if
			if @dest.size
				if @dest.size <> 2 & @dest.size <> 4
					err 'invalid operand size'
				end if
				x86.store_operand_prefix @dest.size
			end if
			x86.store_instruction <0Fh,0BAh>,@dest,postbyte,1,@src.imm
		else
			err 'invalid combination of operands'
		end if
	end macro
end iterate

iterate <instr,ext>, bsf,0BCh, bsr,0BDh
	macro instr? dest*,src*
		x86.parse_operand @dest,dest
		x86.parse_operand @src,src
		local size
		if @dest.size <> 0 & @src.size <> 0 & @dest.size <> @src.size
			err 'operand sizes do not match'
		else
			size = @dest.size or @src.size
			if size <> 2 & size <> 4
				err 'invalid operand size'
			end if
		end if
		if @dest.type = 'reg' & (@src.type = 'mem' | @src.type = 'reg')
			x86.store_operand_prefix @dest.size
			x86.store_instruction <0Fh,ext>,@src,@dest.rm
		else
			err 'invalid combination of operands'
		end if
	end macro
end iterate

iterate <instr,ext>, shld,0A4h, shrd,0ACh
	macro instr? dest*,src*,cnt*
		x86.parse_operand @dest,dest
		x86.parse_operand @src,src
		x86.parse_operand @aux,cnt
		local size
		if @dest.size <> 0 & @src.size <> 0 & @dest.size <> @src.size
			err 'operand sizes do not match'
		else
			size = @dest.size or @src.size
			if size <> 2 & size <> 4
				err 'invalid operand size'
			end if
		end if
		if @aux.size <> 0 & @aux.size <> 1
			err 'invalid operand size'
		end if
		if @aux.type = 'reg' & @aux.size = 1 & @aux.rm = 1 & @src.type = 'reg' & ( @dest.type = 'reg' | @dest.type = 'mem' )
			x86.store_operand_prefix size
			x86.store_instruction <0Fh,ext+1>,@dest,@src.rm
		else if @aux.type = 'imm' & @src.type = 'reg' & ( @dest.type = 'reg' | @dest.type = 'mem' )
			x86.store_operand_prefix size
			x86.store_instruction <0Fh,ext>,@dest,@src.rm,1,@aux.imm
		else
			err 'invalid combination of operands'
		end if
	end macro
end iterate
