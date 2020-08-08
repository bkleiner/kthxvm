#pragma once

#include <vector>

#include <asm/types.h>

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

    virtual std::vector<__u8> read(__u64 offset, __u32 size) = 0;
    virtual void write(__u8 *data, __u64 offset, __u32 size) = 0;

    virtual __u32 device_id() = 0;
    virtual __u32 features() = 0;
    virtual __u32 config_generation() = 0;

    virtual bool update(queue &q, __u8 *ptr) = 0;

    __u32 read_status() {
      return status;
    }

    void write_status(__u32 update) {
      status = update;
    }

    __u64 driver_features = 0;

    bool interrupt_asserted = false;

    bool device_feature_sel = false;
    bool driver_feature_sel = false;

  protected:
    __u8 status = VIRTIO_DEVICE_RESET;
  };
} // namespace kvm::virtio