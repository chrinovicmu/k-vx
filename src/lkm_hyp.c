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
#include "vmcs_state.h"
#include "exit_code.h"
#include "macro.h"

static uint64_t vmxon_phy_addr = 0; 

int exit_msr_store_area(void); 
int entry_msr_load_area(void); 

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


static inline void enable_vmx_operation(void)
{
    uint64_t cr4;

    __asm__ volatile (
        "mov %%cr4, %0\n\t"
        "or %1, %0\n\t"
        "mov %0, %%cr4\n\t"
        : "=&r"(cr4)
        : "i"(1UL << 13)
        : "memory"
    );
}

/*check if processor supports MSR bitmap  */ 

bool msr_bitmap_support(void)
{
    uint64_t proc_ctls = __rdmsr1(MSR_IA32_VMX_PROCBASED_CTLS); 
    uint32_t allowed_1_settings = (uint32_t)(proc_ctls >> 32); 

    return (allowed_1_settings & (1 << VMCS_MSR_BITMAPS_BIT)) != 0;
}

int get_vmx_operation(void)
{
    unsigned long cr4;
    unsigned long cr0;

    uint64_t feature_control;
    uint64_t required;

    uint64_t vmx_cr0_fixed0;  
    uint64_t vmx_cr0_fixed1;
    uint64_t vmx_cr4_fixed0; 
    uint64_t vmx_cr4_fixed1; 
    
    /* enable VMX in CR4 (set bit 13) */

    asm volatile(
        "mov %%cr4, %0"
        : "=r" (cr4)
        : 
        : "memory" 
    );
    
    PDEBUG("Current CR4: 0x%lx\n", cr4);
    cr4 |= X86_CR4_VMXE;  /* Set VMX enable bit (bit 13) */
    printk(KERN_INFO "Setting CR4 to: 0x%lx\n", cr4);
    
    asm volatile(
        "mov %0, %%cr4"
        :
        : "r"(cr4)
        : "memory"
    );
    
    /*configure MSR_IA32_FEATURE_CONTROL MSR to allow VMXON */

    required = IA32_FEATURE_CONTROL_LOCKED;
    required |= IA32_FEATURE_CONTROL_MSR_VMXON_ENABLE_OUTSIDE_SMX;
    
    feature_control = __rdmsr1(MSR_IA32_FEATURE_CONTROL);
    PDEBUG("Current MSR_IA32_FEATURE_CONTROL: 0x%llx\n", feature_control);
    PDEBUG("Required bits: 0x%llx\n", required);
    
    /* check if the MSR is already locked with wrong settings */

    if ((feature_control & IA32_FEATURE_CONTROL_LOCKED) && ((feature_control & required) != required))
    {
        printk(KERN_ERR "Feature Control MSR is locked with incompatible settings!\n");
        printk(KERN_ERR "Current: 0x%llx, Required: 0x%llx\n", feature_control, required);
        return -1;
    }
    
    /* set the required bits if not already set */

    if ((feature_control & required) != required) 
    {   
        uint64_t new_feature_control = feature_control | required;
        uint32_t low = (uint32_t)(new_feature_control & 0xFFFFFFFF);
        uint32_t high = (uint32_t)((new_feature_control >> 32) & 0xFFFFFFFF);
        
        printk(KERN_INFO "Writing MSR_IA32_FEATURE_CONTROL: 0x%llx\n", new_feature_control);
        wrmsr(MSR_IA32_FEATURE_CONTROL, low, high);
        
        /* Verify the write succeeded */

        feature_control = __rdmsr1(MSR_IA32_FEATURE_CONTROL);
        PDEBUG("MSR after write: 0x%llx\n", feature_control);
        
        if ((feature_control & required) != required) 
        {
            printk(KERN_ERR "Failed to set required bits in Feature Control MSR\n");
            return -1;
        }
    }
    
    vmx_cr0_fixed0 = __rdmsr1(MSR_IA32_VMX_CR0_FIXED0);  
    vmx_cr0_fixed1 = __rdmsr1(MSR_IA32_VMX_CR0_FIXED1);  
    vmx_cr4_fixed0 = __rdmsr1(MSR_IA32_VMX_CR4_FIXED0);  
    vmx_cr4_fixed1 = __rdmsr1(MSR_IA32_VMX_CR4_FIXED1);  
    
    PDEBUG("VMX_CR0_FIXED0: 0x%llx (must be 1)\n", vmx_cr0_fixed0);
    PDEBUG("VMX_CR0_FIXED1: 0x%llx (must be 0 inverted)\n", vmx_cr0_fixed1);
    PDEBUG("VMX_CR4_FIXED0: 0x%llx (must be 1)\n", vmx_cr4_fixed0);
    PDEBUG("VMX_CR4_FIXED1: 0x%llx (must be 0 inverted)\n", vmx_cr4_fixed1);
    
    asm volatile (
        "mov %%cr0, %0"
        : "=r" (cr0)
        :
        : "memory"
    );
    
    PDEBUG("Current CR0: 0x%lx\n", cr0);
    
    cr0 |= vmx_cr0_fixed0;   
    cr0 &= vmx_cr0_fixed1;  
    
    PDEBUG("Setting CR0 to: 0x%lx\n", cr0);
    
    asm volatile (
        "mov %0, %%cr0"
        : 
        : "r" (cr0) 
        : "memory"
    );
    
    asm volatile (
        "mov %%cr4, %0"
        : "=r" (cr4)
        :
        : "memory"
    );
    
    PDEBUG("Current CR4 before VMX fixed adjustment: 0x%lx\n", cr4);
    
    cr4 |= vmx_cr4_fixed0;   
    cr4 &= vmx_cr4_fixed1;   
    
    cr4 |= X86_CR4_VMXE;
    
    PDEBUG("Setting final CR4 to: 0x%lx\n", cr4);
    
    asm volatile (
        "mov %0, %%cr4"
        :
        : "r"(cr4)
        : "memory"
    );
    
    vmxon_region = (void*) __get_free_pages(GFP_KERNEL, 0);

    if (!vmxon_region) {
        printk(KERN_ERR "Failed to allocate VMXON Region\n");
        return -1;
    }
    
    memset(vmxon_region, 0, VMXON_REGION_PAGE_SIZE);
    
    /* verify 4KB alignment - VMXON requires physical address to be 4KB aligned */

    if (((uint64_t)vmxon_region & 0xFFF) != 0) 
    {
        printk(KERN_ERR "VMXON region is NOT 4KB aligned! Address: %p\n", vmxon_region);
        free_pages((unsigned long)vmxon_region, 0);
        return -1;
    }
    
    printk(KERN_INFO "VMXON region allocated at: %p (4KB aligned)\n", vmxon_region);
    
    vmxon_phy_addr = virt_to_phys(vmxon_region);
    printk(KERN_INFO "VMXON physical address: 0x%llx\n", vmxon_phy_addr);
    
    uint32_t rev_id = _vmcs_revision_id();
    printk(KERN_INFO "VMCS Revision ID: 0x%x\n", rev_id);
    *(uint32_t *)vmxon_region = rev_id;
    
    printk(KERN_INFO "Attempting VMXON with physical address: 0x%llx\n", vmxon_phy_addr);
    
    if (_vmxon((uint64_t)vmxon_region) != 0)
    {
        printk(KERN_ERR "VMXON instruction failed\n");
        
        unsigned long current_cr0, current_cr4;

        __asm__ __volatile__ (
            "mov %%cr0, %0" 
            : "=r"(current_cr0)
        );

        __asm__ __volatile__ (
            "mov %%cr4, %0" 
            : "=r"(current_cr4)
        );
        
        printk(KERN_ERR "Debug - Current CR0: 0x%lx, CR4: 0x%lx\n", current_cr0, current_cr4);
        printk(KERN_ERR "Debug - VMX enabled in CR4: %s\n", 
               (current_cr4 & X86_CR4_VMXE) ? "YES" : "NO");
        
        free_pages((unsigned long)vmxon_region, 0);
        return -1;
    }
    
    printk(KERN_INFO "VMXON Region setup success - VMX operation enabled\n");

    return 0;
}



