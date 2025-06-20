
#ifndef LKM_HYP_H 
#define LKM_HYP_H

#include <linux/const.h>

#define X86_CR4_VMXE_BIT    13 
#define X86_CR4_VMXE        _BITUL(X86_CR4_VMXE_BIT)


/*enablling vmx through IA32_FEATURE_CONTROL_MSR */ 
#define IA32_FEATURE_CONTROL_LOCKED     (1 << 0)
#define IA32_FEATURE_CONTROL_MSR_VMXON_ENABLE_OUTSIDE_SMX (1 << 2)
#define MSR_IA32_FEATURE_CONTROL        0x0000003A 


#define MSR_IA32_VMX_CR0_FIXED0         0x00000486 
#define MSR_IA32_VMX_CR0_FIXED1         0x00000487 
#define MSR_IA32_VMX_CR4_FIXED0         0x00000488 
#define MSR_IA32_VMX_CR4_FIXED1         0x00000489 

/*revison idenfiier */ 

#define MSR_IA32_VMX_BASIC              0x00000480 

#define VMXON_REGION_PAGE_SIZE          4096 
#define VMCS_REGION_PAGE_SIZE           4096


/*------------- vm-execution control field ---------------*/ 

/*vmcs control field MSRs */ 

#define MSR_IA32_VMX_PINBASED_CTLS      0x00000481 
#define MSR_IA32_VMX_PROCBASED_CTLS     0x00000482
#define MSR_IA32_VMX_EXIT_CTLS          0x00000483 
#define MSR_IA32_VMX_ENTRY_CTLS         0x00000484 

/*control field encodings  */  

#define PIN_BASED_VM_EXEC_CONTROLS      0x00004000 
#define PROC_BASED_VM_EXEC_CONTROLS     0x00004002 
#define VM_EXIT_CONTROLS                0x0000400c 
#define VM_ENTRY_CONTROLS               0x00004012 

#define VM_INSTRUCTION_ERROR_FIELD      0x4400 

#define VMCS_EXCEPTION_BITMAP           0x00004004

#define VMCS_IO_BITMAP_A                0x00002000 
#define VMCS_IO_BITMAP_B                0x00002002 

#define IO_BITMAP_PAGE_SIZE             4096
#define IO_BITMAP_PAGES_ORDER           1 
#define IO_BITMAP_SIZE                  (IO_BITMAP_PAGE_SIZE << IO_BITMAP_PAGES_ORDER);

uint8_t *io_bitmap; 

/*memory regions */

uint64_t *vmxon_region = NULL; 
uint64_t *vmcs_region  = NULL; 

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
        ".long 1b, 2b, ex_handler_rdmsr_unsafe - .\n"
        ".popsection\n"

        : "=a" (low), "=d" (high) 
        : "c" (msr)
        : "memory"
    ); 

    return ((unsigned long long)high << 32) | low; 
}


static inline uint32_t vmcs_revision_id(void)
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

/*allocate physical memory for vmcs region */ 

bool alloc_vmcs_region(void)
{
    vmcs_region = kzalloc(VMCS_REGION_PAGE_SIZE, GFP_KERNEL); 

    if(!vmcs_region)
    {
        printk(KERN_INFO "ERROR alloacating vmcs region\n"); 
        return false; 
    }
    return true; 
}

static inline int _vmptrld(uint64_t vmcs_phys_addr)
{
    uint8_t ret; 

    __asm__ __volatile__ (
        "vmptrld %[pa]; setna %[ret]"
        :[ret] "=rm"(ret)
        :[pa]  "m"  (vmcs_phys_addr)
        : "cc", memory
    ); 
}

static inline int _vmread(uint64_t field_enc, uint64_t *value)
{
    uint8_t ret; 
    uint64_t val; 

    __asm__ __volatile__ (
        "vmread %[field], %[val]; setna %[ret]"
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

        if(_vmread(VM_INSTRUCTION_ERROR_FIELD, &error_code) = 0)
        {
            return (int)error_code; 
        } 
        else
        {
            return - 1; 

        }

    }
}
#endif 
