#pragma once

#include <vector>

#include <fmt/format.h>

#include <asm/types.h>
#include <linux/virtio_ids.h>
#include <linux/virtio_mmio.h>

#include "kvm/device/io_device.h"

#include "blk.h"
#include "rng.h"

namespace kvm::virtio {
  class mmio_device : public ::kvm::device::io_device {
  public:
    mmio_device(__u64 addr, __u64 width, __u32 irq)
        : ::kvm::device::io_device(addr, width, irq) {}

    virtual bool update(__u8 *ptr) = 0;
  };

  template <class device_type>
  class mmio_device_holder : public mmio_device {
  public:
    static constexpr __u32 MMIO_MAGIC_VALUE = 0x74726976;
    static constexpr __u32 VIRT_VENDOR = 0x4b544858; // 'KTHX'

    template <typename... arg_types>
    mmio_device_holder(__u64 addr, __u64 width, __u32 interrupt, arg_types &&... args)
        : mmio_device(addr, width, interrupt)
        , dev(std::forward<arg_types>(args)...) {
    }

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
        *((__u32 *)buf.data()) = dev.device_id();
        break;
      case VIRTIO_MMIO_VENDOR_ID:
        *((__u32 *)buf.data()) = VIRT_VENDOR;
        break;

      case VIRTIO_MMIO_STATUS:
        *((__u32 *)buf.data()) = dev.read_status();
        break;

      case VIRTIO_MMIO_DEVICE_FEATURES: {
        const __u64 features = __u64(dev.features()) | (1ul << VIRTIO_F_VERSION_1);
        const __u32 shift = (dev.device_feature_sel ? 32 : 0);

        *((__u32 *)buf.data()) = (features >> shift) & 0xFFFFFF;
        break;
      }

      case VIRTIO_MMIO_QUEUE_NUM_MAX:
        *((__u32 *)buf.data()) = queue::QUEUE_SIZE_MAX;
        break;

      case VIRTIO_MMIO_QUEUE_READY:
        *((__u32 *)buf.data()) = dev.q().ready;
        break;

      case VIRTIO_MMIO_CONFIG_GENERATION:
        *((__u32 *)buf.data()) = dev.config_generation();
        break;

      case VIRTIO_MMIO_INTERRUPT_STATUS:
        *((__u32 *)buf.data()) = dev.interrupt_asserted ? 0x1 : 0x0;
        break;

      case VIRTIO_MMIO_DEVICE_FEATURES_SEL:
      case VIRTIO_MMIO_DRIVER_FEATURES:
      case VIRTIO_MMIO_DRIVER_FEATURES_SEL:
      case VIRTIO_MMIO_GUEST_PAGE_SIZE:
      case VIRTIO_MMIO_QUEUE_SEL:
      case VIRTIO_MMIO_QUEUE_NUM:
      case VIRTIO_MMIO_QUEUE_ALIGN:
      case VIRTIO_MMIO_QUEUE_NOTIFY:
      case VIRTIO_MMIO_INTERRUPT_ACK:
      case VIRTIO_MMIO_QUEUE_DESC_LOW:
      case VIRTIO_MMIO_QUEUE_DESC_HIGH:
      case VIRTIO_MMIO_QUEUE_AVAIL_LOW:
      case VIRTIO_MMIO_QUEUE_AVAIL_HIGH:
      case VIRTIO_MMIO_QUEUE_USED_LOW:
      case VIRTIO_MMIO_QUEUE_USED_HIGH:
        fmt::print("kvm::virtio::mmio read of write-only register {:#x}\n", offset);
        break;

