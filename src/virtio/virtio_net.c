#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/virtio.h>
#include <linux/virtio_net.h>
#include <linux/scatterlist.h>
#include "virtio_net.h"
#include "virtqueue.h"

static int virtio_net_open(struct net_device *dev)
{
    /*get private data attahced to net_device */ 
    struct virtio_net_dev *vnet_dev = netdev_priv(dev);
    struct virtio_pci_dev *vpci_dev = vnet_dev->vpci_dev; 

    /*select queue and enable */ 

    /*RX queue*/ 
    iowrite16(VIRTIO_NET_QUEUE_RX, &vpci_dev->common_cfg->queue_select);
    iowrite16(VIRTIO_VIRTQUEUE_ENABLE, &vpci_dev->common_cfg->queue_enable); 
    
    /*TX queue*/ 
    iowrite16(VIRTIO_NET_QUEUE_TX, &vpci_dev->common_cfg->queue_select); 
    iowrite16(VIRTIO_VIRTQUEUE_ENABLE, &vpci_dev->common_cfg->queue_enable); 

    /*enable CTRL queue (if it exists)*/ 
    if(vpci_dev->virtqs[VIRTIO_NET_QUEUE_CTRL])
    {
        iowrite16(VIRTIO_NET_QUEUE_CTRL, &vpci_dev->common_cfg->queue_select); 
        iowrite16(VIRTIO_VIRTQUEUE_ENABLE, &vpci_dev->common_cfg->queue_enable); 
    }

    netif_start_queue(dev); 
    return 0;
}

static int virtio_net_stop(struct net_device *dev)
{ 
    /*get private data attahced to net_device */ 
    struct virtio_net_dev *vnet_dev = netdev_priv(dev);
    struct virtio_pci_dev *vpci_dev = vnet_dev->vpci_dev; 

    /*select queue and enable */ 

    /*RX queue*/ 
    iowrite16(VIRTIO_NET_QUEUE_RX, &vpci_dev->common_cfg->queue_select);
    iowrite16(VIRTIO_VIRTQUEUE_DISABLE, &vpci_dev->common_cfg->queue_enable); 
    
    /*TX queue*/ 
    iowrite16(VIRTIO_NET_QUEUE_TX, &vpci_dev->common_cfg->queue_select); 
    iowrite16(VIRTIO_VIRTQUEUE_DISABLE, &vpci_dev->common_cfg->queue_enable); 

    /*enable CTRL queue (if it exists)*/ 
    if(vpci_dev->virtq[VIRTIO_NET_QUEUE_CTRL])
    {
        iowrite16(VIRTIO_NET_QUEUE_CTRL, &vpci_dev->common_cfg->queue_select); 
        iowrite16(VIRTIO_VIRTQUEUE_DISABLE, &vpci_dev->common_cfg->queue_enable); 
    }

    netif_stop_queue(dev); 
    return 0;  
}

static netdev_tx_t virtio_net_xmit(struct sk_buff *skb, struct net_device *dev) 
{
    int sg_entries_len = 1; 

    struct virtio_net_dev *vnet_dev = netdev_priv(dev); 
    struct virtio_pci_dev *vpci_dev = vnet_dev->vpci_dev; 
    struct virtqueue *vq = vpci_dev->virtqs[VIRTIO_NET_QUEUE_TX]; /*transimit virtqueue */ 
    struct scatterlist sg[sg_entries_len];  
    int ret; 

    sg_init_one(sg, skb->data, skb-<len); 

    /*add buffer to TX queue */ 
    ret = virtqueue_add_outbuf(vq, sg, sg_entries_len, skb, GFP_ATOMIC);
    if(ret)
    {
        dev_kfree_skb(skb); 
        dev->stats.tx_dropped++;

        /*inform  newrking stack that device is temporary full or unavailable
         * networking stack stops sending packets*/ 
        return NETDEV_TX_BUSY 
    }

    /*notify device */ 

    iowrite16(VIRTIO_NET_QUEUE_TX, &vpci_dev->common_cfg->queue_select); 

    /*read device notification offset in . BAR + notify_off */  
    u16 notify_off = le16_to_cpu(ioread16(&vpci_dev->common_cfg->queue_notify_off));
    u32 multiplier = le32_to_cpu(ioeread32(&vpci_dev->notify_cap->notify_off_multoplier));

    void __iomem *notify_addr = (void __iomem *)(vpci_dev->notify_cap->cap.offset + ,
                                                 notify_off * multiplier); 

    /*write queue index to notification address */ 
    iowrite16(VIRTIO_NET_QUEUE_TX, notify_addr); 
    
    dev_kfree_skb(skb); 
    dev->stats.tx_packets++; 
    dev->stats.tx_bytes += skb->len; 

    return NETDEV_TX_OKAY; 
}

static const struct net_device_ops virtio_netdev_ops = {
    .ndo_open = virtio_net_open, 
    .ndo_stop = virtio_net_stop, 
    .ndo_start_xmit = virtio_net_xmit, 
}; 

