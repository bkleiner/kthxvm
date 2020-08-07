#pragma once

#include <asm/types.h>

namespace kvm {
  static constexpr __u64 ZERO_PAGE_START = 0x7000;

  static constexpr __u64 BOOT_STACK_START = 0x8000;
  static constexpr __u64 BOOT_STACK_POINTER = 0x8ff0;

  static constexpr __u64 BOOT_GDT_OFFSET = 0x500;
  static constexpr __u64 BOOT_IDT_OFFSET = 0x520;

  static constexpr __u64 PML4_START = 0x9000;
  static constexpr __u64 PDPTE_START = 0xa000;
  static constexpr __u64 PDE_START = 0xb000;

  static constexpr __u64 CMDLINE_START = 0x20000;
  static constexpr __u64 CMDLINE_MAX_SIZE = 0x10000;

  static constexpr __u64 EBDA_START = 0x9fc00;

  static constexpr __u64 HIMEM_START = 0x100000;
}