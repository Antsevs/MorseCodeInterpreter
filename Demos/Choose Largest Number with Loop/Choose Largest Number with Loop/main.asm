.MODEL FLAT
.DATA

linecounts	BYTE 50,2,8,3,2,4,12,1,6,2,2,25,65,1,9,5,47,12,78,42,64,7,2,4,8,45,6
			BYTE 2,5,8,34,6,2,8,2,4,7,9,16,2,4,7,2,9,2,23,45,32,23,1,4 
.CODE

main PROC 
			;----- CLEAR CONTENTS OF REGISTERS -----
			xor eax,eax 
			xor ebx,ebx 
			xor ecx,ecx 
			xor edx,edx
			;----- BEGIN MAIN PROCEDURE HERE -----
			;counter register
			mov cl,linecounts
			;index of highest value (eventually)
			mov dl,cl
			;at beginning start at 1
			mov al,[linecounts+cx]

COMPNEXT:
			;bring current linecount index to bl
			mov bl,[linecounts+cx]

			call GETLARGESTNUMBER

			loop COMPNEXT

DONE:
			nop
			;----- END MAIN PROCEDURE HERE -----
			ret
main ENDP

GETLARGESTNUMBER PROC
			cmp al,bl
			;compare al, bl, if al > bl then done
			ja DONE
			;ax should store larger number, so bx to ax
			mov al,bl
			;dl has index of largest number
			mov dl,cl

DONE:		ret


GETLARGESTNUMBER ENDP

END
