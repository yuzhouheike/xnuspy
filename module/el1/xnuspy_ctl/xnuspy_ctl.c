#include <mach/mach.h>
#include <stdbool.h>
#include <stdint.h>

/* struct xnuspy_ctl_args { */
/*     uint64_t arg0; */
/*     uint64_t arg1; */
/*     uint64_t arg2; */
/*     uint64_t arg3; */
/* }; */

#define MARK_AS_KERNEL_OFFSET __attribute__((section("__DATA,__koff")))

MARK_AS_KERNEL_OFFSET uint64_t iOS_version = 0;
MARK_AS_KERNEL_OFFSET void *kalloc_canblock(vm_size_t *sizep, bool canblock,
        void *site);

/* int xnuspy_ctl(void *p, struct xnuspy_ctl_args *uap, int *retval){ */
int xnuspy_ctl(void *p, uint32_t arg0, uint32_t arg1, uint32_t arg2,
        uint32_t arg3, int *retval){
    uint64_t val = 0x4433221155667788;
    asm volatile("mov x0, %0" : : "r" (val) : );
    asm volatile("blr x0");


    return 0;
}
