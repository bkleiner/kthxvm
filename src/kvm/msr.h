#pragma once

#include "def.h"
#include "kvm.h"

namespace kvm {
  void setup_msrs(vm &v) {
    std::vector<struct kvm_msr_entry> entries;

    entries.emplace_back(kvm_msr_entry{MSR_IA32_SYSENTER_CS, 0, 0x0});
    entries.emplace_back(kvm_msr_entry{MSR_IA32_SYSENTER_ESP, 0, 0x0});
    entries.emplace_back(kvm_msr_entry{MSR_IA32_SYSENTER_EIP, 0, 0x0});

    // x86_64 specific msrs, we only run on x86_64 not x86.
    entries.emplace_back(kvm_msr_entry{MSR_STAR, 0, 0x0});
    entries.emplace_back(kvm_msr_entry{MSR_CSTAR, 0, 0x0});
    entries.emplace_back(kvm_msr_entry{MSR_KERNEL_GS_BASE, 0, 0x0});
    entries.emplace_back(kvm_msr_entry{MSR_SYSCALL_MASK, 0, 0x0});
    entries.emplace_back(kvm_msr_entry{MSR_LSTAR, 0, 0x0});
    // end of x86_64 specific code

    entries.emplace_back(kvm_msr_entry{MSR_IA32_TSC, 0, 0x0});
    entries.emplace_back(kvm_msr_entry{MSR_IA32_MISC_ENABLE, 0, MSR_IA32_MISC_ENABLE_FAST_STRING});

    std::vector<__u8> buf(sizeof(kvm_msrs) + entries.size() * sizeof(kvm_msr_entry));
    struct kvm_msrs *msrs = reinterpret_cast<kvm_msrs *>(buf.data());

    memcpy(buf.data() + sizeof(kvm_msrs), entries.data(), entries.size() * sizeof(kvm_msr_entry));
    msrs->nmsrs = entries.size();

    v.get_vcpu().set_msrs(msrs);
  }
} // namespace kvm