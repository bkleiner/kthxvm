#pragma once

#include <vector>

#include <fmt/format.h>

#include <asm/types.h>
#include <linux/virtio_blk.h>
#include <linux/virtio_config.h>

#include "device.h"

namespace kvm::virtio {

  class rng : public device {
  public:
    std::vector<__u8> read(__u64 offset, __u32 size) {
      std::vector<__u8> buf(size);
      fmt::print("kvm::virtio::rng invalid config read at {:#x}\n", offset);
      return buf;
    }

    void write(__u8 *data, __u64 offset, __u32 size) {
      fmt::print("kvm::virtio::rng invalid config write at {:#x}\n", offset);
    }

    __u32 device_id() {
      return VIRTIO_ID_RNG;
    }

    __u32 features() {
      return 0;
    }

    __u32 config_generation() {
      return 0;
    }

    bool update(queue &q, __u8 *ptr) {
      queue::descriptor_elem_t *next = q.next(ptr);
      if (next == nullptr) {
        return false;
      }

      __u32 len = 0;
      __u32 desc_start = q.avail_id(ptr);

      q.add_used(ptr, desc_start, len);
      return true;
    }
  };

} // namespace kvm::virtio