      default:
        if (offset >= VIRTIO_MMIO_CONFIG) {
          return dev.read(offset - VIRTIO_MMIO_CONFIG, size);
        }
        fmt::print("kvm::virtio::mmio unhandled read at {:#x}\n", offset);
        break;
      }
      return buf;
    }

    void write(__u8 *data, __u64 offset, __u32 size) {
      const __u32 value = *((__u32 *)data);

      switch (offset) {
      case VIRTIO_MMIO_MAGIC_VALUE:
      case VIRTIO_MMIO_VERSION:
      case VIRTIO_MMIO_DEVICE_ID:
      case VIRTIO_MMIO_VENDOR_ID:
      case VIRTIO_MMIO_DEVICE_FEATURES:
      case VIRTIO_MMIO_QUEUE_NUM_MAX:
      case VIRTIO_MMIO_INTERRUPT_STATUS:
      case VIRTIO_MMIO_CONFIG_GENERATION:
        fmt::print("kvm::virtio::mmio write of read-only register {:#x}\n", offset);
        break;

      case VIRTIO_MMIO_STATUS:
        dev.write_status(value);
        break;

      case VIRTIO_MMIO_DEVICE_FEATURES_SEL:
        dev.device_feature_sel = value == 1;
        break;

      case VIRTIO_MMIO_DRIVER_FEATURES: {
        const __u32 shift = (dev.driver_feature_sel ? 32 : 0);
        dev.driver_features |= (__u64(value) << shift);
        break;
      }

      case VIRTIO_MMIO_DRIVER_FEATURES_SEL:
        dev.driver_feature_sel = value == 1;
        break;

      case VIRTIO_MMIO_QUEUE_SEL:
        dev.queue_index = value;
        break;

      case VIRTIO_MMIO_QUEUE_NUM:
        dev.q().size = value;
        break;

      case VIRTIO_MMIO_QUEUE_READY:
        dev.q().ready = value;
        break;

      case VIRTIO_MMIO_QUEUE_NOTIFY:
        dev.q().notify = 1;
        break;

      case VIRTIO_MMIO_QUEUE_DESC_LOW:
        dev.q().desc_addr |= (__u64(value) << 0);
        break;
      case VIRTIO_MMIO_QUEUE_DESC_HIGH:
        dev.q().desc_addr |= (__u64(value) << 32);
        break;
      case VIRTIO_MMIO_QUEUE_AVAIL_LOW:
        dev.q().avail_addr |= (__u64(value) << 0);
        break;
      case VIRTIO_MMIO_QUEUE_AVAIL_HIGH:
        dev.q().avail_addr |= (__u64(value) << 32);
        break;
      case VIRTIO_MMIO_QUEUE_USED_LOW:
        dev.q().used_addr |= (__u64(value) << 0);
        break;
      case VIRTIO_MMIO_QUEUE_USED_HIGH:
        dev.q().used_addr |= (__u64(value) << 32);
        break;

      case VIRTIO_MMIO_INTERRUPT_ACK:
        dev.interrupt_asserted = false;
        break;

      default:
        if (offset >= VIRTIO_MMIO_CONFIG) {
          return dev.write(data, offset - VIRTIO_MMIO_CONFIG, size);
        }
        fmt::print("kvm::virtio::mmio unhandled write at {:#x}\n", offset);
        break;
      }
    }

    bool update(__u8 *ptr) {
      if (!dev.interrupt_asserted && dev.update(ptr)) {
        dev.interrupt_asserted = true;
        return true;
      }
      return false;
    }

  private:
    device_type dev;
  };

  class mmio {
  public:
    std::vector<__u8> read(__u64 offset, __u32 size) {
      for (auto &dev : devices) {
        if (dev->in_range(offset)) {
          return dev->read(dev->offset(offset), size);
        }
      }

      return {};
    }

    void write(__u8 *data, __u64 offset, __u32 size) {
      for (auto &dev : devices) {
        if (dev->in_range(offset)) {
          return dev->write(data, dev->offset(offset), size);
        }
      }
    }

    __u32 update(__u8 *ptr) {
      for (auto &dev : devices) {
        if (dev->update(ptr)) {
          return dev->irq();
        }
      }
      return 0;
    }

    template <class device_type, typename... arg_types>
    void add_device(__u64 addr, __u64 width, __u32 interrupt, arg_types &&... args) {
      devices.emplace_back(new mmio_device_holder<device_type>{addr, width, interrupt, std::forward<arg_types>(args)...});
    }

  private:
    std::vector<std::unique_ptr<mmio_device>> devices;
  };

} // namespace kvm::virtio