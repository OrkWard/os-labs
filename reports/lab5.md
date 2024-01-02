### 实现 `do_mmap`

`do_mmap` 函数接受一个 `task_struct` 和必要的信息，创建一个 vma。

首先增加 vma 计数，随后找到新增的 vma 地址；随后填入相应的信息即可：

![image-20240102145338537](/home/orks/Repos/os-labs/reports/assets/image-20240102145338537.png)

由于 `task_struct` 实际分配的大小是一整页，所以此处可以不用开辟新的内存，直接计算指针位置即可；同时，由于 vmas 成员位于最后一项，也不需要额外的偏移。

这里还是使用了 `file_offset_on_disk` 成员，后面实际映射时引用该成员即可。

### 实现 `find_vma`

遍历 vma 检查边界即可，不存在时返回 0 指针

![image-20240102150202911](/home/orks/Repos/os-labs/reports/assets/image-20240102150202911.png)

### 实现 `do_page_fault`

接受一个 `pt_regs` 结构体，进行映射。

首先根据发生 page fault 的地址找到 vma：

![image-20240102152023284](/home/orks/Repos/os-labs/reports/assets/image-20240102152023284.png)

随后根据 vma 是否为匿名（即是否有对应文件）进行映射：

![image-20240102152043515](/home/orks/Repos/os-labs/reports/assets/image-20240102152043515.png)

跟前面的实验一样，由于 vm_start 不一定页对齐，size 需要把 vm_start 的页内偏移考虑在内。原因是 `alloc_pages` 从页首开始分配内存。

正常创建映射即可，创建后直接返回。

### 修改 `trap_handler`

增加 12、13、15 号 exception 的处理。由于具体需要的权限已经在 `vma->vm_flags` 中，这里统一交给 `do_page_default` 函数处理即可。

![image-20240102152519477](/home/orks/Repos/os-labs/reports/assets/image-20240102152519477.png)

### 修改 `task_init`

删除原先分配内存、复制文件、创建映射的部分，改为调用一次 `do_mmap`

![image-20240102153458567](/home/orks/Repos/os-labs/reports/assets/image-20240102153458567.png)

### 结果

第一次调度 2-3-1；第二次调度 3-2-1（priority 有改动，为方便起见）

`Instruction Page Fault` 时 scause 为 12，`Store Page Fault` 时 scause 为 15（见上图）

共发生 6 次 Page Fault

![image-20240102160804844](/home/orks/Repos/os-labs/reports/assets/image-20240102160804844.png)

### 思考题

1. 本实验中可以不使用。首先为什么 `vm_content_size_in_file` 和 `vm_end - vm - start` 会不同，这是由于 bss 段的存在：bss 段存放*未初始化*的静态变量，由于它们没有初始数据，为了节省空间起见在 ELF 中仅用一个数字代表 size，即这个 section 仅存在于内存中，不存在于 executable 中。

   而这就导致进行 `memcpy` 复制用户可执行文件时，需要使用 `size_in_file` 而不是开辟内存时用的 size，否则可能会导致静态变量初始化为意料之外的值。本实验的用户程序中不涉及，因此用哪个都无所谓。

   另外值得一提的是出于安全生产的考虑，未初始化的值在任何情况下都不应该被使用，除非语言保证变量始终会被初始化。依赖 os loader 的行为属于不必要的外部依赖。

2. 这个域并不占用内存，它起到的作用类似一个结构体内部的 symbol。`&vma_cnt + sizeof(vma_cnt)` 也能起到一样的效果。

   可以换，仍然能找到这个结构体的末端地址，但这样就没有添加这个 symbol 的意义了。

### 讨论心得

理解了 Page Fault 在做什么以后本实验难度不大，毕竟完全没有涉及内存/硬盘间的换页问题，只是做了基本的处理，以至于也没什么可说的，整个实验就是置后了创建映射的时机。除了查了下 size_in_file 的用途外，还翻了下手册找 scause 各种值的含意，此外本实验没有其他需要额外查资料的问题了。