void cleanup_vmxon_region(void)
{
    if(vmxon_region)
    {
        printk(KERN_INFO "Cleaning up VMXON Region...\n"); 
        free_pages((unsigned long)vmxon_region, 0); 
        vmxon_region = NULL; 
    }
}

int vmcs_set(void)
{
    phys_addr_t vmcs_phy_region = 0; 

    /*allocate 4kb memory for vmcs region */ 
    
    vmcs_region = kzalloc(VMCS_REGION_PAGE_SIZE, GFP_KERNEL); 

    if(!vmcs_region)
    {
        printk(KERN_ERR "Failed to allocate VMCS Region\n"); 
        return -1; 
    }
    
    vmcs_phy_region = virt_to_phys(vmcs_region); 

    /*insert revison identifier in first 32 bits of vmcs region*/ 

    *(uint32_t *) vmcs_region = _vmcs_revision_id();

    if(_vmptrld((uint64_t)vmcs_phy_region) != 0)
    {
        return -1;  
    }

    printk(KERN_INFO "VMCS Region setup success\n");

    return 0; 

}

void cleanup_vmcs_region(void)
{
    if(vmcs_region)
    {
        printk(KERN_INFO "Cleaning up VMCS Region...\n"); 
        kfree(vmcs_region); 
        vmcs_region = NULL; 
    }
}


