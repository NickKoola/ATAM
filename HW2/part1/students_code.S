.global	keyMixing, byteSubstitution, shiftRows, cipher

.section	.data
sbox:	.byte	99,124,119,123,-14,107,111,-59,48,1,103,43,-2,-41,-85,118
		.byte	-54,-126,-55,125,-6,89,71,-16,-83,-44,-94,-81,-100,-92,114,-64
		.byte	-73,-3,-109,38,54,63,-9,-52,52,-91,-27,-15,113,-40,49,21
		.byte	4,-57,35,-61,24,-106,5,-102,7,18,-128,-30,-21,39,-78,117
		.byte	9,-125,44,26,27,110,90,-96,82,59,-42,-77,41,-29,47,-124
		.byte	83,-47,0,-19,32,-4,-79,91,106,-53,-66,57,74,76,88,-49
		.byte	-48,-17,-86,-5,67,77,51,-123,69,-7,2,127,80,60,-97,-88
		.byte	81,-93,64,-113,-110,-99,56,-11,-68,-74,-38,33,16,-1,-13,-46
		.byte	-51,12,19,-20,95,-105,68,23,-60,-89,126,61,100,93,25,115
		.byte	96,-127,79,-36,34,42,-112,-120,70,-18,-72,20,-34,94,11,-37
		.byte	-32,50,58,10,73,6,36,92,-62,-45,-84,98,-111,-107,-28,121
		.byte	-25,-56,55,109,-115,-43,78,-87,108,86,-12,-22,101,122,-82,8
		.byte	-70,120,37,46,28,-90,-76,-58,-24,-35,116,31,75,-67,-117,-118
		.byte	112,62,-75,102,72,3,-10,14,97,53,87,-71,-122,-63,29,-98
		.byte	-31,-8,-104,17,105,-39,-114,-108,-101,30,-121,-23,-50,85,40,-33
		.byte	-116,-95,-119,13,-65,-26,66,104,65,-103,45,15,-80,84,-69,22

.section	.text
keyMixing:
	# rdi -> input address
	# rsi -> key address

	mov (%rsi), %rax 	# load first 64 bit of key into rax
	xor %rax, (%rdi) 	# xor first 64 of input
	mov 8(%rsi), %rax 	# load last 64 bit of key into rax
	xor %rax, 8(%rdi) 	# xor last 64 bit of input

	ret


byteSubstitution:
	# rdi -> input address
	push %rbx

	xor %rcx, %rcx # i = 0
	xor %rax, %rax
FOR_LOOP_HW2:
	movq %rcx, %rbx # rbx = i
	add %rdi, %rbx # rbx = input[i]
	movb (%rbx), %al # al = *input[i]

	lea sbox(%rip), %rbx
	add %rax, %rbx
	movb (%rbx), %al # al = *map[*input[i]]
	
	movb %al, (%rdi, %rcx) # *input[i] = *map[*input[i]]
	inc %rcx # i++
	cmp $16, %rcx # if i != 16
	jne FOR_LOOP_HW2

	pop %rbx
	ret




shiftRows:
	push %rbx
	
    xor %rdx, %rdx				# i =0
FOR_LOOP_2_HW2:
    xor %rcx, %rcx
    mov %rdx, %rcx 				# rcx = i #amount of bits that'll be rotated
    sal $3, %rcx 				# rcx = 8*i
    movq (%rdi, %rdx, 4), %rbx 	# ebx = *(arrayAddress + i*8) ## ERR
    xor %r10, %r10
    mov %rdx, %r10 				# r10 is temp
    sal $2, %r10 				# temp = temp*8
    add %rdi, %r10 				# temp = temp + arrayAddress
    
    ror %cl, %ebx 				# rotate 8*i bits to the left
    mov %ebx, (%r10) 			# arrayAdressOffset = ebx
    inc %rdx 					# i++
    cmp $4, %rdx 				# if i != 4
    jne FOR_LOOP_2_HW2			# if i != 4

	pop %rbx
    ret


cipher:
	/*
	 * rdi = input
	 * rsi = key
	 * rdx = len
	**/
xor %r10, %r10 			# i = 0
block_loop_ex2:	
	cmp %rdx, %r10			# if i == len -> goto end
	je end_cipher_ex2		# if i == len -> goto end
	
	push %rax
	push %rcx
	push %rdi
	push %rsi
	push %rdx
	push %r10
	push %r11
	call keyMixing  		# keyMixing (%rdi = block address, %rsi = key)
	pop %r11
	pop %r10
	pop %rdx
	pop %rsi
	pop %rdi
	pop %rcx
	pop %rax

	xor %r11, %r11 			# j = 0
nr_loop_cipher_ex2:
	
	push %rax
	push %rcx
	push %rdi
	push %rsi
	push %rdx
	push %r10
	push %r11
	call byteSubstitution 	# byteSubstitution(%rdi = block address)
	pop %r11
	pop %r10
	pop %rdx
	pop %rsi
	pop %rdi
	pop %rcx
	pop %rax

	push %rax
	push %rcx
	push %rdi
	push %rsi
	push %rdx
	push %r10
	push %r11
	call shiftRows		 	# shiftRows(%rdi = block address)
	pop %r11
	pop %r10
	pop %rdx
	pop %rsi
	pop %rdi
	pop %rcx
	pop %rax

	push %rax
	push %rcx
	push %rdi
	push %rsi
	push %rdx
	push %r10
	push %r11
	call mixColumns 		# mixColumns(%rdi = block address)
	pop %r11
	pop %r10
	pop %rdx
	pop %rsi
	pop %rdi
	pop %rcx
	pop %rax

	push %rax
	push %rcx
	push %rdi
	push %rsi
	push %rdx
	push %r10
	push %r11
	call keyMixing  		# keyMixing (%rdi = block address, %rsi = key)
	pop %r11
	pop %r10
	pop %rdx
	pop %rsi
	pop %rdi
	pop %rcx
	pop %rax
	
	inc %r11 				# j++
	cmp $9, %r11			# if j != 9 -> goto loop
	jne nr_loop_cipher_ex2		# if j != 9 -> goto loop
	
	push %rax
	push %rcx
	push %rdi
	push %rsi
	push %rdx
	push %r10
	push %r11
	call byteSubstitution 	# byteSubstitution(%rdi = block address)
	pop %r11
	pop %r10
	pop %rdx
	pop %rsi
	pop %rdi
	pop %rcx
	pop %rax

	push %rax
	push %rcx
	push %rdi
	push %rsi
	push %rdx
	push %r10
	push %r11
	call shiftRows			# shiftRows(%rdi = block address)
	pop %r11
	pop %r10
	pop %rdx
	pop %rsi
	pop %rdi
	pop %rcx
	pop %rax

	push %rax
	push %rcx
	push %rdi
	push %rsi
	push %rdx
	push %r10
	push %r11
	call keyMixing  		# keyMixing (%rdi = block address, %rsi = key)
	pop %r11
	pop %r10
	pop %rdx
	pop %rsi
	pop %rdi
	pop %rcx
	pop %rax
	
	add $16, %rdi 			# block address += 16
	inc %r10 				# i++
	jmp block_loop_ex2		# if i == len -> goto end
end_cipher_ex2:
	ret
