
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

#define VMXON_REGION_PAGE_SIZE                 4096 

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

static inline uint8_t _vmxon(uint64_t phys)
{
    uint8_t ret; 
    
    __asm__ __volatile__ (
        "vmxon %[pa]; setna %[ret]"
        :[ret]"=rm"(ret)
        :[pa]"m"(phys)
        :"cc", "memory"
    );

    return ret; 
}

#endif 
