#pragma once

#include <mutex>

#include <asm/types.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include "util.h"

namespace kvm {

  class interrupt {
  public:
    interrupt(__u32 num)
        : number(num)
        , fd(eventfd(0, 0))
        , state(false) {}

    void set_level(bool update) {
      const std::lock_guard<std::mutex> lock(mu);
      if (state == update) {
        return;
      }
      if (update) {
        __u64 value = 0x1;
        ssize_t ret = write(fd, &value, 8);
        if (ret < 0) {
          throw std::runtime_error(errno_msg("eventfd write failed"));
        }
      }
      state = update;
    }

    bool level() {
      const std::lock_guard<std::mutex> lock(mu);
      return state;
    }

    __u32 num() {
      return number;
    }

    __u32 event_fd() {
      return fd;
    }

  private:
    std::mutex mu;

    __u32 number;
    __u32 fd;

    bool state;
  };

} // namespace kvm
