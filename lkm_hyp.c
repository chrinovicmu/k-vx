#include <linux/init.h> 
#include <linux/module.h>
#include <linux/kernel.h> 
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
#include <linux/string.h>
#include <linux/mm.h>
#include <asm/asm.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/io.h>
#include <asm/errno.h> 
#include "lkm_hyp.h"



#define CHECK_VMWRITE(field_enc, value)                                 \
    do {                                                               \
        if (_vmwrite((field_enc), (value))) {                           \
            printk(KERN_INFO "VMWrite failed: field_encoding: 0x%lx\n", \
                   (unsigned long)(field_enc));                         \
            return -EIO;                                                \
        }                                                              \
    } while (0)

/*check for vmx surppot on processor 
*check CPUID.1:ECX.VMX[bit 5] = 1 */ 

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

/*check if processor supports MSR bitmap  */ 

bool msr_bitmap_support(void)
{
    uint64_t proc_ctls = __rdmsr1(MSR_IA32_VMX_PROCBASED_CTLS); 
    uint32_t allowed_1_settings = (uint32_t)(proc_ctls >> 32); 

    return (allowed_1_settings & (1 << VMCS_MSR_BITMAPS_BIT)) != 0;
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

    phys_addr_t vmxon_phy_region = 0; 

    /*allocate 4kb memory for vmxon region */  

    vmxon_region = kzalloc(VMXON_REGION_PAGE_SIZE, GFP_KERNEL); 
    
    if(!vmxon_region)
    {
        printk(KERN_INFO "ERROR alloacating vmxon region\n"); 
        return false; 
    }

    /*convert virtual address to physical address */ 

    vmxon_phy_region = virt_to_phys(vmxon_region);

    /*write rivison id to first 32bit(4bytes) of vmxon region memory */ 
 
    *(uint32_t *)vmxon_region = vmcs_revision_id(); 

    if(_vmxon((uint64_t)vmxon_phy_region))
    {
        return false; 
    }
    return true;
}


bool vmcs_set(void)
{
    phys_addr_t vmcs_phy_region = 0; 

    /*allocate 4kb memory for vmcs region */ 

    if(!alloc_vmcs_region())
    {
        return false; 
    }
    
    vmcs_phy_region = virt_to_phys(vmcs_region); 

    /*insert revison identifier in first 32 bits of vmcs region*/ 

    *(uint32_t *) vmcs_region = vmcs_revision_id();

    if(_vmptrld((uint64_t)vmcs_phy_region))
    {
        return false;  
    }
    return true; 

}



int set_io_bitmap(void)
{
    phys_addr_t io_bitmap_phys; 

    /*alllocate 2kb pages for the io_bitmap */ 

    io_bitmap = (uint8_t *)__get_free_pages(GFP_KERNEL, VMCS_IO_BITMAP_PAGES_ORDER); 

    if(!io_bitmap)
    {
        printk(KERN_INFO "Failed to alllocate I/O bitmap memory\n"); 
        return -ENOMEM; 
    }

    /*clear entire io bitmap to 0 : allow i/o ports without vm exits */ 

    memset((void*)io_bitmap, 0, VMCS_IO_BITMAP_SIZE);

    io_bitmap_phys = virt_to_phys((void*)io_bitmap);

    printk(KERN_INFO "Allocated and cleared I/O bitmap at VA %p PA 0x%llx\n",
           (void*)io_bitmap, (unsigned long long)io_bitmap_phys); 

    if(_vmwrite(VMCS_IO_BITMAP_A, (uint64_t)io_bitmap_phys) != 0)
    {
        printk(KERN_INFO "VMWrite IO_BITMAP_A failed\n"); 
        free_pages((unsigned long)io_bitmap, VMCS_IO_BITMAP_PAGES_ORDER); 
        return -EIO;
    }

    if(_vmwrite(VMCS_IO_BITMAP_B, (uint64_t)io_bitmap_phys + VMCS_IO_BITMAP_PAGE_SIZE) != 0)
    {
        printk(KERN_INFO "VMWrite IO_BITMAP_B failed\n"); 
        free_pages((unsigned long)io_bitmap, VMCS_IO_BITMAP_PAGES_ORDER); 
        return -EIO; 
    }

    printk(KERN_INFO "VMCS I/O Bitmap field set successfully\n");
    return 0; 
}

