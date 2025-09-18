#ifndef VIRTIO_NET 
#define VIRTIO_NET


/* 0-23 are original Virtio-net feature bits */

/* Device handles packets with partial checksum (checksum offloading). */
#define VIRTIO_NET_F_CSUM                (1ULL << 0)   /* 0x00000001 */

/* Driver (guest) can handle packets with partial checksum. */
#define VIRTIO_NET_F_GUEST_CSUM          (1ULL << 1)   /* 0x00000002 */

/* Device allows driver to reconfigure offload settings through control channel. */
#define VIRTIO_NET_F_CTRL_GUEST_OFFLOADS (1ULL << 2)   /* 0x00000004 */

/* Device reports the maximum MTU (Maximum Transmission Unit) it supports. */
#define VIRTIO_NET_F_MTU                 (1ULL << 3)   /* 0x00000008 */

/* Device provides a MAC address to the driver. */
#define VIRTIO_NET_F_MAC                 (1ULL << 5)   /* 0x00000020 */

/* Legacy: device supports generic segmentation offload (GSO). */
#define VIRTIO_NET_F_GSO                 (1ULL << 6)   /* 0x00000040 */

/* Driver can receive TCP segmentation offload (TSO) for IPv4. */
#define VIRTIO_NET_F_GUEST_TSO4          (1ULL << 7)   /* 0x00000080 */

/* Driver can receive TCP segmentation offload (TSO) for IPv6. */
#define VIRTIO_NET_F_GUEST_TSO6          (1ULL << 8)   /* 0x00000100 */

/* Driver can receive TCP segmentation offload (TSO) with Explicit Congestion Notification. */
#define VIRTIO_NET_F_GUEST_ECN           (1ULL << 9)   /* 0x00000200 */

/* Driver can receive UDP fragmentation offload (UFO). */
#define VIRTIO_NET_F_GUEST_UFO           (1ULL << 10)  /* 0x00000400 */

/* Device can receive TCP segmentation offload (TSO) for IPv4. */
#define VIRTIO_NET_F_HOST_TSO4           (1ULL << 11)  /* 0x00000800 */

/* Device can receive TCP segmentation offload (TSO) for IPv6. */
#define VIRTIO_NET_F_HOST_TSO6           (1ULL << 12)  /* 0x00001000 */

/* Device can receive TCP segmentation offload (TSO) with Explicit Congestion Notification. */
#define VIRTIO_NET_F_HOST_ECN            (1ULL << 13)  /* 0x00002000 */

/* Device can receive UDP fragmentation offload (UFO). */
#define VIRTIO_NET_F_HOST_UFO            (1ULL << 14)  /* 0x00004000 */

/* Driver can merge multiple receive buffers into one packet. */
#define VIRTIO_NET_F_MRG_RXBUF           (1ULL << 15)  /* 0x00008000 */

/* Device has a status field (provides link status and other info). */
#define VIRTIO_NET_F_STATUS              (1ULL << 16)  /* 0x00010000 */

/* A dedicated control virtqueue is available. */
#define VIRTIO_NET_F_CTRL_VQ             (1ULL << 17)  /* 0x00020000 */

/* Device supports receive mode control (promiscuous, all-multicast, etc.). */
#define VIRTIO_NET_F_CTRL_RX             (1ULL << 18)  /* 0x00040000 */

/* Device supports VLAN filtering configuration via control virtqueue. */
#define VIRTIO_NET_F_CTRL_VLAN           (1ULL << 19)  /* 0x00080000 */

/* Driver can send gratuitous ARP or unsolicited neighbor advertisements. */
#define VIRTIO_NET_F_GUEST_ANNOUNCE      (1ULL << 21)  /* 0x00200000 */

/* Device supports multiple receive queues with steering (multiqueue). */
#define VIRTIO_NET_F_MQ                  (1ULL << 22)  /* 0x00400000 */

/* Driver can set MAC address through control virtqueue. */
#define VIRTIO_NET_F_CTRL_MAC_ADDR       (1ULL << 23)  /* 0x00800000 */


/* Legacy guest RSC (Receive Segment Coalescing) */

/* Driver can coalesce TCP segments for IPv4 (legacy). */
#define VIRTIO_NET_F_GUEST_RSC4          (1ULL << 41)  /* 0x20000000000 */

/* Driver can coalesce TCP segments for IPv6 (legacy). */
#define VIRTIO_NET_F_GUEST_RSC6          (1ULL << 42)  /* 0x40000000000 */


/* Newer feature bits (>= 56) */

/* Device can receive UDP Segmentation Offload (USO). */
#define VIRTIO_NET_F_HOST_USO            (1ULL << 56)  /* 0x0100000000000000 */

/* Device can report per-packet hash values (used for RSS or flow steering). */
#define VIRTIO_NET_F_HASH_REPORT         (1ULL << 57)  /* 0x0200000000000000 */

/* Driver can supply the exact header length when offloading. */
#define VIRTIO_NET_F_GUEST_HDRLEN        (1ULL << 59)  /* 0x0800000000000000 */

/* Device supports Receive-Side Scaling (RSS) to distribute traffic across CPUs. */
#define VIRTIO_NET_F_RSS                 (1ULL << 60)  /* 0x1000000000000000 */

/* Device can report detailed coalesced TCP segment info. */
#define VIRTIO_NET_F_RSC_EXT             (1ULL << 61)  /* 0x2000000000000000 */

/* Device can act as a standby (backup) for high availability setups. */
#define VIRTIO_NET_F_STANDBY             (1ULL << 62)  /* 0x4000000000000000 */

/* Device can report its physical link speed and duplex mode. */
#define VIRTIO_NET_F_SPEED_DUPLEX        (1ULL << 63)  /* 0x8000000000000000 */

#endif // !VIRTIO_NET 

