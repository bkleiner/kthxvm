#pragma once

/* This marks a buffer as continuing via the next field. */
#define VRING_DESC_F_NEXT 1
/* This marks a buffer as write-only (otherwise read-only). */
#define VRING_DESC_F_WRITE 2
/* This means the buffer contains a list of buffer descriptors. */
#define VRING_DESC_F_INDIRECT 4

/*
 * Mark a descriptor as available or used in packed ring.
 * Notice: they are defined as shifts instead of shifted values.
 */
#define VRING_PACKED_DESC_F_AVAIL 7
#define VRING_PACKED_DESC_F_USED 15

/* The Host uses this in used->flags to advise the Guest: don't kick me when
 * you add a buffer.  It's unreliable, so it's simply an optimization.  Guest
 * will still kick if it's out of buffers. */
#define VRING_USED_F_NO_NOTIFY 1
/* The Guest uses this in avail->flags to advise the Host: don't interrupt me
 * when you consume a buffer.  It's unreliable, so it's simply an
 * optimization.  */
#define VRING_AVAIL_F_NO_INTERRUPT 1

/* Enable events in packed ring. */
#define VRING_PACKED_EVENT_FLAG_ENABLE 0x0
/* Disable events in packed ring. */
#define VRING_PACKED_EVENT_FLAG_DISABLE 0x1
/*
 * Enable events for a specific descriptor in packed ring.
 * (as specified by Descriptor Ring Change Event Offset/Wrap Counter).
 * Only valid if VIRTIO_RING_F_EVENT_IDX has been negotiated.
 */
#define VRING_PACKED_EVENT_FLAG_DESC 0x2

/*
 * Wrap counter bit shift in event suppression structure
 * of packed ring.
 */
#define VRING_PACKED_EVENT_F_WRAP_CTR 15

/* We support indirect buffer descriptors */
#define VIRTIO_RING_F_INDIRECT_DESC 28

/* The Guest publishes the used index for which it expects an interrupt
 * at the end of the avail ring. Host should ignore the avail->flags field. */
/* The Host publishes the avail index for which it expects a kick
 * at the end of the used ring. Guest should ignore the used->flags field. */
#define VIRTIO_RING_F_EVENT_IDX 29