int init_vmcs_vm_execution_controls(void)
{
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

    printk(KERN_INFO "VMCS VM-execution control fields setup success\n");

    return 0;
}


int init_vmcs_vm_entry_exit_controls(void)
{
    /* vm-exit controls */

    uint64_t vm_exit_control_msr = __rdmsr1(MSR_IA32_VMX_EXIT_CTLS);
    uint32_t vm_exit_allowed0 = (uint32_t)(vm_exit_control_msr & 0xFFFFFFFF);
    uint32_t vm_exit_allowed1 = (uint32_t)(vm_exit_control_msr >> 32);
    uint32_t vm_exit_control_desired = 0; 
    uint32_t vm_exit_control_final = (vm_exit_control_desired | vm_exit_allowed1) & (vm_exit_allowed0 | vm_exit_allowed1);
    CHECK_VMWRITE(VMCS_EXIT_CONTROLS, vm_exit_control_final);

    /* vm-entry controls */ 

    uint64_t vm_entry_control_msr = __rdmsr1(MSR_IA32_VMX_ENTRY_CTLS);
    uint32_t vm_entry_allowed0 = (uint32_t)(vm_entry_control_msr & 0xFFFFFFFF);
    uint32_t vm_entry_allowed1 = (uint32_t)(vm_entry_control_msr >> 32);
    uint32_t vm_entry_control_desired = 0; 
    uint32_t vm_entry_control_final = (vm_entry_control_desired | vm_entry_allowed1) & (vm_entry_allowed0 | vm_entry_allowed1);

    CHECK_VMWRITE(VMCS_ENTRY_CONTROLS, vm_entry_control_final);


    /*set vm-exit controls 
     * VM_EXIT_HOST_ADDR_SPACE_SIZE : control bit 9 of VM_EXIT_CONTROLS, ensure hypervisor runs in long mode on exits*/ 

    CHECK_VMWRITE(VMCS_EXIT_CONTROLS, __rdmsr1(MSR_IA32_VMX_EXIT_CTLS) | VM_EXIT_HOST_ADDR_SPACE_SIZE); 

    /*set exit msr store area */ 

    if(exit_msr_store_area() != 0)
    {
        printk(KERN_ERR "VM-exit MSR VMCS setup failed\n"); 
        return -1; 
    }

    /*set vm-entry controls 
     * VM_ENTRY_IA32E_MODE : control bit 9 of VMCS_ENTRY_CONTROLS, ensures guest runs in long mode on entry*/ 

    CHECK_VMWRITE(VMCS_ENTRY_CONTROLS, __rdmsr1(MSR_IA32_VMX_ENTRY_CTLS | VM_ENTRY_IA32E_MODE)); 

    /* set entry msr store area */ 

    if(entry_msr_load_area() != 0)
    {
        printk(KERN_ERR "vm-entry MSR VMCS setup failed\n");
        return -1; 
    }
    
    printk(KERN_INFO "VMCS Exit and Entry control field setup success\n");

    return 0;
}

