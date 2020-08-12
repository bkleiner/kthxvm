#pragma once

#include <mutex>

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

    queue(__u8 *ptr)
        : ptr(ptr) {}

    template <class T>
    inline T *translate(__u64 addr) {
      return reinterpret_cast<T *>(ptr + addr);
    }

    inline descriptor_t *desc() {
      return translate<descriptor_t>(desc_addr);
    }

    inline avail_t *avail() {
      return translate<avail_t>(avail_addr);
    }

    inline used_t *used() {
      return translate<used_t>(used_addr);
    }

    inline __u16 avail_id() {
      return avail()->ring[(last_avail - 1) % size];
    }

    descriptor_elem_t *next() {
      {
        const std::lock_guard<std::mutex> lock(mu);
        if (!ready) {
          return nullptr;
        }
      }

      rmb();
      if (avail()->idx <= last_avail) {
        return nullptr;
      }
      last_avail++;
      return &desc()->ring[avail_id()];
    }

    __u16 add_used(__u32 start, __u32 len) {
      wmb();
      used()->ring[used()->idx % size].len = len;
      used()->ring[used()->idx % size].id = start;
      used()->idx++;
      wmb();
      return used()->idx;
    }

    void set_notify() {
      const std::lock_guard<std::mutex> lock(mu);
      notify++;
    }

    void set_ready() {
      const std::lock_guard<std::mutex> lock(mu);
      ready = true;
    }

    bool is_ready() {
      const std::lock_guard<std::mutex> lock(mu);
      return ready;
    }

  public:
    __u32 size = 0;

    __u64 desc_addr = 0;
    __u64 avail_addr = 0;
    __u64 used_addr = 0;

  private:
    __u8 *ptr;
    std::mutex mu;

    __u64 notify = 0;
    __u32 last_avail = 0;
    bool ready = false;
  };

} // namespace kvm::virtio
