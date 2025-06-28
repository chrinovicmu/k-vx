# Lightweight Hypervisor - Intel VT-x Implementation

## Overview

Minimal Type-2 hypervisor implemented as a Linux kernel module using Intel VMX technology. Demonstrates hardware-assisted virtualization with VMCS management, guest state control, and VM lifecycle operations.

## Architecture

### Core Components
- **VMX Operation Setup**: Hardware detection, CR4/MSR configuration, VMXON region initialization
- **VMCS Management**: 4KB control structure allocation and configuration
- **Execution Controls**: Pin-based, processor-based, and secondary controls configuration
- **I/O Virtualization**: 2KB bitmap for port access control (0-65535)
- **MSR Virtualization**: 4KB bitmap for model-specific register access control
- **Control Register Virtualization**: CR0/CR4 mask and shadow register management
- **Guest State Management**: Complete processor state for host/guest contexts
- **VM Entry/Exit Controls**: 64-bit mode enforcement and MSR store/load areas

### Memory Layout
```
VMXON Region:    4KB aligned, contains revision ID
VMCS Region:     4KB aligned, processor control structure
I/O Bitmap:      2KB, port access control (A: 0-7FFF, B: 8000-FFFF)
MSR Bitmap:      4KB, MSR access control for different ranges
MSR Store/Load:  Variable size arrays for VM entry/exit MSR handling
```

## System Requirements

### Hardware
- Intel processor with VMX support (CPUID.1:ECX.VMX[bit 5] = 1)
- VT-x enabled in BIOS/UEFI
- x86-64 architecture

### Software
- Linux kernel with module support
- Kernel headers and build tools
- Root privileges

## Build & Usage

```bash
# Build
make

# Load module
./init.sh 

# Unload
sudo rmmod hypervisor
```

## Implementation Features

- **VMX Support Detection**: CPUID-based hardware capability verification
- **MSR Configuration**: IA32_FEATURE_CONTROL and VMX capability MSRs
- **Fixed Bits Compliance**: CR0/CR4 adjustment per VMX_FIXED0/FIXED1
- **Exception Handling**: Configurable exception bitmap for VM exits
- **CR3 Target Values**: Up to 4 target values for MOV CR3 optimization
- **Guest Entry Point**: Custom RIP/RSP setup with stack alignment

## Error Handling

- Comprehensive VMWRITE/VMREAD validation macros
- Resource cleanup on failure paths
- Detailed kernel log messages for debugging
- Proper memory deallocation on module exit

## Limitations

- Single-core operation only
- Basic guest execution environment
- No EPT (Extended Page Tables) support
- No device emulation
- Educational/research purposes only

## Debug Output

```
VMX support present! continuing...
VMXON Region setup success
VMCS Region setup success
VMCS control fields set successfully
VM exit reason is [exit_code]
VMXOFF operation succeeded!
```

## License

Dual BSD/GPL

## Author

Chrinovic 
