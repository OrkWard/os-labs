### `head.S`

内容如图。这部分应该不需要额外说明。

![image-20240106155148937](.\assets\image-20240106155148937.png)

### Makefile

为 `.o`  添加源文件依赖即可。

### `sbi.c`、`defs.h`

按照格式编写内联汇编即可。

![image-20240106160521894](.\assets\image-20240106160521894.png)

![image-20240106160750258](.\assets\image-20240106160750258.png)

### 实现 `_traps`

保存 32 个寄存器。这里 sp 作为栈底指针，自然伸缩，不需要额外保存。中间函数调用 `trap_handler` 由编译器保证 callee save 的寄存器不会改变，因此这里其实没有必要保存 s 系列寄存器，只是写的时候从 x0 保存到 x31 都由 vim macro 直接生成，额外存一次也无妨。

![image-20240106160858971](.\assets\image-20240106160858971.png)

![image-20240106161337984](.\assets\image-20240106161337984.png)

![image-20240106161411109](.\assets\image-20240106161411109.png)

### 实现 `trap_handler`

处理中断。判断是否为 Interrupt & supervisor time interrupt 即可。在进入中断和结束中断时额外写了一次 sstatus 以保证处理中断时不接收新中断，实际上在整个实验中处理中断的时长都不可能超过一个时间片，因此该处理没什么实际作用。

![image-20240106161830298](.\assets\image-20240106161830298.png)

### 实现 clock 相关函数

逻辑也无需额外说明。

![image-20240106161951275](.\assets\image-20240106161951275.png)

### 结果

// TODO

### 思考题

1. 入参优先使用 a0 - a7，不够时放入内存。返回值使用 a0 和 a1。callee save 的寄存器由被调用者保存，需要函数调用前后不变。caller save 则代表函数调用前后可能改变，如果 caller 需要这个寄存器的值需要自行保存。当然这只是 convention 而非 requisition，比如进程调度的函数执行前后肯定会发生改变。

### 讨论心得

lab1 是我全部 7 个实验中所花时间最长的一个。首先是 riscv ，x86 系列的 ebp + esp 调用规范和 riscv 的调用规范有诸多不同，编写了若干 demo 在 [godbolt](https://godbolt.org/) 上反复对照源码和汇编才逐渐熟悉。这部分大概花了两三天。

其次是 boot 过程，在硬件上从主板 ROM - bootloader - kernel 的过程，在 qemu 中被简化为“从 0x8020000” 处开始运行 kernel，背后的过程完全被隐藏，在这种情况下开始开发让我非常不安。大概也花了两天，深入了解了 BIOS 的 boot 流程，简单看了看 UEFI 的流程，才开始开发。有很多文章说 riscv 的 sbi 所做的工作不同，虽然我没有深入了解，但大体上肯定逃不过上电、初始化内存、初始化硬盘、初始化文件系统几个步骤。至于这中间它初始化了别的一些设备，带了什么驱动，都是现在不需要关心的细枝末节。

最后由于 gdb 未进行配置的原生状态实在是太不好用了，也大概花了一天多的时间折腾各种 gdb 前端或者社区的 gdbinit 配置。

把这三件事做完后在后续的实验过程中基本没有遇到大的难点，比较畅通无阻。