int init_vmcs_io_bitmap(void)
{
    phys_addr_t io_bitmap_phys; 

    /*alllocate 2kb pages for the io_bitmap */ 

    io_bitmap = (uint8_t *)__get_free_pages(GFP_KERNEL, VMCS_IO_BITMAP_PAGES_ORDER); 

    if(!io_bitmap)
    {
        printk(KERN_ERR "Failed to alllocate I/O bitmap memory\n"); 
        return -ENOMEM; 
    }

    /*clear entire io bitmap to 0 : allow i/o ports without vm exits */ 

    memset((void*)io_bitmap, 0, VMCS_IO_BITMAP_SIZE);

    io_bitmap_phys = virt_to_phys((void*)io_bitmap);

    printk(KERN_INFO "Allocated and cleared I/O bitmap at VA %p PA 0x%llx\n",
           (void*)io_bitmap, (unsigned long long)io_bitmap_phys); 

    if(_vmwrite(VMCS_IO_BITMAP_A, (uint64_t)io_bitmap_phys) != 0)
    {
        printk(KERN_ERR "VMWrite IO_BITMAP_A failed\n"); 
        free_pages((unsigned long)io_bitmap, VMCS_IO_BITMAP_PAGES_ORDER); 
        return -EIO;
    }

    if(_vmwrite(VMCS_IO_BITMAP_B, (uint64_t)io_bitmap_phys + VMCS_IO_BITMAP_PAGE_SIZE) != 0)
    {
        printk(KERN_ERR "VMWrite IO_BITMAP_B failed\n"); 
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
        printk(KERN_INFO "Cleaning up I/O bitmap memory...\n");
        free_pages((unsigned long)io_bitmap, VMCS_IO_BITMAP_PAGES_ORDER); 
        io_bitmap = NULL; 
    }
}
 
