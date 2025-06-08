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
#include <utils.h>

/*check CPUID.1:ECX.VMX[bit 5] = 1 */ 
bool vmx_surpport(void) 
{
    int get_vmx_surpport; 
    int vmx_bit; 

    __asm__("mov %1. %rax"); 
    __asm__("cpuid"); 
    __asm__("mov %%ecx, %0\n\t
            :"=r" (get_vmx_surpport)); 

    vmx_bit = (get_vmx_surpport >> 5) & 0x1;

    if(vmx_bit == 1)
    {
        return true; 
    }
    else{
        return false;
    }
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

    feature_control = _rdmsr1(MSR_IA32_FEATURE_CONTROL);

    printk(KERN_INFO "RDMSR output id %ld", (long)feature_control);
    
    if((feature_control & required) != required)
    {
        /*bit 0-31(low): 0s *
        * bit 32-63(high): modified feature value */  
        wrmsr(MSR_IA32_FEATURE_CONTROL, feature_control | required, low1)
    }

    /*ensure bits in cr0 and cr4 are valid for VMX operation*/

    unsigned long cr0; 
    unsigned long cr4; 

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

    vmxon_region = kzalloc(VMXON_REGION_PAGE_SIZE, GFP_KERNEL); 
    
    if(!vmxon_region)
    {
        printk(KERN_INFO "ERROR alloacating vmxon region\n"); 
        return false; 
    }

    vmxon_phy_region = __pa(vmxon);
    *(uint32_t *)vmxon_region = vmcs_revision_id(); 




}



