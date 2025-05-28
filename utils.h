#ifdef UTILS_H 
#define UTILS_H 

#define X86_CR4_VMXE_BIT    13 
#define X86_CR4_VMXE        _BITUL(X86_CR4_VMXE_BIT)

#define _BITUL(x)           (1UL << (x))

/*enablling vmx through IA32_FEATURE_CONTROL_MSR */ 
#define IA32_FEATURE_CONTROL_LOCKED     (1 << 0)
#define IA32_FEATURE_CONTROL_MSR_VMXON_ENABLE_OUTSIDE_SMX (1 << 2)
#define MSR_IA32_FEATURE_CONTROL        0x0000003A 

/*for __rdmsr1 */ 
#define EAX_EDX_VAL(val, low, high) ((low) | (high) << 32)
#define EAX_EDX_RET(val, low, high) "=a" (low), "=d" (high)

#endif // UTILS_H 
#define UTILS_H 


