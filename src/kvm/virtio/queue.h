#pragma once

#include <asm/types.h>

#include "barrier.h"

namespace kvm::virtio {

  class queue {
  public:
    static constexpr __u32 QUEUE_SIZE_MAX = 0x200;

    struct descriptor_elem_t {
      __u64 addr;
      __u32 len;
      __u16 flags;
      __u16 next;
    };

    struct descriptor_t {
      descriptor_elem_t ring[QUEUE_SIZE_MAX];
    } __attribute__((aligned(16)));

    struct avail_t {
      __u16 flags;
      __u16 idx;
      __u16 ring[QUEUE_SIZE_MAX];
      __u16 used_event;
    } __attribute__((aligned(2)));

    struct used_elem_t {
      __u32 id;
      __u32 len;
    };

    struct used_t {
      __u16 flags;
      __u16 idx;
      used_elem_t ring[QUEUE_SIZE_MAX];
      __u16 avail_event;
    } __attribute__((aligned(4)));

    inline descriptor_t *desc(__u8 *ptr) {
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
      if (!notify || !ready) {
        return nullptr;
      }
      notify = false;

      rmb();
      if (avail(ptr)->idx <= last_avail) {
        return nullptr;
      }
      last_avail++;
      return &desc(ptr)->ring[avail_id(ptr)];
    }

    __u16 add_used(__u8 *ptr, __u32 start, __u32 len) {
      wmb();
      used(ptr)->ring[used(ptr)->idx % size].len = len;
      used(ptr)->ring[used(ptr)->idx % size].id = start;
      used(ptr)->idx++;
      wmb();
      return used(ptr)->idx;
    }

    void do_notify() {
      notify = true;
    }

  public:
    __u32 size = 0;
    __u32 ready = 0;

    __u64 desc_addr = 0;
    __u64 avail_addr = 0;
    __u64 used_addr = 0;

    __u32 last_avail = 0;

  private:
    bool notify = false;
  };

} // namespace kvm::virtio
