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



