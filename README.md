# 这是一个基于LLVM(Low Level Virtual Machine)的编译器前端JLC
**请注意,此处的`前端`与常规前端不同,特指`编译器前端`

---

# Works
编译器主要的流程是:
词法分析生成Token(通过状态机) -> 语法分析生成AST(递归下降)
->中间代码生成优化(LLVM IR)->平台无关代码和机器码指令

这里LLVM为我们执行了代码生成部分的内容,
所以基于LLVM的Jvav编译器前端(JLC)的任务是词法分析和语法分析.

---

# Information
Author: Amiriox of **Jvav Development Team**

---

# About LLVM
![](LLVM_logo.svg)
