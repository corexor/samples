; 286 & 386 LOADALL opcode macros for TASM using IDEAL mode
	IDEAL
MACRO	LOADALL286
	db 0fh,05h
ENDM

MACRO	LOADALL386
	db 0fh,07h
ENDM

; if you wish to use MASM synatx further down
; just uncomment next line
;	MASM 
