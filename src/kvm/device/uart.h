#pragma once

#include <array>

#include <asm/types.h>
#include <fmt/format.h>

namespace kvm {
  namespace device {

    class uart {
    public:
      static constexpr __u8 DATA = 0;
      static constexpr __u8 IER = 1;
      static constexpr __u8 IIR = 2;
      static constexpr __u8 LCR = 3;
      static constexpr __u8 MCR = 4;
      static constexpr __u8 LSR = 5;
      static constexpr __u8 MSR = 6;
      static constexpr __u8 SCR = 7;

      static constexpr __u8 DLAB_LOW = 0;
      static constexpr __u8 DLAB_HIGH = 1;

      static constexpr __u8 IER_RECV_BIT = 0x1;
      static constexpr __u8 IER_THR_BIT = 0x2;
      static constexpr __u8 IER_FIFO_BITS = 0x0f;

      static constexpr __u8 IIR_FIFO_BITS = 0xc0;
      static constexpr __u8 IIR_NONE_BIT = 0x1;
      static constexpr __u8 IIR_THR_BIT = 0x2;
      static constexpr __u8 IIR_RECV_BIT = 0x4;

      static constexpr __u8 LCR_DLAB_BIT = 0x80;

      static constexpr __u8 LSR_DATA_BIT = 0x1;
      static constexpr __u8 LSR_EMPTY_BIT = 0x20;
      static constexpr __u8 LSR_IDLE_BIT = 0x40;

      static constexpr __u8 MCR_LOOP_BIT = 0x10;

      uart() {}

      std::vector<char> read(__u8 offset, __u8 size) {
        /*         fmt::print(
          "kvm::device::uart read offset {:#x} size {}\n", 
          offset,
          size
        ); */

        std::vector<char> buf(size);

        if (is_dlab_set()) {
          switch (offset) {
          case DLAB_LOW:
            buf[0] = baud_divisor;
            return buf;
          case DLAB_HIGH:
            buf[0] = baud_divisor >> 8;
            return buf;
          }
        }

        switch (offset) {
        case DATA:
          for (__u8 i = 0; i < size; i++) {
            buf[i] = getchar();
          }
          break;
        case IIR: {
          auto v = registers[IIR] | IIR_FIFO_BITS;
          registers[IIR] = IIR_NONE_BIT;
          buf[0] = v;
          break;
        }
        default:
          memcpy(buf.data(), &registers[offset], size);
          break;
        }
        return buf;
      }

      void write(char *data, __u8 offset, __u8 size) {
        /*         fmt::print(
          "kvm::device::uart write offset {:#x} size {}\n", 
          offset,
          size
        ); */

        if (is_dlab_set()) {
          switch (offset) {
          case DLAB_LOW:
            baud_divisor = (baud_divisor & 0xff00) | __u16(data[0]);
            break;
          case DLAB_HIGH:
            baud_divisor = (baud_divisor & 0x00ff) | (__u16(data[0]) << 8);
            break;
          }
        }

        switch (offset) {
        case DATA:
          printf(std::string(data, size).c_str());
          break;
        case IER:
          registers[offset] = data[0] & IER_FIFO_BITS;
          break;
        default:
          registers[offset] = data[0];
          break;
        }
      }

      bool is_dlab_set() {
        return (registers[LCR] & LCR_DLAB_BIT) != 0;
      }

    private:
      std::array<__u8, 8> registers = {
          0,                            // DATA = 0
          0,                            // IER = 1
          IIR_NONE_BIT,                 // IIR = 2
          0x3,                          // LCR = 3
          0x8,                          // MCR = 4
          LSR_EMPTY_BIT | LSR_IDLE_BIT, // LSR = 5
          0x20 | 0x10 | 0x80,           // MSR = 6
          0,                            // SCR = 7
      };
      __u8 baud_divisor = 12;
    };

  } // namespace device
} // namespace kvm