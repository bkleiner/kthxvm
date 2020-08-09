#pragma once

#include <stdexcept>

#include <fmt/format.h>

#include <fcntl.h>
#include <linux/kvm.h>
#include <sys/ioctl.h>

#include "util.h"

namespace kvm {
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

} // namespace kvm