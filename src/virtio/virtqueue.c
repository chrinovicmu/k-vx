
#include <linux/types.h> 
#include <linux/byteorder/generic.h>  
#include "util.h"
#include "includes/virtqueue.h"

static int virtq_init(struct virtqueue * virtq, size_t size)
{
    size_t size_desc;
    size_t size_avail; 
    size_t size_used; 
    size_t total_size; 

    void *dma_mem; 
    dma_addr_t dma_handle; 

    virtq = kmalloc(sizeof(*vq), GFP_KERNEL); 
    if(!virtq)
        return NULL; 

    virt1->desc_num = VIRTIO_VIRTQUEUE_MAX_SIZE; 

    size_desc = VIRTIO_VIRTQUEUE_SIZE_DT(VIRTIO_VIRTQUEUE_MAX_SIZE);
    size_avail = VIRTIO_VIRTQUEUE_SIZE_AR(VIRTIO_VIRTQUEUE_MAX_SIZE); 
    size_used = VIRTIO_VIRTQUEUE_SIZE_UR(VIRTIO_VIRTQUEUE_MAX_SIZE);

    total_size = size_desc + size_avail + size_used; 

     e

}

