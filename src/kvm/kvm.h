#pragma once

#include <memory>
#include <stdexcept>
#include <vector>

#include <fmt/format.h>

#include <fcntl.h>
#include <linux/kvm.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "device/uart.h"
#include "virtio/mmio.h"

namespace kvm {
  void ioctl_err(std::string ctl) {
    auto err = fmt::format("{}: {} ({})", ctl, strerror(errno), errno);
    throw std::runtime_error(err);
  }

  class kvm {
  public:
    kvm()
        : fd(open("/dev/kvm", O_RDWR))
        , version(api_version()) {
      if (fd < 0)
        throw std::runtime_error("kvm open failed");

      if (version != KVM_API_VERSION)
        throw std::runtime_error("invalid KVM_GET_API_VERSION");

      fmt::print("kvm: open with version {}\n", version);
    }

    int api_version() {
      return ioctl(fd, KVM_GET_API_VERSION, 0);
    }

    int vcpu_mmap_size() {
      return ioctl(fd, KVM_GET_VCPU_MMAP_SIZE, 0);
    }

    int create_vm() {
      return ioctl(fd, KVM_CREATE_VM, 0);
    }

    struct kvm_cpuid2 *get_supported_cpuid(__u32 nent) {
      size_t length = sizeof(kvm_cpuid2) + nent * sizeof(kvm_cpuid_entry2);
      char *buf = new char[length];

      memset(buf, 0, length);

      struct kvm_cpuid2 *cpuid2 = reinterpret_cast<struct kvm_cpuid2 *>(buf);
      cpuid2->nent = nent;

      if (ioctl(fd, KVM_GET_SUPPORTED_CPUID, cpuid2) < 0)
        ioctl_err("KVM_GET_SUPPORTED_CPUID");

      return cpuid2;
    }

    struct kvm_cpuid2 *get_emulated_cpuid(__u32 nent) {
      size_t length = sizeof(kvm_cpuid2) + nent * sizeof(kvm_cpuid_entry2);
      char *buf = new char[length];

      memset(buf, 0, length);

      struct kvm_cpuid2 *cpuid2 = reinterpret_cast<struct kvm_cpuid2 *>(buf);
      cpuid2->nent = nent;

      if (ioctl(fd, KVM_GET_EMULATED_CPUID, cpuid2) < 0)
        ioctl_err("KVM_GET_EMULATED_CPUID");

      return cpuid2;
    }

    bool supports(__u64 cap) {
      return ioctl(fd, KVM_CHECK_EXTENSION, cap);
    }

  private:
    int fd;
    int version;
  };

  class vcpu {
  public:
    vcpu(int fd, int vcpu_mmap_size)
        : fd(fd) {
      if (fd < 0)
        throw std::runtime_error("kvm vcpu create failed");

      if (vcpu_mmap_size <= 0)
        throw std::runtime_error("invalid KVM_GET_VCPU_MMAP_SIZE");

      kvm_run = reinterpret_cast<struct kvm_run *>(mmap(NULL, vcpu_mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
      if (kvm_run == MAP_FAILED)
        throw std::runtime_error("mmap kvm_run failed");

      fmt::print("vcpu: open with mmap_size {:#x}\n", vcpu_mmap_size);
    }

    struct kvm_lapic_state get_lapic() {
      struct kvm_lapic_state lapic;
      if (ioctl(fd, KVM_GET_LAPIC, &lapic) < 0)
        ioctl_err("KVM_GET_LAPIC");
      return lapic;
    }

    void set_lapic(struct kvm_lapic_state &lapic) {
      if (ioctl(fd, KVM_SET_LAPIC, &lapic) < 0)
        ioctl_err("KVM_SET_LAPIC");
    }

    struct kvm_sregs get_sregs() {
      struct kvm_sregs sregs;
      if (ioctl(fd, KVM_GET_SREGS, &sregs) < 0)
        ioctl_err("KVM_GET_SREGS");
      return sregs;
    }

    void set_sreg(struct kvm_sregs &sregs) {
      if (ioctl(fd, KVM_SET_SREGS, &sregs) < 0)
        ioctl_err("KVM_SET_SREGS");
    }

    void set_regs(struct kvm_regs &regs) {
      if (ioctl(fd, KVM_SET_REGS, &regs) < 0)
        ioctl_err("KVM_SET_REGS");
    }

    void set_fpu(struct kvm_fpu &fpu) {
      if (ioctl(fd, KVM_SET_FPU, &fpu) < 0)
        ioctl_err("KVM_SET_FPU");
    }

    void set_msrs(struct kvm_msrs *msrs) {
      if (ioctl(fd, KVM_SET_MSRS, msrs) < 0)
        ioctl_err("KVM_SET_MSRS");
    }

    void set_cpuid2(struct kvm_cpuid2 *cpuid) {
      if (ioctl(fd, KVM_SET_CPUID2, cpuid) < 0)
        ioctl_err("KVM_SET_CPUID2");
    }

    struct kvm_run *run(bool single_step = false) {
      if (single_step) {
        struct kvm_guest_debug debug;
        debug.control = KVM_GUESTDBG_ENABLE | KVM_GUESTDBG_SINGLESTEP;

        if (ioctl(fd, KVM_SET_GUEST_DEBUG, &debug) < 0)
          ioctl_err("KVM_SET_GUEST_DEBUG");
      }

      if (ioctl(fd, KVM_RUN, 0) < 0)
        ioctl_err("KVM_RUN");

      return kvm_run;
    }

  private:
    int fd;
    struct kvm_run *kvm_run;
  };

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

    int run(bool single_step = false) {
      while (true) {
        auto kvm_run = cpu->run(single_step);

        switch (kvm_run->exit_reason) {
        case KVM_EXIT_IO:
          if (kvm_run->io.port >= 0x3f8 && kvm_run->io.port <= 0x3ff) {
            auto offset = kvm_run->io.port - 0x3f8;

            if (kvm_run->io.direction == KVM_EXIT_IO_IN) {
              auto size = kvm_run->io.count * kvm_run->io.size;
              auto buf = uart.read(offset, size);

              auto ptr = (char *)kvm_run + kvm_run->io.data_offset;
              memcpy(ptr, buf.data(), buf.size());
            } else if (kvm_run->io.direction == KVM_EXIT_IO_OUT) {
              auto size = kvm_run->io.count * kvm_run->io.size;
              auto data = (char *)kvm_run + kvm_run->io.data_offset;
              uart.write(data, offset, size);
            }

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

          break;
        case KVM_EXIT_MMIO: {
          if (!kvm_run->mmio.is_write) {
            auto buf = mmio.read(kvm_run->mmio.phys_addr, kvm_run->mmio.len);
            memcpy(kvm_run->mmio.data, buf.data(), buf.size());
          } else {
            mmio.write(&kvm_run->mmio.data[0], kvm_run->mmio.phys_addr, kvm_run->mmio.len);
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
        default:
          fmt::print("got exist reason {} from kvm_run\n", kvm_run->exit_reason);
          return kvm_run->exit_reason;
        }

        while (true) {
          __u32 irq = mmio.update(memory_ptr());
          if (irq == 0) {
            break;
          }
          trigger_interrupt(irq);
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

    int fd;

    __u8 *memory;
    size_t memory_size;

    std::unique_ptr<vcpu> cpu;
    device::uart uart;
    virtio::mmio mmio;
  };

} // namespace kvm