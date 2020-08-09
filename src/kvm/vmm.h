#pragma once

#include <atomic>
#include <iostream>
#include <string>
#include <thread>

#include <asm/bootparam.h>
#include <asm/e820.h>

#include "elf/elf.h"

#include "apic.h"
#include "cpuid.h"
#include "irq.h"
#include "mptable.h"
#include "msr.h"
#include "regs.h"
#include "vm.h"

namespace kvm {

  static constexpr __u64 KERNEL_BOOT_FLAG_MAGIC = 0xaa55;
  static constexpr __u64 KERNEL_HDR_MAGIC = 0x53726448;
  static constexpr __u64 KERNEL_LOADER_OTHER = 0xff;
  static constexpr __u64 KERNEL_MIN_ALIGNMENT_BYTES = 0x1000000;

  class vmm {
  public:
    static constexpr __u64 memory_size = 2 * 1024 * 1024 * 1024ul;

    vmm()
        : run_terminal(true)
        , kvm()
        , term()
        , vm(kvm, &term, 1, memory_size, "guest/debian.ext4") {}

    int start(std::string kernel) {

      auto cpuid2 = kvm.get_supported_cpuid(MAX_KVM_CPUID_ENTRIES);
      filter_cpuid_copy(cpuid2);
      vm.get_vcpu().set_cpuid2(cpuid2);

      int entry = elf::load(vm.memory_ptr(), kernel);

      setup_msrs(vm);
      setup_regs(vm, entry, BOOT_STACK_POINTER, ZERO_PAGE_START);
      setup_fpu(vm);
      setup_long_mode(vm);
      setup_lapic(vm);
      setup_irq_routing(vm);
      setup_bootparams();
      setup_mptable(vm);

      std::thread vm_thread(&vm::run, &vm, false);
      std::thread terminal_thread(&vmm::read_terminal, this);

      vm_thread.join();
      run_terminal = false;
      terminal_thread.join();

      return 0;
    }

    void read_terminal() {
      while (run_terminal) {
        if (!term.readable(-1)) {
          break;
        }
        vm.write_stdio();
      }
    }

    void setup_bootparams() {
      const std::string cmdline = "console=ttyS0 virtio_mmio.device=4K@0xd0000000:12 virtio_mmio.device=0x100@0xd0001000:13 reboot=k panic=1 noapic noacpi pci=off root=/dev/vda init=/sbin/init";

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

  private:
    std::atomic_bool run_terminal;

    ::kvm::kvm kvm;
    os::terminal term;
    ::kvm::vm vm;
  };

} // namespace kvm
