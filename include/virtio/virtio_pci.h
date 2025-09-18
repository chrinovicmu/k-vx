#ifndef VIRTIO_PCI_H 
#define VIRTIO_PCI_H

#include <linux/types.h> 
#include <linux/byteorder/generic.h>  
#include <linux/virtio_pci.h> 
#include <linux/virtio.h> 
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

/* ISR status bit definations */ 

/*one or ore virtqueues have pending events, such as new buffers beign available for processing 
 * e.g device added buffers to used ring or driver added buffers to available ring */ 
#define ISR_STATUS_QUEUE_INTERRUPT      0x1

/* device configuration has changed, such as modifications to device_status or config_generation*/  
#define ISR_STATUS_DEVICE_CFG_INTERRUPT 0x2  

struct virtio_pci_cap
{
    u8      cap_vndr; /*Generic PCI field : vendor specific capability */ 
    u8      cap_next; /*Generic PCI field : link to next capability in capability list in pci config space */ 
    u8      cap_len; /*Generic PCI field length of while pci_cap struct*/ 
    u8      cfg_type; /* identify config structure */ 
    u8      bar; /*where to find it */ 
    u8      id; /*mulitple capabilities of the same type */ 
    u8      padding[2]; /*pad full dword */ 
    __le32  offset; /*offset within bar */ 
    __le32  length; /*length of the structure */ 
}__attribute__((packed)); 

struct virtio_pci_cap64
{
    struct virtio_pci_cap cap; 
    u32     offset_hi;
    u32     length_hi; 
};

struct virtio_pci_common_cfg 
{
    /*device*/
    __le32  device_feature_select; 
    __le32  device_feature; 
    __le32  driver_feature_select; 
    __le32  driver_feature; 
    __le16  config_msix_vector; 
    __le16  num_queues; 
    u8      device_status; 
    u8      config_generation; 

    /*specific virtqueue */  
    __le16  queue_select; 
    __le16  queue_size; 
    __le16  queue_msix_vector;
    __le16  queue_enable; 
    __le16  queue_notify_off; /* Notifications slot offset idx */ 
    __le64  queue_desc; 
    __le64  queue_driver;    /*addr driver area of avail ring */  
    __le64  queue_device;    /*addr device area of used ring */ 
    __le16  queue_notify_data; 
    __le16  queue_reset; 
} __attribute__((packed, aligned(4)));


struct virtio_pci_notify_cap 
{
    struct virtio_pci_cap cap; 

    /*multiplier used for each queue notifu register */ 
    __le32 notify_off_multiplier; 
}__attribute__((packed)); 

struct virtio_pci_isr_data 
{
    union {
        u32 isr_status; 
        struct {
            u8 queue_intr : 1;
            u8 config_intr : 1; 
            u8 reserved : 6;
            u device_specific[3]; 
        } __attribute__((packed)); 
    }; 
}; 

struct virtio_pci_cndr_data 
{
    u8 cap_vndr; 
    u8 cap_next; 
    u8 cap_len; 
    u8 cfg_type; 
    u16 vendor_id; 
}; 

struct virtio_pci_cfg_cap 
{
    struct virtio_pci_cap cap; 
    u8 pci_cfg_data[4]; 
}
struct virtio_pci_dev 
{
    struct virtio_device virtio_dev; 
    struct pci_dev *pdev;
    u64 device_features;    /*device-offered features */  
    u64 guest_features;     /*driver-accepted features */ 
    struct virtio_pci_common_cfg __iomem *common_cfg; 
    struct virtio_pci_notify_cap *notify_cap; 
    struct virtio_pci_cap cap; 
    void __iomem *isr_data;
    void __iomem *device_cfg; 
    struct virtqueue *virtqs;  
    int num_queues; 
}

static int virtio_pci_init(struct virtio_pci_dev *vpci_dev)
{

}
#endif // !VIRTIO_PCI_H 
