#pragma once

#include <asm/types.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include "util.h"

namespace kvm {

  class interrupt {
  public:
    interrupt(__u32 num)
        : number(num)
        , fd(eventfd(0, 0)) {}

    void set_level(bool new_state) {
      if (state == new_state) {
        return;
      }
      if (new_state) {
        __u64 value = 0x1;
        ssize_t ret = write(fd, &value, 8);
        if (ret < 0) {
          throw std::runtime_error(errno_msg("eventfd write failed"));
        }
      }
      state = new_state;
    }

    bool level() {
      return state;
    }

    __u32 num() {
      return number;
    }

    __u32 event_fd() {
      return fd;
    }

  private:
    __u32 number;
    __u32 fd;

    bool state = false;
  };

} // namespace kvm
