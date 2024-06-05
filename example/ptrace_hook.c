#include <errno.h>
#include <mach/mach.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <xnuspy/xnuspy_ctl.h>

static void (*_bzero)(void *p, size_t n);
static int (*copyinstr)(const void *uaddr, void *kaddr, size_t len, size_t *done);
static void *(*current_proc)(void);
static void (*kprintf)(const char *, ...);
static void (*proc_name)(int pid, char *buf, int size);
static pid_t (*proc_pid)(void *);
static int (*_strcmp)(const char *s1, const char *s2);
static void *(*unified_kalloc)(size_t sz);
static void (*unified_kfree)(void *ptr);

static uint64_t kernel_slide;

static uint8_t curcpu(void){
    uint64_t mpidr_el1;
    asm volatile("mrs %0, mpidr_el1" : "=r" (mpidr_el1));
    return (uint8_t)(mpidr_el1 & 0xff);
}

static pid_t caller_pid(void){
    return proc_pid(current_proc());
}

/* bsd/sys/uio.h */
enum uio_seg {
    UIO_USERSPACE       = 0,    /* kernel address is virtual,  to/from user virtual */
    UIO_SYSSPACE        = 2,    /* kernel address is virtual,  to/from system virtual */
    UIO_USERSPACE32     = 5,    /* kernel address is virtual,  to/from user 32-bit virtual */
    UIO_USERSPACE64     = 8,    /* kernel address is virtual,  to/from user 64-bit virtual */
    UIO_SYSSPACE32      = 11    /* deprecated */
};

#define UIO_SEG_IS_USER_SPACE( a_uio_seg )  \
    ( (a_uio_seg) == UIO_USERSPACE64 || (a_uio_seg) == UIO_USERSPACE32 || \
      (a_uio_seg) == UIO_USERSPACE )

/* bsd/sys/namei.h */
#define PATHBUFLEN 256

struct nameidata {
    char * /* __user */ ni_dirp;
    enum uio_seg ni_segflag;
    /* ... */
};

struct ptrace_args {
    int req;         // 4 bytes
    void * unknow1[4];        //和实际内存的中偏移不符.实际pid 是+8，所以加了unknow填充
    
    pid_t pid;   
    
    // void * unknow2[];     // 4 bytes
    user_addr_t addr;// 8 bytes (on a 64-bit system)
    int data;        // 4 bytes
};

static int targetId;
#define BLOCKED_FILE "/var/mobile/testfile.txt"
static int (*ptrace_orig)(struct proc *p, struct ptrace_args *uap, int32_t *retval);

static int my_ptrace(struct proc *p, struct ptrace_args *uap, int32_t *retval){



    kprintf("\n ptrace -- start \n");

    uint8_t cpu = curcpu();
    pid_t caller = caller_pid();

    char *caller_name = unified_kalloc(MAXCOMLEN + 1);

    if(!caller_name){
    kprintf("\n ptrace -- caller_name \n");
        return ptrace_orig(p,uap,retval);
    }
    

    /* proc_name doesn't bzero for some version of iOS 13 */
    _bzero(caller_name, MAXCOMLEN + 1);
    proc_name(caller, caller_name, MAXCOMLEN + 1);

    kprintf("$$$$$$$$$$$$ %s: (CPU %d): '%s' (%d) ptrace_hook  to req : %d pid : %d  \n", __func__, cpu,
            caller_name, caller,uap->req,uap->pid);



    unified_kfree(caller_name);


    if (uap->req == 31 || uap->req == 14)
    {
        /* code */


  
        kprintf("uap->req == 31 uap->req %d \n",uap->req);
        
        kprintf("uap->req == 31 uap->pid %d \n",uap->pid);
        
        kprintf("uap->req == 31 uap->addr %d \n",uap->addr);
        kprintf("uap->req == 31 uap->data %d \n",uap->data);

        
        return 0;
  

    }
    // else{
    //     kprintf("chage uap->req %d",uap->req);
    //     kprintf("chage uap->pid %d",uap->pid);
    // }
    
    // kprintf("ptrace -- end");

    // return 0;
    return ptrace_orig(p,uap,retval);
}




static long SYS_xnuspy_ctl = 0;

static int gather_kernel_offsets(void){
    int ret;
#define GET(a, b) \
    do { \
        ret = syscall(SYS_xnuspy_ctl, XNUSPY_CACHE_READ, a, b, 0); \
        if(ret){ \
            printf("%s: failed getting %s\n", __func__, #a); \
            return ret; \
        } \
    } while (0)

    GET(BZERO, &_bzero);
    GET(COPYINSTR, &copyinstr);
    GET(CURRENT_PROC, &current_proc);
    GET(KPRINTF, &kprintf);
    GET(PROC_NAME, &proc_name);
    GET(PROC_PID, &proc_pid);
    GET(STRCMP, &_strcmp);
    GET(UNIFIED_KALLOC, &unified_kalloc);
    GET(UNIFIED_KFREE, &unified_kfree);

    return 0;
}

int main(int argc, char **argv){


        printf("useage: ptrace_hook target_pid \n");
        printf("argc = %d\n", argc);
    for (int i = 0; i < argc; ++i)
    {
        printf("argv[%d] =%s\n", i,argv[i]);
    }
    // int pid = atoi(argv[1]);
    size_t oldlen = sizeof(long);
    int ret = sysctlbyname("kern.xnuspy_ctl_callnum", &SYS_xnuspy_ctl,
            &oldlen, NULL, 0);

    if(ret == -1){
        printf("sysctlbyname with kern.xnuspy_ctl_callnum failed: %s\n",
                strerror(errno));
        return 1;
    }

    ret = syscall(SYS_xnuspy_ctl, XNUSPY_CHECK_IF_PATCHED, 0, 0, 0);

    if(ret != 999){
        printf("xnuspy_ctl isn't present?\n");
        return 1;
    }

    ret = gather_kernel_offsets();

    if(ret){
        printf("something failed: %s\n", strerror(errno));
        return 1;
    }

    printf("uSYS_xnuspy_ctl \n");
    /* iPhone 7 14.1 */                                //FFFFFFF007584D9C                          
    ret = syscall(SYS_xnuspy_ctl, XNUSPY_INSTALL_HOOK, 0xFFFFFFF007584D9C,
            my_ptrace, &ptrace_orig);

    if(ret){
        printf("Could not hook ptrace: %s\n", strerror(errno));
        return 1;
    }

    for(;;){
        int fd = open(BLOCKED_FILE, O_CREAT);

        if(fd == -1)
            printf("open ptrace failed: %s\n", strerror(errno));
        else{
            printf("Got valid fd? %d\n", fd);
            close(fd);
        }

        sleep(1);
    }
    // for(;;){
     
    //     static int flag = 0;
    //     printf("ptrace_hook %d",flag);
    //     flag = flag + 1;

    //     sleep(1);
    // }

    return 0;
}
