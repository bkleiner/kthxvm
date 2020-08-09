#pragma once

#include <vector>

#include <asm/types.h>

#include "kvm/interrupt.h"

namespace kvm::device {

  class io_device {
  public:
    io_device(__u64 addr, __u64 width, ::kvm::interrupt *irq)
        : addr(addr)
        , width(width)
        , irq(irq) {}

    bool in_range(__u64 port) {
      return port >= addr && port < (addr + width);
    }

    __u64 offset(__u64 port) {
      return port - addr;
    }

    virtual std::vector<__u8> read(__u64 offset, __u32 size) = 0;
    virtual void write(__u8 *data, __u64 offset, __u32 size) = 0;

  protected:
    __u64 addr;
    __u64 width;
    ::kvm::interrupt *irq;
  };

} // namespace kvm::device
