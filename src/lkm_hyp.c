#include <linux/init.h> 
#include <linux/module.h> 
#include <linux/errno.h>
#include <linux/types.h> 
#include <linux/const.h> 
#include <linux/fs.h> 
#include <linux/fcntl.h> 
#include <linux/smp.h> 
#include <linux/major.h> 
#include <linux/device.h> 
#include <linux/cpu.h> 
#include <linux/notifier.h> 
#include <linux/uaccess.h>
#include <linux/gfp.h> 
#include <linux/slab.h>
#include <asm/asm.h>
#include <asm/errno.h> 
#include "lkm_hyp.h"

/*check CPUID.1:ECX.VMX[bit 5] = 1 */ 
bool vmx_support(void) 
{
    unsigned int ecx; 

   __asm__ volatile (
        "cpuid"
        : "=c"(ecx)        
        : "a"(1)          
        : "ebx", "edx"
    );  

    return (ecx & (1 << 5)) != 0;  
}

bool get_vmx_operation(void)
{
    /*set bit 13 of CR4 to enbale VMX virtualization on CPU */ 
    
    unsigned long cr4; 

    __asm__ __volatile__(
        "mov %%cr4, %0"
        : "=r" (cr4)
        : : "memory" 
    );

    cr4 |= X86_CR4_VMXE; 

    __asm__ __volatile__(
        "mov %0, %%cr4"
        :
        :"r"(cr4)
        : "memory"
    ); 

    /*configure MSR_IA32_FEATURE_CONTROL MSR to allow vmxon*/

    uint64_t feature_control; 
    uint64_t required; 
    u32 low1 = 0; 

    required = IA32_FEATURE_CONTROL_LOCKED; 
    required |= IA32_FEATURE_CONTROL_MSR_VMXON_ENABLE_OUTSIDE_SMX; 

    feature_control = __rdmsr1(MSR_IA32_FEATURE_CONTROL);

    printk(KERN_INFO "RDMSR output id %ld", (long)feature_control);
    
    if((feature_control & required) != required)
    {
        /*bit 0-31(low): 0s *
        * bit 32-63(high): modified feature value */  
        wrmsr(MSR_IA32_FEATURE_CONTROL, feature_control | required, low1);
    }

    /*ensure bits in cr0 and cr4 are valid for VMX operation*/

    cr4 = 0; 
    unsigned long cr0;  

    __asm__ __volatile__ (
        "mov %%cr0, %0"
        : "=r" (cr0)
        :
        : "memory"
    );
    cr0 &=  __rdmsr1(MSR_IA32_VMX_CR0_FIXED1); /*mask all allowd bits to 1 */
    cr0 |=  __rdmsr1(MSR_IA32_VMX_CR0_FIXED0); /*set all allowd bits */

    __asm__ __volatile__ (
        "mov %0, %%cr0"
        : 
        :"r" (cr0) 
        :"memory"
    ); 

    __asm__ __volatile__ (

        "mov %%cr4, %0"
        :"=r" (cr4)
        :
        :"memory"
        ); 

    cr4 &= __rdmsr1(MSR_IA32_VMX_CR4_FIXED1); 
    cr4 |= __rdmsr1(MSR_IA32_VMX_CR4_FIXED0); 

    __asm__ __volatile__ (
        "mov %0, %%cr4"
        :
        :"r"(cr4)
        :"memory"
     );

    long int vmxon_phy_region = 0; 

    /*allocate 4kb memory for vmxon region */  
    vmxon_region = kzalloc(VMXON_REGION_PAGE_SIZE, GFP_KERNEL); 
    
    if(!vmxon_region)
    {
        printk(KERN_INFO "ERROR alloacating vmxon region\n"); 
        return false; 
    }

    /*convert virtual address to physical address */ 
    vmxon_phy_region = __pa(vmxon_region);

    /*write rivison id to first 32bit(4bytes) of vmxon region memory */ 
    *(uint32_t *)vmxon_region = vmcs_revision_id(); 

    if(_vmxon(vmxon_phy_region))
    {
        return false; 
    }
    return true;
}


bool vmcs_set(void)
{
    long int vmcs_phy_region = 0; 

    /*allocate 4kb memory for vmcs region */ 

    if(!allocate_vmcs_region())
    {
        return false; 
    }
    
    vmcs_phy_region = __pa(vmcs_region); 

    /*insert revison identifier in first 32 bits of vmcs region*/ 

    *(uint32_t *) vmcs_region = vmxon_phy_region();

    if(_vmptrld(vmcs_phy_region))
    {
        return flase; 
    }
    return true; 

}