int init_vmcs_cr_mask_and_shadows(void) 
{
    uint64_t real_cr0 = _read_cr0(); 
    uint64_t real_cr4 = _read_cr4(); 

    uint64_t cr0_mask = (1ULL << 31); 
    uint64_t cr4_mask = (1ULL << 20);

    /*set read shadow: fake CRO.PG = 0 and CR4.SMEP=0 for guest */
    uint64_t cr0_read_shadow = real_cr0 & ~(1ULL << 31); 
    uint64_t cr4_read_shadow = real_cr4 & ~(1ULL << 20); 


    CHECK_VMWRITE(VMCS_CR0_GUEST_HOST_MASK, cr0_mask); 
    CHECK_VMWRITE(VMCS_CR0_READ_SHADOW, cr0_read_shadow); 

    CHECK_VMWRITE(VMCS_CR4_GUEST_HOST_MASK, cr4_mask); 
    CHECK_VMWRITE(VMCS_CR4_READ_SHADOW, cr4_read_shadow);

    printk(KERN_INFO "VMCS CR4 guest host mask and read shadow setup success\n"); 

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

int init_vmcs_cr3_targets(void) 
{
    
   uint64_t targets[4] = {

        0x0000000000200000,
        0x0000000000300000,
        0x0000000000400000,
        0x0000000000500000
    };

    uint64_t fields[4] = {

        VMCS_CR3_TARGET_VALUE0,
        VMCS_CR3_TARGET_VALUE1,
        VMCS_CR3_TARGET_VALUE2,
        VMCS_CR3_TARGET_VALUE3
    };

    for (int x = 0; x < 4; ++x)
    {
        CHECK_VMWRITE(fields[x], targets[x]);
    }

    printk(KERN_INFO "CR3 targets setup success\n"); 
    return 0; 
}

/*allocate and initialize a 4kb msr bitmap, return it's virtual address */ 

void * setup_msr_bitmap(void)
{
    msr_bitmap = kmalloc(4096, GFP_KERNEL | __GFP_ZERO); 

    if(!msr_bitmap)
    {
        printk(KERN_ERR "Failed to allocate MSR bitmap\n"); 
        return NULL; 
    }

    uint32_t msr_index = IA32_SYSENTER_CS; 
    uint8_t *bitmap = (uint8_t *)msr_bitmap; 
    uint32_t byte = msr_index / 8; 
    uint8_t bit = msr_index % 8; 

    bitmap[byte] |= (1 << bit); 

    return msr_bitmap;
}

/*load v,cs bitmap into VMCS */ 

int init_vmcs_msr_bitmap(void *msr_bitmap_virt_addr)
{
    uint64_t msr_bitmap_phys_addr; 

    msr_bitmap_phys_addr = virt_to_phys(msr_bitmap_virt_addr); 

    CHECK_VMWRITE(VMCS_MSR_BITMAP, msr_bitmap_phys_addr);

    printk(KERN_INFO "MSR Bitmap setup success\n"); 
    return 0;
}

void cleanup_msr_bitmap(void) 
{
    if (msr_bitmap) 
    {
        printk(KERN_INFO "Cleaning up MSR Bitmap memory...\n"); 
        kfree(msr_bitmap);
        msr_bitmap = NULL;
    }
}


int exit_msr_store_area(void)
{
    phys_addr_t exit_phys_addr;

    _vm_exit_msr_area = kzalloc(sizeof(struct _msr_entry) * MSR_AREA_ENTRIES, GFP_KERNEL | GFP_DMA); 

    if(!_vm_exit_msr_area)
    {
        printk(KERN_ERR "Failed to allocate Exit MSR area memory region\n");
        return - ENOMEM; 
    }

    _vm_exit_msr_area[0].index = IA32_SYSENTER_CS; 
    _vm_exit_msr_area[0].reserved = 0;

    exit_phys_addr = virt_to_phys(_vm_exit_msr_area); 

    CHECK_VMWRITE(VM_EXIT_MSR_STORE_COUNT, MSR_AREA_ENTRIES); 
    CHECK_VMWRITE(VM_EXIT_MSR_STORE_ADDR, exit_phys_addr); 

    printk(KERN_INFO "VM Exit MSR setup success!\n");
    return 0; 
}

void cleanup_exit_msr_area(void)
{
    if(_vm_exit_msr_area)
    {
        printk(KERN_INFO "Cleaning up Exit MSR area memory...\n"); 
        kfree(_vm_exit_msr_area); 
        _vm_exit_msr_area = NULL;
    }
}

int entry_msr_load_area(void)
{
    phys_addr_t entry_phys_addr; 

    _vm_entry_msr_area  = kzalloc(sizeof(struct _msr_entry) * MSR_AREA_ENTRIES, GFP_KERNEL | GFP_DMA ); 

    if(!_vm_exit_msr_area)
    {
        printk(KERN_INFO "Failed to allocate Entry MSR area memory region\n"); 
        return -ENOMEM; 
    }

    _vm_entry_msr_area[0].index = 0x174; 
    _vm_entry_msr_area[0].reserved = 0; 

    entry_phys_addr = virt_to_phys(_vm_entry_msr_area); 
     
    CHECK_VMWRITE(VM_ENTRY_MSR_LOAD_COUNT, MSR_AREA_ENTRIES); 
    CHECK_VMWRITE(VM_ENTRY_MSR_LOAD_ADDR, entry_phys_addr); 

    printk(KERN_INFO "VM Entry MSR setup success!\n");

    return 0; 
}

void cleanup_entry_msr_area(void)
{
    if(_vm_entry_msr_area)
    {
        printk(KERN_INFO "Cleaning up Entry MSR area memory\n"); 
        kfree(_vm_entry_msr_area); 
        _vm_entry_msr_area = NULL; 
    }
}

int init_vmcs_host_guest_states(void)
{
    CHECK_VMWRITE(HOST_CR0, _read_cr0());
    CHECK_VMWRITE(HOST_CR3, _read_cr3());
    CHECK_VMWRITE(HOST_CR4, _read_cr4()); 
    
    CHECK_VMWRITE(HOST_IA32_SYSENTER_ESP, __rdmsr1(MSR_IA32_SYSENTER_ESP));
    CHECK_VMWRITE(HOST_IA32_SYSENTER_EIP, __rdmsr1(MSR_IA32_SYSENTER_EIP));
    CHECK_VMWRITE(HOST_IA32_SYSENTER_CS,  __rdmsr(MSR_IA32_SYSENTER_CS));
        
    uint64_t tmp;

    CHECK_VMREAD(HOST_ES_SELECTOR, tmp);
    CHECK_VMWRITE(GUEST_ES_SELECTOR, tmp);

    CHECK_VMREAD(HOST_CS_SELECTOR, tmp);
    CHECK_VMWRITE(GUEST_CS_SELECTOR, tmp);

    CHECK_VMREAD(HOST_SS_SELECTOR, tmp);
    CHECK_VMWRITE(GUEST_SS_SELECTOR, tmp);

    CHECK_VMREAD(HOST_DS_SELECTOR, tmp);
    CHECK_VMWRITE(GUEST_DS_SELECTOR, tmp);

    CHECK_VMREAD(HOST_FS_SELECTOR, tmp);
    CHECK_VMWRITE(GUEST_FS_SELECTOR, tmp);

    CHECK_VMREAD(HOST_GS_SELECTOR, tmp);
    CHECK_VMWRITE(GUEST_GS_SELECTOR, tmp);

    CHECK_VMWRITE(GUEST_LDTR_SELECTOR, 0);

    CHECK_VMREAD(HOST_TR_SELECTOR, tmp);
    CHECK_VMWRITE(GUEST_TR_SELECTOR, tmp);

    CHECK_VMWRITE(GUEST_ES_LIMIT, -1);
    CHECK_VMWRITE(GUEST_CS_LIMIT, -1);
    CHECK_VMWRITE(GUEST_SS_LIMIT, -1);
    CHECK_VMWRITE(GUEST_DS_LIMIT, -1);
    CHECK_VMWRITE(GUEST_FS_LIMIT, -1);
    CHECK_VMWRITE(GUEST_GS_LIMIT, -1);
    CHECK_VMWRITE(GUEST_LDTR_LIMIT, -1);
    CHECK_VMWRITE(GUEST_TR_LIMIT, 0x67);
    CHECK_VMWRITE(GUEST_GDTR_LIMIT, 0xffff);
    CHECK_VMWRITE(GUEST_IDTR_LIMIT, 0xffff);
    
    /* AR bytes section */ 

    CHECK_VMREAD(GUEST_ES_SELECTOR, tmp);
    CHECK_VMWRITE(GUEST_ES_AR_BYTES, tmp == 0 ? 0x10000 : 0xc093);

    CHECK_VMWRITE(GUEST_CS_AR_BYTES, 0xa09b);
    CHECK_VMWRITE(GUEST_SS_AR_BYTES, 0xc093);

    CHECK_VMREAD(GUEST_DS_SELECTOR, tmp);
    CHECK_VMWRITE(GUEST_DS_AR_BYTES, tmp == 0 ? 0x10000 : 0xc093);

    CHECK_VMREAD(GUEST_FS_SELECTOR, tmp);
    CHECK_VMWRITE(GUEST_FS_AR_BYTES, tmp == 0 ? 0x10000 : 0xc093);

    CHECK_VMREAD(GUEST_GS_SELECTOR, tmp);
    CHECK_VMWRITE(GUEST_GS_AR_BYTES, tmp == 0 ? 0x10000 : 0xc093);

    CHECK_VMWRITE(GUEST_LDTR_AR_BYTES, 0x10000);
    CHECK_VMWRITE(GUEST_TR_AR_BYTES, 0x8b);

    /* BASE fields */

    CHECK_VMWRITE(GUEST_ES_BASE, 0);
    CHECK_VMWRITE(GUEST_CS_BASE, 0);
    CHECK_VMWRITE(GUEST_SS_BASE, 0);
    CHECK_VMWRITE(GUEST_DS_BASE, 0);

    CHECK_VMREAD(HOST_FS_BASE, tmp);
    CHECK_VMWRITE(GUEST_FS_BASE, tmp);

    CHECK_VMREAD(HOST_GS_BASE, tmp);
    CHECK_VMWRITE(GUEST_GS_BASE, tmp);

    CHECK_VMWRITE(GUEST_LDTR_BASE, 0);

    CHECK_VMREAD(HOST_TR_BASE, tmp);
    CHECK_VMWRITE(GUEST_TR_BASE, tmp);

    CHECK_VMREAD(HOST_GDTR_BASE, tmp);
    CHECK_VMWRITE(GUEST_GDTR_BASE, tmp);

    CHECK_VMREAD(HOST_IDTR_BASE, tmp);
    CHECK_VMWRITE(GUEST_IDTR_BASE, tmp);

    /* set the required MSR */ 

    CHECK_VMWRITE(GUEST_IA32_DEBUGCTL, 0);

    CHECK_VMREAD(HOST_IA32_PAT, tmp);
    CHECK_VMWRITE(GUEST_IA32_PAT, tmp);

    CHECK_VMREAD(HOST_IA32_EFER, tmp);
    CHECK_VMWRITE(GUEST_IA32_EFER, tmp);

    CHECK_VMREAD(HOST_IA32_PERF_GLOBAL_CTRL, tmp);
    CHECK_VMWRITE(GUEST_IA32_PERF_GLOBAL_CTRL, tmp);

    CHECK_VMREAD(HOST_IA32_SYSENTER_CS, tmp);
    CHECK_VMWRITE(GUEST_SYSENTER_CS, tmp);

    CHECK_VMREAD(HOST_IA32_SYSENTER_ESP, tmp);
    CHECK_VMWRITE(GUEST_SYSENTER_ESP, tmp);

    CHECK_VMREAD(HOST_IA32_SYSENTER_EIP, tmp);
    CHECK_VMWRITE(GUEST_SYSENTER_EIP, tmp);

    /*set guest non-register state */ 

    CHECK_VMWRITE(GUEST_ACTIVITY_STATE, 0);
    CHECK_VMWRITE(VMCS_LINK_POINTER, -1ull);
    CHECK_VMWRITE(VMX_PREEMPTION_TIMER_VALUE, 0);
    CHECK_VMWRITE(GUEST_INTR_STATUS, 0);
    CHECK_VMWRITE(GUEST_PML_INDEX, 0);

    printk(KERN_INFO "VM Host Guest register state setup success\n");

    return 0; 
}

int setup_guest_entry_point(void) 
{
    void *costum_rsp;
    void *costum_rip; 

    /*custum rsp points to top of stack 
     * align the stack pointer to 16 bytes (required by x86-64 ABI)*/ 
    costum_rsp = (void *)(((uintptr_t)&guest_stack[GUEST_STACK_SIZE]) & ~0xFULL);
    costum_rip = guest_code; 
    CHECK_VMWRITE(GUEST_RSP, (uint64_t)costum_rsp);
    CHECK_VMWRITE(GUEST_RIP, (uint64_t)costum_rip);

    return 0;
}


/*set up vmcs execution control field */ 

bool init_vmcs_control_field(void) 
{

    if(!init_vmcs_vm_execution_controls())
    {
        printk(KERN_ERR "VMCS VM-exection controls field setup failed"); 
        return -1; 
    }

    if(!init_vmcs_vm_entry_exit_controls())
    {
        printk(KERN_ERR "VMCS VM-Exit VM-Entry controls field setup failed\n");
        return -1; 
    }

    /*set exception bitmap to 0 to ignore vmexit for any guest exception */ 

    uint32_t exception_bitmap = 0; 
    CHECK_VMWRITE(VMCS_EXCEPTION_BITMAP, exception_bitmap);

    if(init_vmcs_io_bitmap() != 0)
    {
        printk(KERN_ERR "VMCS I/O Bitmap setup failed\n"); 
        return -1; 
    }
    
    if(init_vmcs_cr_mask_and_shadows() != 0)
    {
        printk(KERN_ERR "VMCS CR mask and shadow setup failed\n"); 
        return -1; 
    }

    if(init_vmcs_cr3_targets() != 0)
    {
        printk(KERN_ERR "VMCS CR3 targets setup failed\n"); 
        return -1; 
    }

    if(msr_bitmap_support())
    {
        void * msr_bitmap = setup_msr_bitmap(); 
        if(init_vmcs_msr_bitmap(msr_bitmap) != 0)
        {
            printk(KERN_ERR "VMCS MSR bitmap setup failed\n"); 
            return -1; 
        }
    }

    if(init_vmcs_host_guest_states() != 0)
    {
        printk(KERN_ERR "VMCS Host Guest states fields setup failed\n"); 
        return -1; 
    }

    if(setup_guest_entry_point())
    {
        printk(KERN_ERR "VMCS guest entry point setup failed\n"); 
        return -1; 
    }


    printk(KERN_INFO "VMCS control fields set successfully\n");
    return 0; // Success

}

uint32_t vmexit_res(void)
{
    uint32_t exit_reason; 

    CHECK_VMREAD(VM_EXIT_REASON,exit_reason);
    exit_reason = exit_reason & 0xffff;

    return exit_reason; 
}

int init_vmlaunch(void)
{
    int vmlaunch_status = _vmlaunch(); 

    if(vmlaunch_status != 0)
    {
        printk(KERN_ERR "VM launch failed with error code : 0x%x\n", vmlaunch_status);
        return -1; 
    }

    printk(KERN_ERR "VM exit reason is %lu!\n", (unsigned long)vmexit_res());

    return 0; 
}

int vmx_off(void)
{
    __asm__ __volatile__ (
        "vmxoff\n"
        : 
        : 
        : "cc"  
    );

    return 0; 
}


static int __init hyp_init(void)
{
    if(!vmx_support()) 
    {
        printk(KERN_INFO "VMX support not present\n");
        return -EOPNOTSUPP;
    }
    printk(KERN_INFO "VMX support present! continuing...\n");

    enable_vmx_operation();

    if (get_vmx_operation() != 0)
    {
        printk(KERN_ERR "VMX operation failed! exiting...\n");
        return -EIO;
    }

    if (vmcs_set() != 0)
    {
        printk(KERN_ERR "VMCS Region allocation failed! exiting...\n");
        cleanup_vmxon_region();
        return -ENOMEM;
    }

    if (init_vmcs_control_field() != 0) 
    {
        printk(KERN_ERR "VMCS control field initialization failed! exiting...\n");
        cleanup_vmcs_region();
        cleanup_vmxon_region();
        return -EINVAL;
    }

    if (init_vmlaunch() != 0) 
    {
        printk(KERN_ERR "VMLAUNCH failed! exiting...\n");
        cleanup_vmcs_region();
        cleanup_vmxon_region();
        return -EIO;
    }

    if (vmx_off() != 0)
    {
        printk(KERN_ERR "VMXOFF operation failed! exiting...\n");
        return -EIO;
    }

    printk(KERN_INFO "VMXOFF operation succeeded! continuing...\n");
    return 0;
}

static void __exit hyp_exit(void)
{
    printk(KERN_INFO "Cleaning all resources used:\n");

    cleanup_vmxon_region();
    cleanup_vmcs_region();
    cleanup_io_bitmap();
    cleanup_msr_bitmap();
    cleanup_exit_msr_area();
    cleanup_entry_msr_area();

    printk(KERN_INFO "Resources cleanup completed!\n");
    printk(KERN_INFO "Exiting hypervisor...\n");

    asm volatile("vmxoff");
}

module_init(hyp_init); 
module_exit(hyp_exit); 

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Chrinovic M");
MODULE_DESCRIPTION("Lightweight Hypervisior ");
    

