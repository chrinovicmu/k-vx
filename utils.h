#ifdef UTILS_H 
#define UTILS_H 

#define X86_CR4_VMXE_BIT    13 
#define X86_CR4_VMXE        _BITUL(X86_CR4_VMXE_BIT)

#define _BITUL(x)           (1UL << (x))

/*enablling vmx through IA32_FEATURE_CONTROL_MSR */ 
#define IA32_FEATURE_CONTROL_LOCKED     (1 << 0)
#define IA32_FEATURE_CONTROL_MSR_VMXON_ENABLE_OUTSIDE_SMX (1 << 2)
#define MSR_IA32_FEATURE_CONTROL        0x0000003A 


#define MSR_IA32_VMX_CR0_FIXED0         0x00000486 
#define MSR_IA32_VMX_CR0_FIXED1         0x00000487 
#define MSR_IA32_VMX_CR4_FIXED0         0x00000488 
#define MSR_IA32_VMX_CR4_FIXED1         0x00000489 



#endif // UTILS_H 
#define UTILS_H 


