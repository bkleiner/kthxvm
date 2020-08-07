#pragma once

#include <vector>

#include <asm/types.h>
#include <linux/virtio_ids.h>
#include <linux/virtio_mmio.h>

namespace kvm {
  namespace virtio {
    class mmio {
    public:
      static constexpr __u32 MMIO_MAGIC_VALUE = 0x74726976;

      std::vector<__u8> read(__u64 offset, __u32 size) {
        std::vector<__u8> buf(size);
        switch (offset) {
        case VIRTIO_MMIO_MAGIC_VALUE:
          *((__u32 *)buf.data()) = MMIO_MAGIC_VALUE;
          break;
        case VIRTIO_MMIO_VERSION:
          *((__u32 *)buf.data()) = 2;
          break;
        case VIRTIO_MMIO_DEVICE_ID:
          *((__u32 *)buf.data()) = VIRTIO_ID_BLOCK;
          break;
        case VIRTIO_MMIO_VENDOR_ID:
          *((__u32 *)buf.data()) = 0;
          break;
        }
        return buf;
      }

      void write(__u8 *data, __u64 offset, __u32 size) {
        switch (offset) {
        case VIRTIO_MMIO_MAGIC_VALUE:
          break;
        }
      }

    private:
    };
  } // namespace virtio
} // namespace kvm