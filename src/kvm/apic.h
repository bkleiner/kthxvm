#pragma once

#include "kvm.h"
#include "vm.h"

namespace kvm {
  static constexpr __u64 APIC_LVT0 = 0x350;
  static constexpr __u64 APIC_LVT1 = 0x360;

  static constexpr __u32 APIC_MODE_NMI = 0x4;
  static constexpr __u32 APIC_MODE_EXTINT = 0x7;

  __u32 apic_delivery_mode(__u32 reg, __u32 mode) {
    return (((reg) & !0x700) | ((mode) << 8));
  }

  void setup_lapic(vm &vm) {
    auto lapic = vm.get_vcpu().get_lapic();

    __u32 *lvt_lint0 = reinterpret_cast<__u32 *>(lapic.regs + APIC_LVT0);
    *lvt_lint0 = apic_delivery_mode(*lvt_lint0, APIC_MODE_EXTINT);

    __u32 *lvt_lint1 = reinterpret_cast<__u32 *>(lapic.regs + APIC_LVT1);
    *lvt_lint1 = apic_delivery_mode(*lvt_lint1, APIC_MODE_NMI);

    vm.get_vcpu().set_lapic(lapic);
  }
} // namespace kvm