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

#include "device/i8042.h"
#include "device/rtc.h"
#include "device/uart.h"

#include "virtio/blk.h"
#include "virtio/net.h"
#include "virtio/rng.h"

namespace kvm {

  class vmm {
  public:
    static constexpr __u64 CPU_COUNT = 2;
    static constexpr __u64 MEMORY_SIZE_MB = 4 * 1024ul;
    static constexpr __u64 MB_SHIFT = (20);

    vmm()
        : run_terminal(true)
        , kvm()
        , term()
        , vm(kvm, CPU_COUNT, MEMORY_SIZE_MB << MB_SHIFT) {}

    int start(std::string kernel, std::string disk) {
      std::string cmdline = "console=ttyS0";
      cmdline += " virtio_mmio.device=0x1000@0xd0000000:12";
      cmdline += " virtio_mmio.device=0x1000@0xd0001000:13";
      cmdline += " virtio_mmio.device=0x1000@0xd0002000:14";
      cmdline += " reboot=k panic=1 pci=off";
      cmdline += " i8042.noaux i8042.nomux i8042.nopnp i8042.dumbkbd";
      cmdline += " root=/dev/vda init=/sbin/init";

      auto cpuid2 = kvm.get_supported_cpuid(MAX_KVM_CPUID_ENTRIES);
      filter_cpuid_copy(cpuid2);
      for (size_t i = 0; i < CPU_COUNT; i++) {
        vm.get_vcpu(i).set_cpuid2(cpuid2);
      }

      int entry = elf::load(vm.memory_ptr(), kernel);

      setup_irq_routing(vm);

      for (size_t i = 0; i < CPU_COUNT; i++) {
        setup_msrs(vm.get_vcpu(i));
        setup_fpu(vm.get_vcpu(i));
        setup_lapic(vm.get_vcpu(i));
      }

      setup_regs(vm.get_vcpu(0), entry, BOOT_STACK_POINTER, ZERO_PAGE_START);
      setup_long_mode(vm, vm.get_vcpu(0));
      setup_bootparams(vm, cmdline);
      setup_mptable(vm, CPU_COUNT);

      // keyboard
      vm.add_io_device<device::i8042>(0x60, 5, 1);

      ttyS0 = vm.add_io_device<device::uart>(0x3f8, 8, 4, &term); // ttyS0
      vm.add_io_device<device::uart>(0x2f8, 8, 3, nullptr);       // ttyS1
      vm.add_io_device<device::uart>(0x3e8, 8, 4, nullptr);       // ttyS2
      vm.add_io_device<device::uart>(0x2e8, 8, 3, nullptr);       // ttyS3

      vm.add_mmio_device<virtio::blk>(0xd0000000, 0x1000, 12, disk);
      vm.add_mmio_device<virtio::rng>(0xd0001000, 0x1000, 13);
      vm.add_mmio_device<virtio::net>(0xd0002000, 0x1000, 14);

      std::array<std::thread, CPU_COUNT> vm_threads;
      for (size_t i = 0; i < CPU_COUNT; i++) {
        vm_threads[i] = std::thread(&vm::run_cpu, &vm, i, false);
      }

      std::thread terminal_thread(&vmm::read_terminal, this);

      vm_threads[0].join();
      vm.stop();

      for (size_t i = 1; i < CPU_COUNT; i++) {
        vm_threads[i].join();
      }

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
