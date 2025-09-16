#include <linux/module.h>
#include <linux/pci.h>
#include <linux/virtio_config.h>
#include "virtio/virtio_pci.h"

/* Offsets for fields in struct virtio_pci_cap */
#define VIRTIO_PCI_CAP_VNDR_OFFSET      0  /* cap_vndr, cap_next, cap_len, cfg_type */
#define VIRTIO_PCI_CAP_BAR_OFFSET       4  /* bar, id, padding[2] */
#define VIRTIO_PCI_CAP_OFFSET_OFFSET    8  /* offset */
#define VIRTIO_PCI_CAP_LENGTH_OFFSET   12  /* length */

static const struct pci_device_id virtio_pci_id_table[] = {
    {PCI_DEVICE(0x1AF4, PCI_ANY_ID)}, 
    {0}
};
MODULE_DEVICE_TABLE(pci, virtio_pci_id_table); 
static int virtio_pci_map_common_cfg(struct virtio_pci_dev *vpci_dev, u8 pos)
{
    struct pci_dev *pdev = vpci_dev->pdev; 
    struct virtio_pci_cap cap = {0}; 
    void __iomem *base;   
    int ret; 

    ret = pci_read_config_dword(pdev, pos + VIRTIO_PCI_CAP_VNDR_OFFSET, (u32 *)&cap);
    if (ret) {
        dev_err(&pdev->dev, "Failed to read capability at 0x%x (vendor fields)\n",
                pos + VIRTIO_PCI_CAP_VNDR_OFFSET);
        return ret;
    }
    ret = pci_read_config_dword(pdev, pos + VIRTIO_PCI_CAP_BAR_OFFSET, (u32 *)&cap.bar);
    if (ret) {
        dev_err(&pdev->dev, "Failed to read capability at 0x%x (BAR fields)\n",
                pos + VIRTIO_PCI_CAP_BAR_OFFSET);
        return ret;
    }
    ret = pci_read_config_dword(pdev, pos + VIRTIO_PCI_CAP_OFFSET_OFFSET, &cap.offset);
    if (ret) {
        dev_err(&pdev->dev, "Failed to read capability at 0x%x (offset field)\n",
                pos + VIRTIO_PCI_CAP_OFFSET_OFFSET);
        return ret;
    }
    ret = pci_read_config_dword(pdev, pos + VIRTIO_PCI_CAP_LENGTH_OFFSET, &cap.length);
    if (ret) {
        dev_err(&pdev->dev, "Failed to read capability at 0x%x (length field)\n",
                pos + VIRTIO_PCI_CAP_LENGTH_OFFSET);
        return ret;
    }
    if(cap.cfg_type != VIRTIO_PCI_CAP_COMMON_CFG) 
    {
        dev_err(&pdev, "Expected common cfg capablilty at 0x%x, got type %d\n", pos, cap.cfg_type); 
        return -EINVAL; 
    }
    if(cap.bar >= PCI_STD_NUM_BARS)
    {
        dev_err(&pdev->dev, "Invalid Bar index %d for commong cfg capablilty\n", cap.bar); 
        return -EINVAL; 
    }
    if (cap.length < sizeof(struct virtio_pci_common_cfg)) {
        dev_err(&pdev->dev, "Common cfg capability length %d too small\n", cap.length);
        return -EINVAL;
    }
    base = pci_ioremap_bar(pdev, cap.bar);
    if (!base)
    {
        dev_err(&pdev->dev, "Failed to map BAR %d for common cfg\n", cap.bar);
        return -ENOMEM;
    }
    vpci_dev->common_cfg = base + cap.offset; 
    if(ioread8(&vpci_dev->common_cfg->device_status) == 0xFF)
    {
        dev_err(&pdev->dev, "Common cfg region at BAR %d Offset 0x%x is invaild\n"
                cap.bar, cap.offset); 
        iounmap(base); 
        vpci_dev->common_cfg = NULL; 
        return -EIO; 
    }
}
static int virtio_pci_find_caps(struct virtio_pci_dev *vpci_dev)
{
    struct pci_dev *pdev = vpci_dev->pdev; 
    u8 pos; 
    int ret; 

    /*PCI_CAP_ID_VNDR (0x09)- indiactes vendor specofoc capablilty
     * returns offset if the capablilty in the configuration space */
    pos = pci_find_capability(pdev, PCI_CAP_ID_VNDR); 
    if(!pos)
    {
        dev_err(&pdev->dev, "No vendor-specific capablilty found\n");
         return -EINVAL; 
    }

    while(pos)
    {
        struct virtio_pci_cap cap;
        u8 cfg_type; 

        /*read config space at POS, and store into cap */ 
        ret = pci_read_config_dword(pdev, pos, (u32 *)&cap); 
        if(ret)
        {
            dev_err(&pdev->dev, "Failed to read capability at 0x%x\n", pos);
            return ret;
        }

        cfg_type = cap.cfg_type; 

        swith(cfg_type)
        {
            case VIRTIO_PCI_CAP_COMMON_CFG:
                
                ret = 
        }
    }

}

static int virtio_pci_probe(struct pci_dev *pdev, const struct pci_device_id)
{
    struct virtio_pci_dev *vpci_dev; 
    int ret; 

    vpci_dev = kzalloc(sizeof(struct virtio_pci_dev), GFP_KERNEL);
    if(!vpci_dev)
    {
        dev_err(&pdev->dev, "Failed to allocate memory for virtio_pci_dev\n"); 
        return -ENOMEN; 
    }

    vpci_dev->pdev = pdev; 
    
    /*enable pci device */ 
    ret = pci_enable(pdev); 
    if(ret)
    {
        dev_err(&pdev->dev, "Failed to enable PCI device\n"); 
        goto err_disable_dev; 
    }

    /*request pci regions */ 
    ret = pci_request_regions(pdev, "virtio_pci");
    if(ret)
    {
        dev_err(&pdev->dev, "failed to request PCI regions\n"); 
        goto err_disable_dev; 
    }

}



