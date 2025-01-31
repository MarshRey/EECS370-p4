	lw	0	1	iter	// iter
	lw	0	2	one	// add to iter
	lw	0	3	four	// outer loop
	lw	0	4	five	// inner loop
	beq	1	0	loop	// always taken control hazard jump
	lw	0	5	one	// never run
	beq	4	3	done	// never run
	add	1	1	2	// never run
	add	3	3	5	// never run
loop	beq	1	3	done	// taken 4 times
	nor	0	0	0
	add	1	1	2	// increment iter
done	halt
one	.fill	1
four	.fill	4
five	.fill	5
iter	.fill	0
