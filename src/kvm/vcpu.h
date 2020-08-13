#pragma once

#include <memory>
#include <stdexcept>
#include <vector>

#include <fmt/format.h>

#include <fcntl.h>
#include <linux/kvm.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "util.h"

namespace kvm {

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
    retry_label:
      if (ioctl(fd, KVM_RUN, 0) < 0) {
        if (errno == 11) {
          ioctl_warn("KVM_RUN");
          goto retry_label;
        } else if (errno != 4) {
          ioctl_err("KVM_RUN");
        } else {
          ioctl_warn("KVM_RUN");
        }
      }

      return kvm_run;
    }

  private:
    int fd;
    struct kvm_run *kvm_run;
  };

} // namespace kvm
