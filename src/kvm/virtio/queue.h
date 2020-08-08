#pragma once

#include <asm/types.h>

namespace kvm::virtio {

  class queue {
  public:
    static constexpr __u32 QUEUE_SIZE_MAX = 0x100;

    struct descriptor_elem_t {
      __u64 addr;
      __u32 len;
      __u16 flags;
      __u16 next;
    };

    struct descriptor_t {
      descriptor_elem_t ring[QUEUE_SIZE_MAX];
    };

    struct avail_t {
      __u16 flags;
      __u16 idx;
      __u16 ring[QUEUE_SIZE_MAX];
      __u16 used_event;
    };

    struct used_elem_t {
      __u32 id;
      __u32 len;
    };

    struct used_t {
      __u16 flags;
      __u16 idx;
      used_elem_t ring[QUEUE_SIZE_MAX];
      __u16 avail_event;
    };

    inline descriptor_t *descriptors(__u8 *ptr) {
      return reinterpret_cast<descriptor_t *>(ptr + desc_addr);
    }

    inline avail_t *avail(__u8 *ptr) {
      return reinterpret_cast<avail_t *>(ptr + avail_addr);
    }

    inline __u16 avail_id(__u8 *ptr) {
      return avail(ptr)->ring[(last_avail - 1) % size];
    }

    inline used_t *used(__u8 *ptr) {
      return reinterpret_cast<used_t *>(ptr + used_addr);
    }

    descriptor_elem_t *next(__u8 *ptr) {
      if (!ready || avail(ptr)->idx <= last_avail) {
        return nullptr;
      }
      last_avail++;
      return &descriptors(ptr)->ring[avail_id(ptr)];
    }

  public:
    __u32 notify = 0;

    __u32 index = 0;
    __u32 size = 0;
    __u32 ready = 0;

    __u64 desc_addr = 0;
    __u64 avail_addr = 0;
    __u64 used_addr = 0;

    __u32 last_avail = 0;

  private:
  };

} // namespace kvm::virtio