void cleanup_io_bitmap(void)
{
    if(io_bitmap)
    {
        free_pages((unsigned long)io_bitmap, VMCS_IO_BITMAP_PAGES_ORDER); 
        io_bitmap = NULL; 
    }
}
 
int setup_cr_mask_and_shadows(void) 
{
    uint64_t real_cr0 = real_cr0(); 
    uint64_t real_cr4 = real_cr4(); 

    uint64_t cr0_mask = (1ULL << 31); 
    uint64_t cr4_mask = (1ULL << 20);

    /*set read shadow: fake CRO.PG = 0 and CR4.SMEP=0 for guest */
    uint64_t cr0_read_shadow = real_cr0 & ~(1ULL << 31); 
    uint64_t cr4_read_shadow = real_cr4 & ~(1ULL << 20); 


    CHECK_VMWRITE(VMCS_CR0_GUEST_HOST_MASK, cr0_mask); 
    CHECK_VMWRITE(VMCS_CR0_GUEST_READ_SHADOW, cr0_read_shadow); 

    CHECK_VMWRITE(VMCS_CR4_GUEST_HOST_MASK, cr0_mask); 
    CHECK_VMWRITE(VMCS_CR4_GUEST_READ_SHADOW, cr4_read_shadow); 

    return 0; 
}


/*
 * set_cr3_targets - Sets the CR3-target values in the VMCS.
 *
 * CR3-target values allow the guest to perform MOV to CR3 instructions
 * without causing a VM exit, but only if the value being written matches
 * one of these target values. This can reduce the number of VM exits
 * during guest context switches or address space changes.
 *
 * The Intel VMX architecture allows up to 4 CR3 target values.
 * If the guest writes any other CR3 value not in this list, a VM exit
 * will occur as usual.
 *
 * @targets: An array of 4 possible CR3 target values.
 *           (Unused slots should be set to 0.)
 */

int set_cr3_targets(uint64_t targets[4]) 
{
    uint64_t fields[4] = {

        VMCS_CR3_TARGET_VALUE0,
        VMCS_CR3_TARGET_VALUE1,
        VMCS_CR3_TARGET_VALUE2,
        VMCS_CR3_TARGET_VALUE3
    };

    for (int x = 0; x < 4; ++x)
    {
        CHECK_VMWRITE(fields[x], targeta[x]);
    }

    return 0; 
}

/*allocate and initialize a 4kb msr bitmap, return it's virtual address */ 

void * setup_msr_bitmap(void)
{
    msr_bitmap = kmalloc(4096, GFP_KERNEL | _GFP_ZERO); 

    if(!msr_bitmap)
    {
        printk(KERN_INFO "Failed to allocate MSR bitmap\n"); 
        return NULL; 
    }

    uint32_t msr_index = 0x174; 
    uint8_t *bitmap = (uint8_t *)msr_bitmap; 
    uint32_t byte = msr_index / 8; 
    uint8_t bit = msr_index % 8; 

    bitmap[byte] |= (1 << bit); 

    return msr_bitmap;
}

/*load v,cs bitmap into VMCS */ 

int load_msr_bitmap(void *msr_bitmap_virt_addr)
{
    uint64_t msr_bitmap_phys_addr; 

    msr_bitmap_phys_addr = virt_to_phys(msr_bitmap_virt_addr); 

    CHECK_VMWRITE(VMCS_MSR_BITMAP, msr_bitmap_phys_addr);

    return 0; 
}

void cleanup_msr_bitmap(void) 
{
    if (msr_bitmap) {
        kfree(msr_bitmap);
        msr_bitmap = NULL;
    }
}



/*set up vmcs execution control field */ 

