#pragma once

#include <vector>

#include <fmt/format.h>

#include <asm/types.h>
#include <linux/virtio_rng.h>

#include "device.h"

namespace kvm::virtio {

  class rng : public queue_device<VIRTIO_ID_RNG, 1> {
  public:
    rng(::kvm::interrupt *irq, __u8 *ptr)
        : queue_device<VIRTIO_ID_RNG, 1>(irq, ptr)
        , file("/dev/random") {
      if (!file.is_open())
        throw std::runtime_error(fmt::format("could not open file {}", "/dev/random"));
    }

    std::vector<__u8> read(__u64 offset, __u32 size) {
      std::vector<__u8> buf(size);
      fmt::print("kvm::virtio::rng invalid config read at {:#x}\n", offset);
      return buf;
    }

    void write(__u8 *data, __u64 offset, __u32 size) {
      fmt::print("kvm::virtio::rng invalid config write at {:#x}\n", offset);
    }

    __u32 features() {
      return 0;
    }

    __u32 config_generation() {
      return 0;
    }

    void update(__u8 *ptr) {
      queue::descriptor_elem_t *next = q().next();
      if (next == nullptr) {
        return;
      }

      __u32 len = 0;
      __u32 desc_start = q().avail_id();

      file.read((char *)(ptr + next->addr), next->len);
      len += next->len;

      q().add_used(desc_start, len);
      irq->set_level(true);
    }

  private:
    std::ifstream file;
  };

} // namespace kvm::virtio