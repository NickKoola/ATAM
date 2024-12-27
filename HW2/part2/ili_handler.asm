    .globl my_ili_handler

.text
.align 4, 0x90
my_ili_handler:
  ####### Some smart student's code here #######
    push %rax
    push %rdi
    push %rsi
    push %rdx
    push %rcx
    push %r8
    push %r9
    push %r10
    push %r11
    
    push %rbx
    push %rsp
    push %rbp
    push %r12
    push %r13
    push %r14
    push %r15
    
    mov 128(%rsp), %rbx #rbx = rip = opcode in LE: 0f12 -> 0x120F

    xor %rdi,%rdi
    movzwq (%rbx),%rdi #0F12:  120F -> rdi
    
    cmpb $0x0F, %dil
    je two_byte_opcode_HW2
    ########if one_byte_opcode:########
    
    add $1, %rbx 
    
    push %rbx
    call what_to_do 
    pop %rbx
    jmp continue #skip two_byte_opcode
    
    two_byte_opcode_HW2:
    shr $8, %rdi
    add $2, %rbx
    
    push %rbx
    call what_to_do
    pop %rbx
    
    continue:
    #no need to push rbx because it's callee-saved
    test %rax,%rax #check what_to_do's return value
    jz returned_zero_HW2 #if what_to_do returns 0
    #########if didn't return 0:#########
    mov %rbx, 128(%rsp)
    mov %rax, %rdi
    # pop everything
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbp
    pop %rsp
    pop %rbx
    
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rcx
    pop %rdx
    pop %rsi
    pop %rax #need to keep curr rdi
    pop %rax
    
    iretq
    
    returned_zero_HW2:
    #pop everything in the right order
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbp
    pop %rsp
    pop %rbx
    
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rcx
    pop %rdx
    pop %rsi
    pop %rdi
    pop %rax
    
    jmp *old_ili_handler

   