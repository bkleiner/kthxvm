#pragma once

#include <memory>
#include <stdexcept>
#include <vector>

#include <fmt/format.h>

#include <fcntl.h>
#include <linux/kvm.h>
#include <sys/ioctl.h>

#include <sys/mman.h>

#include "interrupt.h"
#include "kvm.h"
#include "layout.h"
#include "vcpu.h"

#include "virtio/mmio.h"

namespace kvm {
  static constexpr __u64 KERNEL_BOOT_FLAG_MAGIC = 0xaa55;
  static constexpr __u64 KERNEL_HDR_MAGIC = 0x53726448;
  static constexpr __u64 KERNEL_LOADER_OTHER = 0xff;
  static constexpr __u64 KERNEL_MIN_ALIGNMENT_BYTES = 0x1000000;

  class vm {
  public:
    vm(kvm &k, int ncpus, size_t mem)
        : fd(k.create_vm()) {

      if (fd < 0)
        throw std::runtime_error("kvm vm create failed");

      fmt::print("vm: open with memory {:#x}\n", mem);
      enable(KVM_CAP_X2APIC_API);
      create_memory(mem);
      create_irq_chip();
      create_pit();
      create_vcpu(k);
    }

    __u8 *memory_ptr() {
      return memory;
    }

    vcpu &get_vcpu() {
      return *cpu;
    }

    void set_gsi_routing(struct kvm_irq_routing *router) {
      if (ioctl(fd, KVM_SET_GSI_ROUTING, router) < 0)
        ioctl_err("KVM_SET_GSI_ROUTING");
    }

    void send_interrupt(__u32 num, bool level) {
      struct kvm_irq_level irq;
      irq.irq = num;
      irq.level = level ? 1 : 0;
      if (ioctl(fd, KVM_IRQ_LINE, &irq) < 0)
        ioctl_err("KVM_IRQ_LINE");
    }

    void trigger_interrupt(__u32 num) {
      send_interrupt(num, true);
      send_interrupt(num, false);
    }

    void enable(__u32 cap) {
      struct kvm_enable_cap enable {
        cap : cap,
      };
      if (ioctl(fd, KVM_ENABLE_CAP, &enable) < 0)
        ioctl_err("KVM_ENABLE_CAP");
    }

    template <class device_type, typename... arg_types>
    device_type *add_io_device(__u64 addr, __u64 width, __u32 interrupt, arg_types &&... args) {
      auto ptr = new device_type{addr, width, interrupt, std::forward<arg_types>(args)...};
      if (interrupt) {
        register_irq(ptr->interrupt);
      }
      io_devices.emplace_back(ptr);
      return ptr;
    }

    template <class device_type, typename... arg_types>
    void add_mmio_device(__u64 addr, __u64 width, __u32 interrupt, arg_types &&... args) {
      mmio.add_device<device_type>(addr, width, interrupt, std::forward<arg_types>(args)...);
    }

    void handle_io_device(kvm_run *kvm_run, device::io_device &dev) {
      auto offset = dev.offset(kvm_run->io.port);

      if (kvm_run->io.direction == KVM_EXIT_IO_IN) {
        auto size = kvm_run->io.count * kvm_run->io.size;
        auto buf = dev.read(offset, size);

        auto ptr = (__u8 *)kvm_run + kvm_run->io.data_offset;
        memcpy(ptr, buf.data(), buf.size());
        return;
      }

      if (kvm_run->io.direction == KVM_EXIT_IO_OUT) {
        auto size = kvm_run->io.count * kvm_run->io.size;
        auto data = (__u8 *)kvm_run + kvm_run->io.data_offset;

        dev.write(data, offset, size);
        return;
      }
    }

