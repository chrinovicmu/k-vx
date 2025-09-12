#ifndef VIRTIO_H 
#define VIRTIO_H 

#include <linux/types.h> 
#include <linux/byteorder/generic.h>  
#include "util.h"

/*virtqueue alignment requiremnts */ 
#define VIRTIO_VIRTQUEUE_ALIGN_DT       16 
#define VIRTIO_VIRTQUEUE_ALIGN_AR       2 
#define VIRTIO_VIRTQUEUE_ALIGN_UR       4 

/*virtqueue size constants */ 
#define VIRTIO_VIRTQUEUE_SIZE_DT(q_size) (16 * (q_size))
#define VIRTIO_VIRTQUEUE_SIZE_AR(q_size) (6 + 2 * (q_size))
#define VIRTIO_VIRTQUEUE_SIZE_UR(q_size) (6 + 8 * (q_size))

/*mark a buffer as continuing via next field */ 
#define VIRTQ_DESC_F_NEXT               1;
/*mark a buffer as device write only(otherwise device read-only) */ 
#define VIRTQ_DESC_F_WRITE              2; 
/*buffer contains list of buffer descriptors */ 
#define VIRTQ_DESC_F_INDIRECT           4; 

/*max size of descriptor chain (2 ^32) */ 
#define VIRTQ_DESC_CHAIN_MAX_BYTES      0x100000000ULL  

/*used buffer notifications supressions */ 
#define VIRTQ_AVAIL_F_NO_INTERRUPT      1 
#define VIRTIO_F_EVENT_IDX              (1 << 29)
#define VIRTQ_USED_F_NO_NOTIFY          1 
#define VIRIO_VIRTQUEUE_MAX_SIZE        32768 

/*support for indirect descriptors */ 
#define VIRTIO_F_INDIRECT_DESC          28 

/*Arbitrary descriptor layout */ 
 #define VIRTIO_F_ANY_LAYOUT            27 

struct virtq_desc 
{
    __le64 addr; 
    __le32 len; 
    __le16 flags; /*VIRTQ_DESC_F_NEXT, VIRTQ_DESC_F_WRITE */  
    __le16 next; /*index of next descriptor */ 
    uint8_t padding[CACHE_LINE_SIZE - sizeof(__le64) - sizeof(__le32) - 2 * sizeof(__le16)]; 
}__attribute__((packed, aligned(VIRTIO_VIRTQUEUE_ALIGN_DT))); 

/*for Driver Area*/ 
struct virtq_avail 
{
    __le16 flags; 
    __le16 idx;    /*index of next availabe descriptor */ 
    __le16 ring[VIRTQ_VIRTQUEUE_MAX_SIZE]; /* array of descriptor indexes */ 
    __le16 used_event; /*if VIRTIO_F_EVENT_IDX */ 
}__attribute__((packed, aligned(VIRTIO_VIRTQUEUE_SIZE_AR))); 

struct virtq_used_elem
{
    __le32 id;  /*desctiptor index */ 
    __le32 len; /*bytes wrriten */ 
} __attribute__((packed)); 

/*for Device Area */  
struct virtq_used 
{
    __le16 flags; 
    __le16 idx; 
    struct virtq_used_elem ring[VIRTIO_VIRTQUEUE_MAX_SIZE]; 
    __le16 avail_event; /*if VIRTIO_F_EVENT_IDX */ 
} __attribute__((packed, aligned(VIRTIO_VIRTQUEUE_ALIGN_UR))); 

struct virtqueue
{
    __le16 desc_num;   /*number of descriptors */ 
    struct virtq_desc *desc; 
    struct virtq_avail *avail; 
    struct virtq_used *used; 
    __le16 last_used_idx; /*last processed index*/ 
    uint8_t padding[CACHE_LINE_SIZE - sizeof(__le16) - 3 * sizeof(void *) - sizeof(__le16)]; 
} __attribute__((packed, aligned(CACHE_LINE_SIZE))); 

static struct virtqueue *virtq; 

static int virtq_init(struct virtqueue, size_t size); 
void virtqueue_free(struct virtqueue);

#endif // !VIRTIO_H 