bool init_vmcs_control_field(void) 
{
    /*setting pin based controls 
     * proc based controls
     * vm exit and entry controls */


      /* pin-based execution controls */ 

    uint64_t pinbased_control_msr = __rdmsr1(MSR_IA32_VMX_PINBASED_CTLS);
    uint32_t pin_allowed0 = (uint32_t)(pinbased_control_msr & 0xFFFFFFFF);
    uint32_t pin_allowed1 = (uint32_t)(pinbased_control_msr >> 32);
    uint32_t pinbased_control_desired = 0; // Specify your desired features here
    uint32_t pinbased_control_final = (pinbased_control_desired | pin_allowed1) & (pin_allowed0 | pin_allowed1);

    CHECK_VMWRITE(VMCS_PIN_BASED_EXEC_CONTROLS, pinbased_control_final);

    /* primary processor-based controls */ 

    uint64_t procbased_control_msr = __rdmsr1(MSR_IA32_VMX_PROCBASED_CTLS);
    uint32_t proc_allowed0 = (uint32_t)(procbased_control_msr & 0xFFFFFFFF);
    uint32_t proc_allowed1 = (uint32_t)(procbased_control_msr >> 32);
    uint32_t procbased_control_desired = 0; 
    uint32_t procbased_control_final = (procbased_control_desired | proc_allowed1) & (proc_allowed0 | proc_allowed1);
    CHECK_VMWRITE(VMCS_PROC_BASED_EXEC_CONTROLS, procbased_control_final);

    /* secondary processor-based controls */ 

    uint64_t procbased_secondary_control_msr = __rdmsr1(MSR_IA32_VMX_PROCBASED_CTLS2);
    uint32_t proc_secondary_allowed0 = (uint32_t)(procbased_secondary_control_msr & 0xFFFFFFFF);
    uint32_t proc_secondary_allowed1 = (uint32_t)(procbased_secondary_control_msr >> 32);
    uint32_t procbased_secondary_control_desired = 0;
    uint32_t procbased_secondary_control_final = (procbased_secondary_control_desired | proc_secondary_allowed1) & 
                                                 (proc_secondary_allowed0 | proc_secondary_allowed1);

    CHECK_VMWRITE(VMCS_PROC_BASED_EXEC_CONTROLS, procbased_secondary_control_final);

    /* vm-exit controls */

    uint64_t vm_exit_control_msr = __rdmsr1(MSR_IA32_VMX_EXIT_CTLS);
    uint32_t vm_exit_allowed0 = (uint32_t)(vm_exit_control_msr & 0xFFFFFFFF);
    uint32_t vm_exit_allowed1 = (uint32_t)(vm_exit_control_msr >> 32);
    uint32_t vm_exit_control_desired = 0; 
    uint32_t vm_exit_control_final = (vm_exit_control_desired | vm_exit_allowed1) & (vm_exit_allowed0 | vm_exit_allowed1)
    ;
    CHECK_VMWRITE(VMCS_EXIT_CONTROLS, vm_exit_control_final);

    /* vm-entry controls */ 

    uint64_t vm_entry_control_msr = __rdmsr1(MSR_IA32_VMX_ENTRY_CTLS);
    uint32_t vm_entry_allowed0 = (uint32_t)(vm_entry_control_msr & 0xFFFFFFFF);
    uint32_t vm_entry_allowed1 = (uint32_t)(vm_entry_control_msr >> 32);
    uint32_t vm_entry_control_desired = 0; 
    uint32_t vm_entry_control_final = (vm_entry_control_desired | vm_entry_allowed1) & (vm_entry_allowed0 | vm_entry_allowed1);

    CHECK_VMWRITE(VMCS_ENTRY_CONTROLS, vm_entry_control_final);

    /*set exception bitmap to 0 to ignore vmexit for any guest exception */ 

    uint32_t exception_bitmap = 0; 
    CHECK_VMWRITE(VMCS_EXCEPTION_BITMAP, exception_bitmap);

    printk(KERN_INFO "VMCS control fields set successfully\n");
    return 0; // Success

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
    cleanup_io_bitmap(); 
    printk(KERN_INFO "cleaning up I/O bitmap memory\n");

    cleanup_msr_bitmap(); 
    printk(KERN_INFO "cleaning up MSR bitmap\n")

    printk(KERN_INFO "Exiting hypervisor\n"); 
}

module_init(hyp_init); 
module_exit(hyp_exit); 

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Chrinovic M");
MODULE_DESCRIPTION("Lightweight Hypervisior ");
    
