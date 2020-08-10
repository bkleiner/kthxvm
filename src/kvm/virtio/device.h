#pragma once

#include <vector>

#include <asm/types.h>

#include "kvm/interrupt.h"
#include "queue.h"

namespace kvm::virtio {
  class device {
  public:
    enum status {
      VIRTIO_DEVICE_RESET = 0,
      VIRTIO_DEVICE_ACKNOWLEDGE = 1,
      VIRTIO_DEVICE_DRIVER = 2,
      VIRTIO_DEVICE_DRIVER_OK = 4,
      VIRTIO_DEVICE_FEATURES_OK = 8,
      VIRTIO_DEVICE_DEVICE_NEEDS_RESET = 64,
      VIRTIO_DEVICE_FAILED = 128,
    };

    device(::kvm::interrupt *irq)
        : irq(irq) {}

    virtual std::vector<__u8> read(__u64 offset, __u32 size) = 0;
    virtual void write(__u8 *data, __u64 offset, __u32 size) = 0;

    virtual __u32 device_id() = 0;
    virtual __u32 features() = 0;
    virtual __u32 config_generation() = 0;

    virtual queue &q() = 0;
    virtual queue &q(__u32 index) = 0;

    virtual void update(__u8 *ptr) = 0;

    __u32 read_status() {
      return status;
    }

    void write_status(__u32 update) {
      status = update;
    }

    bool irq_level() {
      return irq->level();
    }

    void irq_ack() {
      irq->set_level(false);
    }

    __u64 driver_features = 0;
    __u32 queue_index = 0;

    bool device_feature_sel = false;
    bool driver_feature_sel = false;

  protected:
    ::kvm::interrupt *irq;
    __u8 status = VIRTIO_DEVICE_RESET;
  };

  template <__u32 dev_id, size_t queue_count>
  class queue_device : public device {
  public:
    queue_device(::kvm::interrupt *irq, __u8 *ptr)
        : device(irq) {
      for (size_t i = 0; i < queue_count; i++) {
        queues[i] = std::make_unique<queue>(ptr);
      }
    }

    queue &q(__u32 index) override {
      return *queues[index];
    }

    queue &q() override {
      return *queues[queue_index];
    }

    __u32 device_id() override {
      return dev_id;
    }

  protected:
    std::array<std::unique_ptr<queue>, queue_count> queues;
  };

} // namespace kvm::virtio