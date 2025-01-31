	lw	0	1	zero	//iterator
	lw	0	2	one	// 1 to add
	lw	0	3	four	// 4 to check
	lw	0	4	zero	// sum
loop	beq	1	3	end	// if iterator == 4
	add	1	2	1	// iterator++
	add	4	3	4	// sum += 4
	beq	0	0	loop	// loop
end	halt
zero	.fill	0
one	.fill	1
four	.fill	4
