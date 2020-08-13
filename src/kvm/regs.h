#pragma once

#include "def.h"
#include "gdt.h"
#include "kvm.h"
#include "layout.h"

namespace kvm {
  static void setup_long_mode(vm &v, vcpu &cpu) {
    struct kvm_sregs sregs = cpu.get_sregs();

    __u64 pml4_addr = PML4_START;
    __u64 *pml4 = (__u64 *)(v.memory_ptr() + pml4_addr);

    __u64 pdpte_addr = PDPTE_START;
    __u64 *pdpte = (__u64 *)(v.memory_ptr() + pdpte_addr);

    __u64 pde_addr = PDE_START;
    __u64 *pde = (__u64 *)(v.memory_ptr() + pde_addr);

    pml4[0] = pdpte_addr | PDE64_PRESENT | PDE64_RW;
    pdpte[0] = pde_addr | PDE64_PRESENT | PDE64_RW;

    for (__u64 i = 0; i < 512; i++)
      pde[i] = (i << 21) + PDE64_PRESENT | PDE64_RW | PDE64_PS;

    sregs.cr0 = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM | CR0_PG;
    sregs.cr3 = pml4_addr;
    sregs.cr4 = CR4_PAE | CR4_OSFXSR | CR4_OSXMMEXCPT;
    sregs.efer = EFER_LME | EFER_LMA | EFER_SCE;

    auto gdt_table = gdt::table();

    auto code_seg = gdt::segment(1, gdt_table[1]);
    auto data_seg = gdt::segment(2, gdt_table[2]);
    auto tss_seg = gdt::segment(3, gdt_table[3]);

    memcpy(v.memory_ptr() + BOOT_GDT_OFFSET, gdt_table.data(), gdt_table.size());
    sregs.gdt.base = BOOT_GDT_OFFSET;
    sregs.gdt.limit = gdt_table.size() * sizeof(__u64) - 1;

    memset(v.memory_ptr() + BOOT_IDT_OFFSET, 0, sizeof(__u64));
    sregs.idt.base = BOOT_IDT_OFFSET;
    sregs.idt.limit = sizeof(__u64) - 1;

    sregs.cs = code_seg;
    sregs.ds = data_seg;
    sregs.es = data_seg;
    sregs.fs = data_seg;
    sregs.gs = data_seg;
    sregs.ss = data_seg;
    sregs.tr = tss_seg;

    cpu.set_sreg(sregs);
  }

  static void setup_regs(vcpu &cpu, __u64 rip, __u64 rsp, __u64 rsi) {
    struct kvm_regs regs;
    memset(&regs, 0, sizeof(regs));
    regs.rflags = 0x2;
    regs.rip = rip; // entry point
    regs.rsp = rsp; // stack
    regs.rsi = rsi; // boot_params

    cpu.set_regs(regs);
  }

  static void setup_fpu(vcpu &cpu) {
    struct kvm_fpu fpu {
      fcw : 0x37f,
          mxcsr : 0x1f80,
    };
    cpu.set_fpu(fpu);
  }
} // namespace kvm