/* early_pgtbl: 用于 setup_vm 进行 1GB 的 映射。 */
#include "vm.h"
#include "defs.h"
#include "mm.h"
#include "printk.h"
#include "string.h"
#include "types.h"

extern uint64 _stext;
extern uint64 _etext;
extern uint64 _srodata;
extern uint64 _erodata;
extern uint64 _sdata;
extern uint64 _edata;

unsigned long early_pgtbl[512] __attribute__((__aligned__(0x1000)));

void setup_vm(void) {
    /*
    1. 由于是进行 1GB 的映射 这里不需要使用多级页表
    2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
        high bit 可以忽略
        中间9 bit 作为 early_pgtbl 的 index
        低 30 bit 作为 页内偏移 这里注意到 30 = 9 + 9 + 12，
        即我们只使用根页表， 根页表的每个 entry 都对应 1GB 的区域。
    3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    */
    // 0x80000000 = 2^31 = 2 * 2^30，PPN[2] = 2
    // 0x80000000 = 0x800000 * 2^12 = 0x800000 * page_size
    early_pgtbl[2] = 0x800000ul << 10 | 0b1111;

    // 0xffff_ffe0_0000_0000[PPN] = 0b110_000_000
    early_pgtbl[0b110000000u] = 0x800000ul << 10 | 0b1111;
}

/* swapper_pg_dir: kernel pagetable 根目录， 在 setup_vm_final 进行映射。 */
unsigned long swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

void setup_vm_final(void) {
    memset(swapper_pg_dir, 0x0, PGSIZE);

    // No OpenSBI mapping required

    // mapping kernel text X|-|R|V
    printk("%x\n%x\n", _stext, &_stext);
    create_mapping(swapper_pg_dir, _stext, _stext - PA2VA_OFFSET,
                   _etext - _stext, 0b1011);

    // mapping kernel rodata -|-|R|V
    create_mapping(swapper_pg_dir, _sdata, _sdata - PA2VA_OFFSET,
                   _erodata - _srodata, 0b11);

    // mapping other memory -|W|R|V
    create_mapping(swapper_pg_dir, _stext, _sdata - PA2VA_OFFSET,
                   _edata - _sdata, 0b111);

    // set satp with swapper_pg_dir
    asm volatile("csrw satp, %[table]" ::[table] "r"(swapper_pg_dir));

    // flush TLB
    asm volatile("sfence.vma zero, zero");

    // flush icache
    asm volatile("fence.i");

    return;
}

/**** 创建多级页表映射关系 *****/
/* 不要修改该接口的参数和返回值 */
void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz,
                    uint64 perm) {
    /*
    pgtbl 为根页表的基地址
    va, pa 为需要映射的虚拟地址、物理地址
    sz 为映射的大小，单位为字节
    perm 为映射的权限 (即页表项的低 8 位)

    创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
    可以使用 V bit 来判断页表项是否存在
    */
    uint64 page_number = PGROUNDUP(sz) / PGSIZE;
    while (page_number--) {
        uint64 vpn2 = ((va & 0x7fc0000000) >> 30);
        uint64 vpn1 = ((va & 0x3fe00000) >> 21);
        uint64 vpn0 = ((va & 0x1ff000) >> 12);
    }
}