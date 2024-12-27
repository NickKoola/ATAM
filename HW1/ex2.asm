.section .text
_start:

movq $source, %rax
movq $destination, %rbx
movq $0, %rcx #counter
xor %rdx, %rdx
xor %r8, %r8 #helper
movl (num), %edx


cmp $0, %edx
jg positive_num_HW1
je end_HW1
jmp negative_num_HW1

positive_num_HW1:

    cmpq %rax, %rbx
    jg dest_is_bigger_HW1
    jl src_is_bigger_HW1
    
    
src_is_bigger_HW1: #regular copy
    movb (%rax, %rcx), %r8b
    movb %r8b, (%rbx, %rcx)
    incq %rcx
    decl %edx
    cmp $0, %edx
    je end_HW1
    jmp src_is_bigger_HW1
    
dest_is_bigger_HW1: # reverse copy
    dec %edx # num--
    movb (%rax, %rdx), %r8b
    movb %r8b, (%rbx,%rdx)
    cmp $0, %edx
    je end_HW1
    jmp dest_is_bigger_HW1


negative_num_HW1:
movl %edx, (%rbx)

end_HW1:
