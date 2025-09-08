
#ifndef LKM_HYP_H 
#define LKM_HYP_H

#include <linux/const.h>
#include "linux/kern_levels.h"
#include "vmcs_state.h"

#define X86_CR4_VMXE_BIT    13 
#define X86_CR4_VMXE        _BITUL(X86_CR4_VMXE_BIT)


/*enablling vmx through IA32_FEATURE_CONTROL_MSR */ 
#define IA32_FEATURE_CONTROL_LOCKED     (   1 << 0)
#define IA32_FEATURE_CONTROL_MSR_VMXON_ENABLE_OUTSIDE_SMX (1 << 2)
#define MSR_IA32_FEATURE_CONTROL            0x0000003A 


#define MSR_IA32_VMX_CR0_FIXED0             0x00000486 
#define MSR_IA32_VMX_CR0_FIXED1             0x00000487 
#define MSR_IA32_VMX_CR4_FIXED0             0x00000488 
#define MSR_IA32_VMX_CR4_FIXED1             0x00000489 

/*revison idenfiier */ 

#define MSR_IA32_VMX_BASIC                  0x00000480 

#define VMXON_REGION_PAGE_SIZE              4096 
#define VMCS_REGION_PAGE_SIZE               4096


/*------------- vm-execution control field ---------------*/ 

/*VMCS control field MSRs */ 

#define MSR_IA32_VMX_PINBASED_CTLS          0x00000481 
#define MSR_IA32_VMX_PROCBASED_CTLS         0x00000482
#define MSR_IA32_VMX_EXIT_CTLS              0x00000483 
#define MSR_IA32_VMX_ENTRY_CTLS             0x00000484 

/*control field encodings  */  

#define VMCS_PIN_BASED_EXEC_CONTROLS        0x00004000 
#define VMCS_PROC_BASED_EXEC_CONTROLS       0x00004002 
#define VMCS_EXIT_CONTROLS                  0x0000400c 
#define VMCS_ENTRY_CONTROLS                 0x00004012 
#define VM_EXIT_HOST_ADDR_SPACE_SIZE        0x00000200 
#define VM_ENTRY_IA32E_MODE                 0x00000200 
#define VMCS_INSTRUCTION_ERROR_FIELD        0x00004400 

#define VMCS_EXCEPTION_BITMAP               0x00004004

/* for checking bit 28 of procbased control if msr bitmaps is enabled (bit 28 = 1) */ 

#define VMCS_MSR_BITMAPS_BIT                 28 

/*msr vitmap field in the VMCS */ 

#define VMCS_MSR_BITMAP                     0x00002004

#define VMCS_IO_BITMAP_A                    0x00002000 
#define VMCS_IO_BITMAP_B                    0x00002002 

#define VMCS_IO_BITMAP_PAGE_SIZE             4096
#define VMCS_IO_BITMAP_PAGES_ORDER           1 
#define VMCS_IO_BITMAP_SIZE                  (VMCS_IO_BITMAP_PAGE_SIZE << VMCS_IO_BITMAP_PAGES_ORDER)

#define VMCS_CR0_GUEST_HOST_MASK            0x00006004
#define VMCS_CR0_READ_SHADOW                0x00006006
#define VMCS_CR4_GUEST_HOST_MASK            0x00006008
#define VMCS_CR4_READ_SHADOW                0x0000600A

/* VMCS field encodings for CR3 targets */ 

#define VMCS_CR3_TARGET_VALUE0              0x0000600C
#define VMCS_CR3_TARGET_VALUE1              0x0000600E
#define VMCS_CR3_TARGET_VALUE2              0x00006010
#define VMCS_CR3_TARGET_VALUE3              0x00006012

/*for storing MSRs on vm exit */

#define MSR_AREA_ENTRIES                    1 
#define VM_EXIT_MSR_STORE_COUNT             0x00002004 
#define VM_EXIT_MSR_STORE_ADDR              0x00002006
#define VM_ENTRY_MSR_LOAD_COUNT             0x00002008 
#define VM_ENTRY_MSR_LOAD_ADDR              0x0000200A

#define IA32_SYSENTER_CS                    0x00000174

/*memory regions */

uint64_t *vmxon_region = NULL; 
uint64_t *vmcs_region  = NULL; 

uint8_t *io_bitmap; 
void * msr_bitmap; 


asmlinkage void ex_handler_rdmsr_unsafe(void); 
asmlinkage void ex_handler_rdmsr_unsafe(void)
{

}

static inline unsigned long long notrace __rdmsr1(unsigned int msr)
{
    /* low = eax = 0-31 
     * high = edx = 32-63 */  

    unsigned int high;  
    unsigned int low; 

    __asm__ __volatile__ (
        "1: rdmsr\n"
        "2:\n"
        /*switch to exception table temporarily */ 
        ".pushsection __ex_table, \"a\"\n"
        ".balign 8\n"
        ".long (1b - .), (2b - .), ex_handler_rdmsr_unsafe - .\n"
        ".popsection\n"

        : "=a" (low), "=d" (high) 
        : "c" (msr)
        : "memory"
    ); 

    return ((unsigned long long)high << 32) | low; 
}


static inline uint64_t _read_cr0(void)
{
    uint64_t val;

    __asm__ __volatile__ (
        "mov %%cr0, %0" 
        : "=r"(val)
    );

    return val;
}

static inline uint64_t _read_cr2(void)
{
    uint64_t val;

    __asm__ __volatile__ (
        "mov %%cr2, %0" 
        : "=r"(val)
    );

    return val;
}

static inline uint64_t _read_cr3(void) 
{
    uint64_t val;

    __asm__ __volatile__ (
        "mov %%cr3, %0" 
        :"=r"(val)
    );

    return val;
}

static inline uint64_t _read_cr4(void) 
{
    uint64_t val;

    __asm__ __volatile__ ("mov %%cr4, %0" 
        : "=r"(val)
    );

    return val;
}


/*get revision id*/ 

static inline uint32_t _vmcs_revision_id(void)
{
    return __rdmsr1(MSR_IA32_VMX_BASIC); 
}

static inline uint8_t _vmxon(uint64_t vmxon_phys_addr)
{
    uint8_t ret; 
    
    __asm__ __volatile__ (
        "vmxon %[pa]; setna %[ret]"
        :[ret] "=rm"(ret)
        :[pa]  "m"  (vmxon_phys_addr)
        :"cc", "memory"
    );

    return ret; 
}

static inline int _vmptrld(uint64_t vmcs_phys_addr)
{
    uint8_t ret; 

    __asm__ __volatile__ (
        "vmptrld %[pa]; setna %[ret]"
        :[ret] "=rm"(ret)
        :[pa]  "m"  (vmcs_phys_addr)
        : "cc", "memory"
    ); 
    return ret; 
}

static inline int _vmread(uint64_t field_enc, uint64_t *value)
{
    uint8_t ret; 
    uint64_t val; 

    __asm__ __volatile__ (
        "vmread %[field_enc], %[val]; setna %[ret]"
        :[ret] "=rm"(ret), [val] "=r"(val)
        :[field_enc] "r" (field_enc)
        :"cc"
    ); 
     
    *value = val;

    return ret ? 1 : 0;
}

static inline int _vmwrite(uint64_t field_enc, uint64_t value)
{
    uint8_t status; 

    __asm__ __volatile__ (
        "vmwrite %[value], %[field_enc]; setna %[status]"
        :[status] "=rm" (status)
        :[value] "rm"(value), [field_enc] "r" (field_enc)
        :"cc"
    ); 

    if(!status)
    {
        return 0; 
    }

    /*pushes error code to error field of vmcs on write fails */ 

    else
    {
        uint64_t error_code; 

        if(_vmread(VMCS_INSTRUCTION_ERROR_FIELD, &error_code) == 0)
        {
            return (int)error_code; 
        } 
        else
        {
            return - 1; 

        }

    }
}

static inline int _vmlaunch(void)
{
    int ret;

    __asm__ __volatile__ (
        "push %%rbp;"
        "push %%rcx;"
        "push %%rdx;"
        "push %%rsi;"
        "push %%rdi;"
        "push $0;"
        "vmwrite %%rsp, %[host_rsp];"
        "lea 1f(%%rip), %%rax;"
        "vmwrite %%rax, %[host_rip];"
        "vmlaunch;"
        "incq (%%rsp);"
        "1: pop %%rax;"
        "pop %%rdi;"
        "pop %%rsi;"
        "pop %%rdx;"
        "pop %%rcx;"
        "pop %%rbp;"
        : [ret] "=&a"(ret)
        : [host_rsp] "r"((uint64_t)HOST_RSP),
        [host_rip] "r"((uint64_t)HOST_RIP)
        : "memory", "cc", "rbx", "r8", "r9", "r10", 
        "r11", "r12", "r14", "r15"
    );

    return ret; 
}

static inline int _get_vmcs_size(void)
{
    uint64_t vmx_basic = __rdmsr1(MSR_IA32_VMX_BASIC); 
    uint32_t vmcs_size = (vmx_basic > 32) & 0x1FFF; 

    if(!vmcs_size)
    {
        printk(KERN_ERR "Invalid VMCS size from VMX_BASIC MSR: 0x%ll\n", vmx_basic); 
        return -1; 

    }

    printk(KERN_INFO "VMX_BASIC MSR: 0x%llx, VMCS size: %u bytes\n", vmx_basic, vmcs_size);
    return vmcs_size;
}
struct _msr_entry 
{
    uint32_t index; 
    uint32_t reserved; 
    uint64_t value; 
}__attribute__ ((packed, aligned(16))); 

struct _msr_entry *_vm_exit_msr_area; 
struct _msr_entry *_vm_entry_msr_area; 



#endif 
