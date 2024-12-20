.global swap_context
.section .text

swap_context:
    # 输入参数context存放在 RDI 寄存器中
    # rax,rcx,rdx,rsi,rdi,r8,r9,r10,r11是调用者保存寄存器，调用swap_context函数时，系统应自动在栈上保存了它们。
    # 而切换到目标协程则是通过swap_context的返回地址返回实现的，因此也能正常还原这些寄存器
    # 故不需要手动保存
    movq %rbx, 0(%rdi)
    movq %rbp, 8(%rdi)
    # 此时栈顶是"swap_context下一条指令地址"，正是返回此协程后需要运行的位置，所以不用修改
    # 之后set_context返回时，会直接返回到"swap_context下一条指令地址"
    movq %rsp, 16(%rdi)
    movq %r12, 24(%rdi)
    movq %r13, 32(%rdi)
    movq %r14, 40(%rdi)
    movq %r15, 48(%rdi)
    
    movq 0(%rsi), %rbx
    movq 8(%rsi), %rbp
    movq 16(%rsi), %rsp
    movq 24(%rsi), %r12
    movq 32(%rsi), %r13
    movq 40(%rsi), %r14
    movq 48(%rsi), %r15
    ret