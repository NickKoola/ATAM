.global _start

.section .text
_start:

mov (num), %rax
xor %bl, %bl

loop_HW1:
cmp $0, %rax
je end_HW1
sal $1, %rax
jae loop_HW1
inc %bl
jmp loop_HW1

end_HW1:

movb %bl, (Bool)
