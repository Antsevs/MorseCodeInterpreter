.MODEL FLAT
.DATA

linecounts	BYTE 2,8,3,2,4,12,1,6,2,2,25,65,1,9,5,47,12,78,42,64,7,2,4,8,45,6 
			BYTE 2,5,8,34,6,2,8,2,4,7,9,16,2,4,7,2,9,2,23,45,32,23,1,4,0FFh 
.CODE

main PROC 
			;----- CLEAR CONTENTS OF REGISTERS -----
			xor eax,eax 
			xor ebx,ebx 
			xor ecx,ecx 
			xor edx,edx
			;----- BEGIN MAIN PROCEDURE HERE -----

			;instantiate counter
			mov cl,1
			;assume largest is first at beginning
			mov dl,1
			;move first index linecounts into al
			mov al,linecounts
			;increment to linecounts[counter index]
COMPNEXT:
			mov bl,[linecounts+cx]
			;check if at end of list, if so, DONE, xor getlargestnumber
			cmp bl,0FFh
			je DONE
			call GETLARGESTNUMBER
			;increment counter
			inc cl
			;move to next value in list
			jmp COMPNEXT

DONE:
			nop
			;----- END MAIN PROCEDURE HERE -----
			ret
main ENDP

GETLARGESTNUMBER PROC
			;if al > bl, done, xor move bl to al, move counter to dl, increment dl
			cmp al,bl
			ja DONE
			mov al,bl
			mov dl,cl
			inc dl

DONE:			
			ret

GETLARGESTNUMBER ENDP

END