/*set up vmcs execution control field */ 

bool init_vmcs_control_field(void) 
{
    /*setting pin based controls 
     * proc based controls
     * vm exit and entry controls */

    /*  pin-based controls */ 
    uint64_t pinbased_control_msr = __rdmsr1(MSR_IA32_VMX_PINBASED_CTLS);
    uint32_t pin_allowed0 = (uint32_t)(pinbased_control_msr & 0xFFFFFFFF);
    uint32_t pin_allowed1 = (uint32_t)(pinbased_control_msr >> 32);
    uint32_t pinbased_control_desired = 0; // Specify your desired features here
    uint32_t pinbased_control_final = (pinbased_control_desired | pin_allowed1) & (pin_allowed0 | pin_allowed1);
    _vmwrite(PIN_BASED_EXEC_CONTROLS, pinbased_control_final);

    /* primary processor-based controls */ 
    uint64_t procbased_control_msr = __rdmsr1(MSR_IA32_VMX_PROCBASED_CTLS);
    uint32_t proc_allowed0 = (uint32_t)(procbased_control_msr & 0xFFFFFFFF);
    uint32_t proc_allowed1 = (uint32_t)(procbased_control_msr >> 32);
    uint32_t procbased_control_desired = 0; 
    uint32_t procbased_control_final = (procbased_control_desired | proc_allowed1) & (proc_allowed0 | proc_allowed1);
    _vmwrite(PROC_BASED_EXEC_CONTROLS, procbased_control_final);

    /* secondary processor-based controls */ 
    uint64_t procbased_secondary_control_msr = __rdmsr1(MSR_IA32_VMX_PROCBASED_CTLS2);
    uint32_t proc_secondary_allowed0 = (uint32_t)(procbased_secondary_control_msr & 0xFFFFFFFF);
    uint32_t proc_secondary_allowed1 = (uint32_t)(procbased_secondary_control_msr >> 32);
    uint32_t procbased_secondary_control_desired = 0;
    uint32_t procbased_secondary_control_final = (procbased_secondary_control_desired | proc_secondary_allowed1) & (proc_secondary_allowed0 | proc_secondary_allowed1);
    _vmwrite(PROC2_BASED_EXEC_CONTROLS, procbased_secondary_control_final);

    /* vm-exit controls */
    uint64_t vm_exit_control_msr = __rdmsr1(MSR_IA32_VMX_EXIT_CTLS);
    uint32_t vm_exit_allowed0 = (uint32_t)(vm_exit_control_msr & 0xFFFFFFFF);
    uint32_t vm_exit_allowed1 = (uint32_t)(vm_exit_control_msr >> 32);
    uint32_t vm_exit_control_desired = 0; 
    uint32_t vm_exit_control_final = (vm_exit_control_desired | vm_exit_allowed1) & (vm_exit_allowed0 | vm_exit_allowed1);
    _vmwrite(VM_EXIT_CONTROLS, vm_exit_control_final);

    /* vm-entry controls */ 
    uint64_t vm_entry_control_msr = __rdmsr1(MSR_IA32_VMX_ENTRY_CTLS);
    uint32_t vm_entry_allowed0 = (uint32_t)(vm_entry_control_msr & 0xFFFFFFFF);
    uint32_t vm_entry_allowed1 = (uint32_t)(vm_entry_control_msr >> 32);
    uint32_t vm_entry_control_desired = 0; 
    uint32_t vm_entry_control_final = (vm_entry_control_desired | vm_entry_allowed1) & (vm_entry_allowed0 | vm_entry_allowed1);
    _vmwrite(VM_ENTRY_CONTROLS, vm_entry_control_final);
}
}

static int __init hyp_init(void)
{
    if(!vmx_support())
    {
        printk(KERN_INFO "VMX not surpported! EXITING"); 
        return 0; 
    }

    printk(KERN_INFO "VMX is surpported! CONTINUING"); 

    if(!get_vmx_operation())
    {
        printk(KERN_INFO "VMX operation failed! EXITING"); 
        return 0; 
    }

    printk(KERN_INFO "VMX operation succeeded! CONTINUING"); 

    __asm__ __volatile__ (
        "vmxoff\n"
        : : 
        :"cc"
    ); 
    return 0; 
}

static void __exit hyp_exit(void)
{
    printk(KERN_INFO "exiting hypervisor\n"); 
}

module_init(hyp_init); 
module_exit(hyp_exit); 

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Chrinovic M");
MODULE_DESCRIPTION("Lightweight Hypervisior ");
    
