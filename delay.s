@delay
@r0 is amt
@r1 is the counter
@r2 is 10
.global delay
delay:
	mov r1, #0
	mov r2, #10
	mul r0, r2, r0
.loop:
	add r1, r1, #1
	cmp r1, r0
	beq .done
	b .loop
.done:
	mov pc, lr
