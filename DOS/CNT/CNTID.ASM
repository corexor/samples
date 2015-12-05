; cntid.asm: simple utility to detect CnT Super386 CPU based on http://www.os2museum.com/wp/more-on-the-ct-super386/
; CPU code detection idea comes from great book:
;  The Undocumented PC: A Programmer's Guide to I/O, CPUs, and Fixed Memory Areas 
;  by Frank van Gilluwe 
; compile with FASM 1.7.4x or newer using: fasm cnt.asm
; if you want to skip NOP after SCALL for debugging purposes set DEBUG to 0
; if you want to skip check for MS Window set CHKWIN to 0
; if you want to skip check for protected/v86 mode set CHKPM to 0

format MZ		;use DOS MZ exe format
entry cseg:start	;entrypoint at cseg:start
stack 100h		;define stack
segment cseg use16	;using 16-bit segment for code and data inside normal DOS EXE

;conditional assembly constants

DEBUG	= 0		;if 1 generates additional NOP as a placeholder for int3 after SCALL to aid debugging
CHKWIN	= 1		;if 1 checks if Windows are running before calling SCALL
CHKPM	= 1		;if 1 checks if CPU is in protected/v86 mode before calling SCALL

; macros definitions

macro	scall { 	;SCALL: Super386 only instruction
	db 0fh, 18h, 0c0h
}

; entry point
start:
	mov ax,cs	;setup data segment to point at code segment
	mov ds,ax	;since we have only single segment EXE

	mov ah,9	;display logo
	mov dx,hellomsg
	int 21h

if CHKWIN		;check if Windows are running since SCALL may break Win environment
	mov ax,1600h
	int 2fh 	;Windows check
	cmp al,0
	je @f
	cmp al,80h
	je @f
	mov ah,9
	mov dx,winmsg
	int 21h
	jmp exit
end if

@@:			;check for 186 CPU
	mov bx,sp	;checks what is pushed on stack with a push sp
	push sp
	pop ax
	cmp ax,bx	;on >186 current SP is pushed and later stack pointer gets updated
	je @f		;286 found so move further checking for pm mode or 386
	mov dx,badcpumsg
	jmp exit


@@:			;check for 286 CPU based on top 4 bits of EFLAG register
			;on 286 those bits are always set to 0 while 386+ allows those bits to be changed
	cli		;disable interrupts so nothing will mess up our stack
	pushf
	pushf
	pop ax
	or ax,0f000h
	push ax
	popf
	pushf
	pop ax
	sti		;enable interrupts
	test ax,0f000h
	jnz @f		;we found 386+ so we can proceed with SCALL test
	mov dx,badcpumsg
	jmp exit
@@:

			;check for protected/v86 mode since we can execute smsw instruction safely
			;calling SCALL from pm/v86 mode could be possibly dangerous???
if CHKPM
	smsw ax 	;there is no way to check for v86 other than to check for PM=1 bit in CR0 register
	test ax,1	;if PM=1 we assume PM (286) or v86 (386) mode 
	jz @f		;nope - we are still in real mode, so move further
	mov dx,badcpumodemsg
	jmp exit
end if

@@:				;386 found, now test for C&T Super386 in particular
	mov [newoffs],noscall
	call hook06		;hook int 6 in case when SCALL isn't supported, this is used for detecting Super386 CPU
	xor eax,eax		;function 0
	scall
if DEBUG
	nop			;we get here if SCALL is supported, nop left as placeholder for int3 for debugging purposes
end if
	call unhook06		;remove our int 6 handler as we don't need it anymore
	mov dx,cntfoundmsg
	jmp exit
noscall:			;int 6 handler has been triggered - SCALL isn't supported by this CPU
	call unhook06		;remove our int 6 handler as we don't need it anymore
	mov dx,notcntmsg
	jmp exit
exit:				;expect msg in DX
	mov ah,9
	int 21h
	mov ah,9
	mov dx,crlf
	int 21h
	mov ah,4ch
	int 21h

;DATA Section

crlf		db 13,10,'$'
hellomsg	db ' ',254,' C&T Super386 detection utility ',254,13,10
		db ' corexor.wordpress.com  github.com/corexor',13,10
		db ' For more info about C&T visit:',13,10
		db ' http://www.os2museum.com/wp/more-on-the-ct-super386/',13,10
		db 13,10,'$'
if CHKWIN
winmsg		db 'Windows is running. Restart to DOS.$'
end if
badcpumsg	db '386+ CPU not detected.$'
if CHKPM
badcpumodemsg	db 'CPU is in protected/v86 mode. Restart to real mode.$'
end if
notcntmsg	db 'No C&T Super386 CPU found.$'
cntfoundmsg	db 'C&T Super386 CPU found. $'

oldint06	dd ?		;old int 6 handler address
newoffs 	dw ?		;new return offset from int 6 handler


; PROCEDURES section

int06handler:			;our int 6 handler (INVALID OPCODE) changes return address afert exception gets triggered
	push bp
	push bx
	mov bx,[cs:newoffs]	;get new return address from our newoffs variable
	mov bp,sp		;let bp point at stack pointer
	mov [ss:bp+4],bx	;change return address on stack to value from newoffs variable
	pop bx			;restore modified registers
	pop bp
	iret			;return from int 6 handler


hook06: 			;install our own int 6 handler temporary using DOS functions for safety
	push es
	mov ax,3506h
	int 21h 		;read int 6 seg:offs into es:bx and store it at oldint06
	mov word [oldint06+2],es
	mov word [oldint06],bx
	pop es
	mov ax,2506h
	mov dx,int06handler
	int 21h 		;change int 6 seg:offs to cs:int06handler
	ret


unhook06:			;uninstall our own temporary int 6 handler using DOS functions for safety
	push ds
	mov ax,2506h
	mov dx,word [oldint06]
	mov ds,word [oldint06+2]
	int 21h 		;restore int 6 original seg:offs from oldint06
	pop ds
	ret
