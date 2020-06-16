# 这是一个基于LLVM(Low Level Virtual Machine)的编译器前端JLC
**请注意,此处的`前端`与常规前端不同,特指`编译器前端`

---

操作系统要求: Ubuntu 16或以上, Mac OS X 10.12(Sierra)或以上

编译器要求: clang 6.0或以上

首先执行使用apt安装clang和llvm
```bash
$ sudo apt install clang
$ sudo apt install llvm
```
查看clang是否安装成功
```bash
$ clang -v
$ clang++ -v
```
找到llvm目录(STLExtras.h在llvm/ADT/下):
```bash
$ locate STLExtras.h
```
复制该文件夹到自己的目录(非必须)
```bash
$ sudo cp -Rf /usr/include/llvm-6.0/ ~/dev
```
把`jvavc.cpp`放入llvm-6.0下
```
$ sudo cp ~/localFile/jvavc.cpp ~/dev/llvm-6.0/
```
执行编译,warning别管:
```
$ clang++ -g jvavc.cpp `llvm-config --cxxflags --ldflags --system-lib --libs core` -o jvavc.out
```
运行:
```
./jvavc.out
```


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
无法显示LLVM_logo.svg,属正常现象
> LLVM是一个自由软件项目，它是一种编译器基础设施，以C++写成，包含一系列模块化的编译器组件和工具链，用来开发编译器前端和后端。它是为了任意一种编程语言而写成的程序，利用虚拟技术创造出编译时期、链接时期、运行时期以及“闲置时期”的最优化。它最早以C/C++为实现对象，而目前它已支持包括ActionScript、Ada、D语言、Fortran、GLSL、Haskell、Java字节码、Objective-C、Swift、Python、Ruby、Crystal、Rust、Scala[1]以及C#[2]等语言。
![](LLVM_logo.svg)
