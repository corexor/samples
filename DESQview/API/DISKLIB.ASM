INCLUDE DVAPI.INC

datgrp     GROUP   mydata

mydata     SEGMENT PUBLIC 'DATASEG'

p_panlib   DD      panlib
l_panlib   DW      PANLEN,0
panlib     DB      'examples.plb',0 ; ASCIIZ library filename
PANLEN     EQU     $ - panlib

p_panname  DD      panname
l_panname  DW      NAMLEN,0
panname    DB      'POPUP'          ; panel name
NAMLEN     EQU     $ - panname

mywin      DD      0
retwin     DD      0
retkey     DD      0
buffer     DD      0
buflng     DD      0

panhan     DD      0

mydata     ENDS

stack      SEGMENT WORD STACK 'STACK'
           DW      64 DUP(?)
stack      ENDS

code       SEGMENT PUBLIC 'CODE'
           ASSUME  CS:code,DS:mydata
go:
           MOV     AX,datgrp     ; Setup segment register
           MOV     DS,AX
           MOV     BX,0200H      ; Set correct APILEVEL
           @CALL   APILEVEL
           @SEND   HANDLE,ME     ; Remember my handle
           @POP    mywin

           @SEND   NEW,PANEL     ; NEW step (INITIALIZATION)
           @POP    panhan

           @PUSH   p_panlib      ; pointer to string 'EXAMPLES.PLB'
           @PUSH   l_panlib      ; length of string 'EXAMPLES.PLB'
           @SEND   OPEN,panhan   ; OPEN step (INITIALIZATION)

           @PUSH   p_panname     ; pointer to string 'POPUP'
           @PUSH   l_panname     ; length of string 'POPUP'
           @PUSH   mywin         ; apply to my window
           @SEND   APPLY,panhan  ; APPLY step
           @POP    retwin        ; handle of new window
           @POP    retkey        ; handle of new keyboard

           @SEND   READ,retkey   ; Wait for input
           @POP    buflng
           @POP    buffer

           @SEND   FREE,retwin   ; Free new window,keyboard

           ; Use the open panel object repeatedly using
           ; APPLY (and READ, if input required) as necessary

           @SEND   CLOSE,panhan  ; CLOSE step (TERMINATION)
           @SEND   FREE,panhan   ; FREE step  (TERMINATION)

           MOV     AX,4C00H
           INT     21H

code       ENDS
           END     go