    int run(bool single_step = false) {
      while (true) {
        auto kvm_run = cpu->run(single_step);

        switch (kvm_run->exit_reason) {
        case KVM_EXIT_IO:
          for (auto &dev : io_devices) {
            if (dev->in_range(kvm_run->io.port)) {
              handle_io_device(kvm_run, *dev);
              goto break_label;
            }
          }

          /* currently breaks uart. no idea why
          if (kvm_run->io.port == 0x70 && kvm_run->io.direction == KVM_EXIT_IO_OUT) {
            auto data = (char *)kvm_run + kvm_run->io.data_offset;
            auto offset = kvm_run->io.port - 0x70;
            auto size = kvm_run->io.count * kvm_run->io.size;

            rtc.write_index(data, offset, size);
            break;
          }

          if (kvm_run->io.port == 0x71) {
            auto offset = kvm_run->io.port - 0x71;

            if (kvm_run->io.direction == KVM_EXIT_IO_IN) {
              auto size = kvm_run->io.count * kvm_run->io.size;
              auto buf = rtc.read(offset, size);

              auto ptr = (char *)kvm_run + kvm_run->io.data_offset;
              memcpy(ptr, buf.data(), buf.size());
            } else if (kvm_run->io.direction == KVM_EXIT_IO_OUT) {
              auto data = (char *)kvm_run + kvm_run->io.data_offset;
              auto size = kvm_run->io.count * kvm_run->io.size;
              rtc.write(data, offset, size);
            }
            break;
          }
          */

          if (kvm_run->io.port == 0x80) {
            // bios post codes
            break;
          }

          if (kvm_run->io.port == 0x60 || kvm_run->io.port == 0x64) {
            // ps2 keyboard
            break;
          }

          fmt::print(
              "kvm::vcpu::io {} port {:#x} offset {:#x} size {} ({}) value {:#x}\n",
              kvm_run->io.direction == KVM_EXIT_IO_IN ? "in" : "out",
              kvm_run->io.port,
              kvm_run->io.data_offset,
              kvm_run->io.size,
              kvm_run->io.count,
              *((char *)kvm_run + kvm_run->io.data_offset));
        break_label:
          break;
        case KVM_EXIT_MMIO: {
          if (!kvm_run->mmio.is_write) {
            auto buf = mmio.read(kvm_run->mmio.phys_addr, kvm_run->mmio.len);
            memcpy(kvm_run->mmio.data, buf.data(), buf.size());
          } else {
            mmio.write(&kvm_run->mmio.data[0], kvm_run->mmio.phys_addr, kvm_run->mmio.len);
          }
          while (true) {
            __u32 irq = mmio.update(memory_ptr());
            if (irq == 0) {
              break;
            }
            trigger_interrupt(irq);
          }
          break;
        }
        case KVM_EXIT_DEBUG:
          fmt::print(
              "kvm::vcpu::debug exception {} dr6 {:#x} dr7 {:#x} pc {:#x}\n",
              kvm_run->debug.arch.exception,
              kvm_run->debug.arch.dr6,
              kvm_run->debug.arch.dr7,
              kvm_run->debug.arch.pc);
          break;
        case KVM_EXIT_HLT:
          fmt::print("kvm::vcpu halted\n");
          return 0;
        case KVM_EXIT_INTR:
          break;
        default:
          fmt::print("got exist reason {} from kvm_run\n", kvm_run->exit_reason);
          return kvm_run->exit_reason;
        }
      }
      return 1;
    }

  private:
    void create_memory(size_t mem) {
      memory = reinterpret_cast<__u8 *>(mmap(
          NULL,
          mem,
          PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
          -1,
          0));
      if (memory == MAP_FAILED) {
        throw std::runtime_error("memory map failed");
      }

      // madvise(memory, mem, MADV_MERGEABLE);

      struct kvm_userspace_memory_region memreg {
        0,
            0,
            0,
            mem,
            (unsigned long)memory
      };
      if (ioctl(fd, KVM_SET_USER_MEMORY_REGION, &memreg) < 0)
        ioctl_err("KVM_SET_USER_MEMORY_REGION");

      if (ioctl(fd, KVM_SET_TSS_ADDR, 0xfffbd000) < 0)
        ioctl_err("KVM_SET_TSS_ADDR");

      memory_size = mem;
    }

    void create_irq_chip() {
      if (ioctl(fd, KVM_CREATE_IRQCHIP, 0) < 0)
        ioctl_err("KVM_CREATE_IRQCHIP");
    }

    void create_pit() {
      struct kvm_pit_config pit {
        flags : KVM_PIT_SPEAKER_DUMMY,
      };
      if (ioctl(fd, KVM_CREATE_PIT2, &pit) < 0)
        ioctl_err("KVM_CREATE_PIT2");
    }

    void create_vcpu(kvm &k) {
      int vcpu_mmap_size = k.vcpu_mmap_size();
      int vcpu_fd = ioctl(fd, KVM_CREATE_VCPU, 0);
      cpu = std::make_unique<vcpu>(vcpu_fd, vcpu_mmap_size);
    }

    void register_irq(interrupt &irq) {
      struct kvm_irqfd irqfd = {
          irq.event_fd(),
          irq.num(),
          0,
          0,
      };
      if (ioctl(fd, KVM_IRQFD, &irqfd) < 0)
        ioctl_err("KVM_IRQFD");
    }

    int fd;

    __u8 *memory;
    size_t memory_size;

    std::unique_ptr<vcpu> cpu;
    std::vector<std::unique_ptr<device::io_device>> io_devices;

    virtio::mmio mmio;
  };

  void setup_bootparams(vm &vm, const std::string cmdline, __u64 memory_size) {
    struct boot_params *boot = reinterpret_cast<struct boot_params *>(vm.memory_ptr() + ZERO_PAGE_START);
    memset(boot, 0, sizeof(struct boot_params));

    memcpy(vm.memory_ptr() + CMDLINE_START, cmdline.data(), cmdline.size());

    boot->e820_table[boot->e820_entries].addr = 0;
    boot->e820_table[boot->e820_entries].size = EBDA_START;
    boot->e820_table[boot->e820_entries].type = E820_RAM;
    boot->e820_entries += 1;

    boot->e820_table[boot->e820_entries].addr = HIMEM_START;
    boot->e820_table[boot->e820_entries].size = memory_size - HIMEM_START;
    boot->e820_table[boot->e820_entries].type = E820_RAM;
    boot->e820_entries += 1;

    boot->hdr.type_of_loader = KERNEL_LOADER_OTHER;
    boot->hdr.boot_flag = KERNEL_BOOT_FLAG_MAGIC;
    boot->hdr.header = KERNEL_HDR_MAGIC;
    boot->hdr.cmd_line_ptr = CMDLINE_START;
    boot->hdr.cmdline_size = cmdline.size();
    boot->hdr.kernel_alignment = KERNEL_MIN_ALIGNMENT_BYTES;
  }

} // namespace kvm
