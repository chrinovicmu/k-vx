#ifndef VIRTIO_DEVICE_H
#define VIRTIO_DEVICE_H 

#include <linux/types.h> 
#include <linux/byteorder/generic.h>  
#include "util.h"

#define VIRTIO_CONFIG_SPACE_RESET       0x00 
#define VIRTIO_CONFIG_SPACE_ACKNOWLEDGE 0x01
#define VIRTIO_CONFIG_SPACE_DRIVER      0x02 
#define VIRTIO_CONFIG_SPACE_DRIVER_OK   0x04 
#define VIRTIO_CONFIG_SPACE_FEATURES_OK 0x80

#endif 
