.global _start

.section .text
_start:

movq $array1, %rdx # array1 pointer
movq $array2, %rdi # array2 pointer
movq $mergedArray, %rsi # mergedArray pointer
movl $0, %r8d

loop_HW1:
movl (%rdx), %eax # array1 value
movl (%rdi), %ebx # array2 value

cmp %eax, %ebx
jg add2_HW1

add1_HW1:
cmp $0, %eax
je should_end_HW1

cmp %r8d, %eax
je skip1_HW1

movl %eax, (%rsi)
movl %eax, %r8d
add $0x4, %rsi # mergedArray pointer ++

skip1_HW1:
add $0x4, %rdx # array1 pointer ++
jmp loop_HW1

add2_HW1:
cmp $0, %ebx
je should_end_HW1

cmp %r8d, %ebx
je skip2_HW1

movl %ebx, (%rsi)
movl %ebx, %r8d
add $0x4, %rsi # mergedArray pointer ++

skip2_HW1:
add $0x4, %rdi # array2 pointer ++


should_end_HW1:
cmp $0, %ebx
jne loop_HW1
cmp $0, %eax
jne loop_HW1
end_HW1:
movl $0, (%rsi)
