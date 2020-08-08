#pragma once

#include "kvm.h"

namespace kvm {
  static constexpr __u32 IRQCHIP_MASTER = 0;
  static constexpr __u32 IRQCHIP_SLAVE = 1;
  static constexpr __u32 IRQCHIP_IOAPIC = 2;

  struct kvm_irq_routing_entry create_irq_routing_entry(__u32 gsi, __u32 type, __u32 irqchip, __u32 pin) {
    struct kvm_irq_routing_entry entry;
    entry.gsi = gsi;
    entry.type = type;
    entry.u.irqchip.irqchip = irqchip;
    entry.u.irqchip.pin = pin;
    return entry;
  }

  void setup_irq_routing(vm &vm) {

    std::vector<struct kvm_irq_routing_entry> entries;

    /* Hook first 8 GSIs to master IRQCHIP */
    for (__u32 i = 0; i < 8; i++)
      if (i != 2)
        entries.push_back(create_irq_routing_entry(i, KVM_IRQ_ROUTING_IRQCHIP, IRQCHIP_MASTER, i));

    /* Hook next 8 GSIs to slave IRQCHIP */
    for (__u32 i = 8; i < 16; i++)
      entries.push_back(create_irq_routing_entry(i, KVM_IRQ_ROUTING_IRQCHIP, IRQCHIP_SLAVE, i - 8));

    /* Last but not least, IOAPIC */
    for (__u32 i = 0; i < 24; i++) {
      if (i == 0)
        entries.push_back(create_irq_routing_entry(i, KVM_IRQ_ROUTING_IRQCHIP, IRQCHIP_IOAPIC, 2));
      else if (i != 2)
        entries.push_back(create_irq_routing_entry(i, KVM_IRQ_ROUTING_IRQCHIP, IRQCHIP_IOAPIC, i));
    }

    std::vector<__u8> router_data(sizeof(struct kvm_irq_routing) + entries.size() * sizeof(struct kvm_irq_routing_entry));
    struct kvm_irq_routing *router = reinterpret_cast<struct kvm_irq_routing *>(router_data.data());

    router->nr = entries.size();
    router->flags = 0;
    memcpy(router->entries, entries.data(), entries.size() * sizeof(struct kvm_irq_routing_entry));

    vm.set_gsi_routing(router);
  }

} // namespace kvm
