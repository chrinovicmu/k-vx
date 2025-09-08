#ifndef VIRTIO_H 
#define VIRTIO_H 

#include <linux/types.h> 
#include <linux/byteorder/generic.h>  

#define VIRTIO_CONFIG_SPACE_RESET       0x00 
#define VIRTIO_CONFIG_SPACE_ACKNOWLEDGE 0x01
#define VIRTIO_CONFIG_SPACE_DRIVER      0x02 
#define VIRTIO_CONFIG_SPACE_DRIVER_OK   0x04 
#define VIRTIO_CONFIG_SPACE_FEATURES_OK 0x80

#define VIRTIO_F_IN_ORDER               0X04000000 

#define VIRTIO_VIRTQUEUE_ALIGN_DT       16 
#define VIRTIO_VIRTQUEUE_ALIGN_AR       2 
#define VIRTIO_VIRTQUEUE_ALIGN_UR       4 

/*mark a buffer as continuing via next field */ 
#define VIRTQ_DESC_F_NEXT               1;
/*mark a buffer as device write only(otherwise device read-only) */ 
#define VIRTQ_DESC_F_WRITE              2; 

/*buffer contains list of buffer descriptors */ 
#define VIRTQ_DESC_F_INDIRECT           4; 

#define MAX_QUEUE_SIZE                  32768 

struct virtq_desc 
{
    __le64 addr; 
    __le32 len; 
    __le16 flags; /*VIRTQ_DESC_F_NEXT, VIRTQ_DESC_F_WRITE */  
    __le16 next; /*index of next descriptor */ 
}__attribute__((packed));

/*for Driver Area*/ 
struct virtq_avail 
{
    u16 flags; 
    u16 idx;    /*index of next availabe descriptor */ 
    u16 ring[]; /* array of descriptor indexes */ 
}__attribute__((packed)); 

struct virtq_used_elem
{
    u32 id;  /*desctiptor index */ 
    u32 len; /*bytes wrriten */ 
} __attribute__((packed)); 

/*for Device Area */  
struct virtq_used 
{
    u16 flags; 
    u16 idx; 
    struct virtq_used_elem ring[]; 
} __attribute__((packed));

struct virtqueue
{
    u16 desc_num;   /*number of descriptors */ 
    struct virtq_desc *desc; 
    struct virtq_avail *avail; 
    struct virtq_used *used; 
    u16 last_used_idx; /*last processed index*/ 
} __attribute__((packed)); 

#endif // !VIRTIO_H 



