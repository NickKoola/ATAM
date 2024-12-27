.global _start

.section .text
_start:

movq (new_node), %rbx # rbx = new_node.data
movq (root), %rax # rax = current_node
movq $root, %rdx

loop_HW1:
cmp  $0, %rax # check if current_node is null

jz set_node_HW1

movq (%rax), %rcx # rcx = current_node.data

cmp %rcx, %rbx # current_node.data > new_node.data
je end_HW1 # if current_node.data == new_node.data do nothing

jg right_HW1
movq %rax, %rdx
add %8, %rdx
movq 8(%rax), %rax # set current_node to left son

jmp loop_HW1

right_HW1:
movq %rax, %rdx
add $16, %rdx
movq 16(%rax), %rax # set current_node to right son

jmp loop_HW1

set_node_HW1:
movq $new_node, (%rdx) # parent = new_node
jmp end_HW1

end_HW1:

