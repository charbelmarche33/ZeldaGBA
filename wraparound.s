@ wraparound
@ r0 is int n
@ r1 is tilemap
.global wraparound 
wraparound:
	mov r3, r0
	b .cmp
.cmp:
	cmp r3, r1
	blt .done
	b .sub
.sub:
	sub r3, r3, r1
	b .cmp
.done:
	mov r0, r3
	mov pc, lr
