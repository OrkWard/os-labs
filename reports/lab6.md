### 准备工作

扩容 task 数组、修改初始化和调度的部分就不详述了。此外，为了方便添加映射，`vm_area_struct` 数组额外添加了一个域标识是否已经映射：

![image-20240104020744767](/home/orks/Repos/os-labs/reports/assets/image-20240104020744767.png)

### 修改 _traps 调用

首先需要说明，我的 `_traps` 写法比较特殊，进入时只将 caller save 的寄存器保存在栈中，callee save 的寄存器在 `__switch_to` 调用中切换。由于 `trap_handler` 本身是一个完整函数调用，编译器可以帮忙保证 callee save 寄存器的不变性，而进程切换时也等价于保存并切换了全部寄存器，因此在前面的实验中都没遇到问题。

但在此实验中，由于从中间直接开始运行，需要做额外的处理。先来看一下原先的流程：

```
normal trap handle:
  _traps -> save caller regs -> call trap_handler -> load caller regs

task switch:
  _traps -> save caller regs(task1) -> call trap_handler -> __switch_to -> save callee regs(task1) -> load callee regs(task2) -> load caller regs(task2)
```

而创建子进程后，子进程第一次被调度时，有：

```
_traps -> save caller regs(parent) -> call trap_handler -> __switch_to -> save callee regs(parent) -> load callee regs(child) -> load caller regs(child)
```

流程跟上面完全一样。但是！caller save 的寄存器存在栈上，也就是该进程对应的内核栈，因此在创建内核栈时已经从父进程拷贝；而 callee save 的寄存器虽然也存在栈上，但**并不与 fork 调用时父进程的状态一致**，从父进程拷贝过来的寄存器**仍为父进程初始化时的状态**，这显然不是我们所期望的。因此这里要手动填充 `thread_struct` 中所有的域：

![image-20240104022637986](/home/orks/Repos/os-labs/reports/assets/image-20240104022637986.png)

而这些东西都通过 `pt_regs` 结构体传进来，因此我们需要在 _traps 中额外地往栈上存一些东西：

![image-20240104022823350](/home/orks/Repos/os-labs/reports/assets/image-20240104022823350.png)

sepc 寄存器在 `__swtich_to` 中设置，这里直接忽略，从其他寄存器开始恢复：

![image-20240104022908352](/home/orks/Repos/os-labs/reports/assets/image-20240104022908352.png)

### 修改 sys_call 函数

添加分支处理 fork 调用：

![image-20240104023117921](/home/orks/Repos/os-labs/reports/assets/image-20240104023117921.png)

#### 1. 创建子进程内核栈并复制父进程内核栈

![image-20240104023205393](/home/orks/Repos/os-labs/reports/assets/image-20240104023205393.png)

#### 2. 填充的 `thread` 结构体

![image-20240104023247323](/home/orks/Repos/os-labs/reports/assets/image-20240104023247323.png)

sp 的值通过父进程 sp 与 父进程内核栈底的偏移计算得到。ra 设置为创建的符号，其他域上述已有说明。

#### 3. 子进程 fork 返回值

![image-20240104023746107](/home/orks/Repos/os-labs/reports/assets/image-20240104023746107.png)

由于使子进程正常调度的寄存器已经在上一部中完全处理，这里简单设置返回值为 0 即可。

#### 4. 分配并拷贝内存

利用新添加的 mapped 域，判断一下做映射即可。映射完直接拷贝。这里本来打算复用之前的 Page Fault 处理函数，但由于有多处冲突且需要用到中间变量，遂作罢。

![image-20240104023959551](/home/orks/Repos/os-labs/reports/assets/image-20240104023959551.png)

#### 5. 将子进程放入任务队列

走一个循环即可。顺便设置一下父进程的 fork 返回值

![image-20240104025449370](/home/orks/Repos/os-labs/reports/assets/image-20240104025449370.png)

### 结果

> 为了方便演示，这里调整了 priority 算法和 TIMECLOCK

第一次调度，2(4) -> 4 -> 3(5) -> 5 -> 1(6) -> 6，括号表示创建子进程

![image-20240104025911338](/home/orks/Repos/os-labs/reports/assets/image-20240104025911338.png)

第二次调度，5 -> 3 -> 4 -> 2 -> 6 -> 1，可以观察到父/子进程的全局变量确实互相独立。

![image-20240104030229613](/home/orks/Repos/os-labs/reports/assets/image-20240104030229613.png)

### 思考题

1. 内核栈；整个 thread_struct 结构体。结构体里包括了 vmas 和 thread（callee save registers）
2. 上面已经提到：*sp 的值通过父进程 sp 与 父进程内核栈底的偏移计算得到*
3. 我的写法不需要设置 sp 和 sepc。原来方式中，sp 在进入 `__switch_to` 时会被替换，因此走到 `__ret_from_fork` 时还需要取一次 sp。我在 `_traps` 中没有取 sp 的动作，栈以自然的方式伸缩。

### 讨论心得

如果按照正常实验流程，所有 callee save 寄存器和一些 csr 会被**存/取两次**，这在本实验中会**非常让人迷惑**，出了问题排查困难。现在我的代码中 sepc 在正常进程调度时还被存/取了两次，暂时没有精力分辨能否删去。

归根到底 caller/callee 都是 convention 而非 demand，只要能中断发生前后、函数调用前后程序正常运行即可；但直接写汇编带来的高自由度极容易产生问题。

怎么解决自然是反复确认流程仔细 debug 解决。