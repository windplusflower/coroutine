.globl swap_context
.section .text

get_context:
    # 输入参数context存放在 RDI 寄存器中
    movq %rax, (%rdi)
    movq %rbx, 8(%rdi)
    movq %rcx, 16(%rdi)
    movq %rdx, 24(%rdi)
    movq %rsi, 32(%rdi)
    movq %rdi, 40(%rdi)
    movq %rbp, 48(%rdi)
    # 调用get_context时也有返回地址压栈的过程，所以实际应该保存的sp需要加上8
    # 此时sp指向的是swap_context需要返回的地址
    leaq 8(%rsp), %eax
    movq %eax, 56(%rdi)
    movq %r8, 64(%rdi)
    movq %r9, 72(%rdi)
    movq %r10, 80(%rdi)
    movq %r11, 88(%rdi)
    movq %r12, 96(%rdi)
    movq %r13, 104(%rdi)
    movq %r14, 112(%rdi)
    movq %r15, 120(%rdi)
    ret
    
set_context:
    movq (%rdi), %rax
    movq 8(%rdi), %rbx
    movq 16(%rdi), %rcx
    movq 24(%rdi), %rdx
    movq 32(%rdi), %rsi
    movq 48(%rdi), %rbp
    movq 56(%rdi), %rsp
    movq 64(%rdi), %r8
    movq 72(%rdi), %r9
    movq 80(%rdi), %r10
    movq 88(%rdi), %r11
    movq 96(%rdi), %r12
    movq 104(%rdi), %r13
    movq 112(%rdi), %r14
    movq 120(%rdi), %r15
    # rdi需要放到最后
    movq 40(%rdi), %rdi
    ret

swap_context:
    # rdi保存olducp,rsi保存newucp
    call get_context
    movq %rsi, %rdi
    call set_context