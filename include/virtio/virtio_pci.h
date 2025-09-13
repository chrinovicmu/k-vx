#ifndef VIRTIO_PCI_H 
#define VIRTIO_PCI_H

#include <linux/types.h> 
#include <linux/byteorder/generic.h>  
#include "util.h"

/*Common configuration */ 
#define VIRTIO_PCI_CAP_COMMON_CFG       1
/*Notifications*/ 
#define VIRTIO_PCI_CAP_NOTIFY_CFG       2  
/*ISR status */
#define VIRTIO_PCI_CAP_ISR_CFG          3 
/*Device specific configuration*/   
#define VIRTIO_PCI_CAP_DEVICE_CFG       4
/*PCI configuration acceess */ 
#define VIRTIO_PCI_CAP_PCI_CFG          5 
/*shared memory region */ 
#define VIRTIO_PCI_CAP_SHARED_MEMORY_CFG 8
/*Vendor-specific data */ 
#define VIRTIO_PCI_CAP_VENDOR_CFG       9 

struct virtio_pci_cap
{
    u8 cap_vndr; /*Generic PCI field : vendor specific capability */ 
    u8 cap_next; /*Generic PCI field : link to next capability in capability list in pci config space */ 
    u8 cap_len; /*Generic PCI field length of while pci_cap struct*/ 
    u8 cfg_type; /* identify config structure */ 
    u8 bar; /*where to find it */ 
    u8 id; /*mulitple capabilities of the same type */ 
    u8 padding[2]; /*pad full dword */ 
    __le32 0ffset; /*offset within bar */ 
    __le32 length; /*length of the structure */ 
}; 

struct virtio_pci_cap64
{
    struct virtio_pci_cap cap; 
    u32 offset_hi;
    u32 length_hi; 
};


#endif // !VIRTIO_PCI_H 
