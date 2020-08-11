#pragma once

#include <array>
#include <vector>

#include <asm/types.h>

#include "io_device.h"

namespace kvm::device {
  class i8042 : public io_device {
  public:
    static constexpr __u8 DATA_REG = 0x0;
    static constexpr __u8 COMMAND_REG = 0x04;

    static constexpr __u8 CMD_READ_CTR = 0x20;
    static constexpr __u8 CMD_WRITE_CTR = 0x60;
    static constexpr __u8 CMD_READ_OUTP = 0xD0;
    static constexpr __u8 CMD_WRITE_OUTP = 0xD1;
    static constexpr __u8 CMD_RESET_CPU = 0xFE;
    static constexpr __u8 CMD_RESET_KBD = 0xFF;

    static constexpr __u8 SB_OUT_DATA_AVAIL = 0x0001; // Data available at port 0x60
    static constexpr __u8 SB_I8042_CMD_DATA = 0x0008; // i8042 expecting command parameter at port 0x60
    static constexpr __u8 SB_KBD_ENABLED = 0x0010;    // 1 = kbd enabled, 0 = kbd locked

    static constexpr __u8 CB_KBD_INT = 0x0001; // kbd interrupt enabled
    static constexpr __u8 CB_POST_OK = 0x0004; // POST ok (should always be 1)

    static constexpr __u32 BUF_SIZE = 16;

    i8042(__u64 addr, __u64 width, ::kvm::interrupt *irq)
        : io_device(addr, width, irq) {}

    void trigger_irq() {
      if (control & CB_KBD_INT) {
        irq->set_level(true);
        irq->set_level(false);
      }
    }

    std::vector<__u8> read(__u64 offset, __u32 size) {
      std::vector<__u8> buf(size);
      switch (offset) {
      case COMMAND_REG:
        buf[0] = status;
        break;

      case DATA_REG:
        buf[0] = pop();
        if (status & SB_OUT_DATA_AVAIL) {
          trigger_irq();
        }
        break;

      default:
        break;
      }
      return buf;
    }

    void write_command(__u8 cmd) {
      switch (cmd) {
      case CMD_READ_CTR:
        flush();
        push(control);
        break;
      case CMD_WRITE_CTR:
        flush();
        status |= SB_I8042_CMD_DATA;
        command = cmd;
        break;

      case CMD_READ_OUTP:
        flush();
        push(outp);
        break;
      case CMD_WRITE_OUTP:
        status |= SB_I8042_CMD_DATA;
        command = cmd;
        break;

      case CMD_RESET_CPU:
        fmt::print("kvm::device::i8042 CMD_RESET_CPU\n");
        break;

      case CMD_RESET_KBD:
        status = 0x0;
        break;

      default:
        break;
      }
    }

    void write(__u8 *data, __u64 offset, __u32 size) {
      switch (offset) {
      case COMMAND_REG:
        write_command(data[0]);
        break;

      case DATA_REG:
        if (status & SB_I8042_CMD_DATA) {
          // write to register

          switch (command) {
          case CMD_WRITE_CTR:
            control = data[0];
            break;
          case CMD_WRITE_OUTP:
            outp = data[0];
            break;
          default:
            break;
          }

          status &= !SB_I8042_CMD_DATA;
        } else {
          // direct write
          flush();
          push(0xFA);

          trigger_irq();
        }
        break;

      default:
        break;
      }
    }

  private:
    __u8 command = 0;
    __u8 status = SB_KBD_ENABLED;
    __u8 control = CB_POST_OK | CB_KBD_INT;

    __u8 outp = 0;

    std::array<__u8, BUF_SIZE> buffer;
    __u32 buffer_head = 0;
    __u32 buffer_tail = 0;

    __u32 buffer_len() {
      return buffer_tail - buffer_head;
    }

    __u8 pop() {
      if (!buffer_len()) {
        return 0;
      }

      const auto res = buffer[buffer_head % BUF_SIZE];
      buffer_head++;

      if (!buffer_len()) {
        status &= !SB_OUT_DATA_AVAIL;
      }

      return res;
    }

    void push(__u8 val) {
      if (buffer_len() == BUF_SIZE) {
        return;
      }

      status |= SB_OUT_DATA_AVAIL;
      buffer[buffer_tail % BUF_SIZE] = val;
      buffer_tail++;
    }

    void flush() {
      buffer_head = 0;
      buffer_tail = 0;
      status &= !SB_OUT_DATA_AVAIL;
    }
  };
} // namespace kvm::device
