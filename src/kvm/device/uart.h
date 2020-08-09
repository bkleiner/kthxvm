#pragma once

#include <array>
#include <iostream>
#include <mutex>

#include <asm/types.h>
#include <fmt/format.h>

#include "kvm/interrupt.h"
#include "os/terminal.h"

namespace kvm {
  namespace device {

    class uart {
    public:
      static constexpr __u8 DATA = 0;
      static constexpr __u8 IER = 1;
      static constexpr __u8 IIR = 2;
      static constexpr __u8 FCR = 2;
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
      static constexpr __u8 LSR_BREAK_BIT = 0x10;
      static constexpr __u8 LSR_EMPTY_BIT = 0x20;
      static constexpr __u8 LSR_IDLE_BIT = 0x40;

      static constexpr __u8 MCR_LOOP_BIT = 0x10;

      static constexpr __u8 FIFO_LEN = 64;

      uart(__u8 irq, ::os::terminal *terminal)
          : interrupt(irq)
          , term(terminal) {}

      std::vector<char> read(__u8 offset, __u8 size) {
        /*         fmt::print(
          "kvm::device::uart read offset {:#x} size {}\n", 
          offset,
          size
        ); */

        const std::lock_guard<std::mutex> lock(mu);

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
          if (rx_cnt == rx_done)
            break;

          if (lsr & LSR_BREAK_BIT) {
            lsr &= ~LSR_BREAK_BIT;
            buf[0] = 0;
            break;
          }

          buf[0] = rx_buf[rx_done++];
          if (rx_cnt == rx_done) {
            lsr &= ~LSR_DATA_BIT;
            rx_cnt = rx_done = 0;
          }
          break;
        case IER:
          buf[0] = ier;
          break;
        case IIR:
          buf[0] = iir | IIR_FIFO_BITS;
          break;
        case LCR:
          buf[0] = lcr;
          break;
        case MCR:
          buf[0] = mcr;
          break;
        case LSR:
          buf[0] = lsr;
          break;
        case MSR:
          buf[0] = msr;
          break;
        case SCR:
          buf[0] = scr;
          break;

        default:
          break;
        }

        update_irq();
        return buf;
      }

      void write(char *data, __u8 offset, __u8 size) {
        /*         fmt::print(
          "kvm::device::uart write offset {:#x} size {}\n", 
          offset,
          size
        ); */

        const std::lock_guard<std::mutex> lock(mu);

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
          if (mcr & MCR_LOOP_BIT) {
            // loopback mode
            if (rx_cnt < FIFO_LEN) {
              rx_buf[rx_cnt++] = data[0];
              lsr |= LSR_DATA_BIT;
            }
            break;
          }

          if (tx_cnt < FIFO_LEN) {
            tx_buf[tx_cnt++] = data[0];
            lsr &= ~LSR_IDLE_BIT;
            if (tx_cnt == FIFO_LEN / 2) {
              lsr &= ~LSR_EMPTY_BIT;
            }
            flush_tx();
          } else {
            /* Should never happpen */
            lsr &= ~(LSR_EMPTY_BIT | LSR_IDLE_BIT);
          }
          break;

        case IER:
          ier = data[0] & IER_FIFO_BITS;
          break;
        case FCR:
          fcr = data[0];
          break;
        case LCR:
          lcr = data[0];
          break;
        case MCR:
          mcr = data[0];
          break;
        case LSR:
          /* Factory test */
          break;
        case MSR:
          /* Not used */
          break;
        case SCR:
          scr = data[0];
          break;

        default:
          break;
        }

        update_irq();
        return;
      }

      bool is_dlab_set() {
        return (lcr & LCR_DLAB_BIT) != 0;
      }

      bool is_loop() {
        return (mcr & MCR_LOOP_BIT) != 0;
      }

      void flush_tx() {
        lsr |= LSR_EMPTY_BIT | LSR_IDLE_BIT;

        if (tx_cnt) {
          if (term) {
            term->put(tx_buf.data(), tx_cnt);
          }
          tx_cnt = 0;
        }
      }

      void update_irq() {
        __u8 tmp_iir;

        if ((ier & IER_RECV_BIT) && (lsr & LSR_DATA_BIT)) {
          tmp_iir |= IIR_RECV_BIT;
        }

        if ((ier & IER_THR_BIT) && (lsr & LSR_IDLE_BIT)) {
          tmp_iir |= IIR_THR_BIT;
        }

        if (!tmp_iir) {
          iir = IIR_NONE_BIT;
          interrupt.set_level(false);
        } else {
          iir = tmp_iir;
          interrupt.set_level(true);
        }

        if (!(ier & IER_THR_BIT))
          flush_tx();
      }

      size_t write_stdio() {
        const std::lock_guard<std::mutex> lock(mu);

        if (mcr & MCR_LOOP_BIT) {
          return 0;
        }

        if ((lsr & LSR_DATA_BIT) || rx_cnt) {
          return 0;
        }

        while (term->readable(0) && rx_cnt < FIFO_LEN) {
          int c = term->get();
          if (c < 0) {
            break;
          }

          rx_buf[rx_cnt++] = c;
          lsr |= LSR_DATA_BIT;
        }

        update_irq();
        return 1;
      }

      ::kvm::interrupt interrupt;

    private:
      std::mutex mu;

      ::os::terminal *term;
      __u16 baud_divisor = 0;

      __u8 data = 0;
      __u8 ier = 0;
      __u8 iir = IIR_NONE_BIT;
      __u8 fcr = 0;
      __u8 lcr = 0;
      __u8 mcr = 0x08;
      __u8 lsr = LSR_EMPTY_BIT | LSR_IDLE_BIT;
      __u8 msr = 0x20 | 0x10 | 0x80;
      __u8 scr = 0;

      std::array<__u8, FIFO_LEN> rx_buf;
      __u8 rx_cnt = 0;
      __u8 rx_done = 0;

      std::array<__u8, FIFO_LEN> tx_buf;
      __u8 tx_cnt = 0;
    };

  } // namespace device
} // namespace kvm