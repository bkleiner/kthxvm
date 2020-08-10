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

#include "device/rtc.h"
#include "device/uart.h"

#include "virtio/blk.h"
#include "virtio/net.h"
#include "virtio/rng.h"

namespace kvm {

  class vmm {
  public:
    static constexpr __u64 memory_size = 2 * 1024 * 1024 * 1024ul;

    vmm()
        : run_terminal(true)
        , kvm()
        , term()
        , vm(kvm, 1, memory_size) {}

    int start(std::string kernel, std::string disk) {
      std::string cmdline = "console=ttyS0";
      cmdline += " virtio_mmio.device=0x1000@0xd0000000:12";
      cmdline += " virtio_mmio.device=0x1000@0xd0001000:13";
      //cmdline += " virtio_mmio.device=0x1000@0xd0002000:14";
      cmdline += " reboot=k panic=1 pci=off";
      cmdline += " root=/dev/vda init=/sbin/init";

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
      setup_bootparams(vm, cmdline, memory_size);
      setup_mptable(vm);

      ttyS0 = vm.add_io_device<device::uart>(0x3f8, 8, 4, &term); // ttyS0
      vm.add_io_device<device::uart>(0x2f8, 8, 3, nullptr);       // ttyS1
      vm.add_io_device<device::uart>(0x3e8, 8, 4, nullptr);       // ttyS2
      vm.add_io_device<device::uart>(0x2e8, 8, 3, nullptr);       // ttyS3

      vm.add_mmio_device<virtio::blk>(0xd0000000, 0x1000, 12, disk);
      vm.add_mmio_device<virtio::rng>(0xd0001000, 0x1000, 13);
      //vm.add_mmio_device<virtio::net>(0xd0002000, 0x1000, 14);

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
        ttyS0->write_stdio();
      }
    }

  private:
    std::atomic_bool run_terminal;

    ::kvm::kvm kvm;
    os::terminal term;
    ::kvm::vm vm;

    device::uart *ttyS0;
  };

} // namespace kvm
