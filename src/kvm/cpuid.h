#pragma once

#include "kvm.h"

#include <fmt/format.h>

#include <asm/types.h>

union EBChar4 {
  uint32_t u32;
  char c[4];
};

void print_feature(uint32_t test, const char *name) {
  printf("%s - %s\n", (test) ? "YES" : "NO ", name);
}

void print_ebchar4(uint32_t a) {
  EBChar4 a_c;
  a_c.u32 = a;
  printf("%c%c%c%c", a_c.c[0], a_c.c[1], a_c.c[2], a_c.c[3]);
}

static constexpr __u32 LEAFBH_INDEX1_APICID_SHIFT = 6;

namespace leaf_0x1 {
  namespace eax {
    static constexpr __u32 EXTENDED_FAMILY_ID_SHIFT = 20;
    static constexpr __u32 EXTENDED_PROCESSOR_MODEL_SHIFT = 16;
    static constexpr __u32 PROCESSOR_TYPE_SHIFT = 12;
    static constexpr __u32 PROCESSOR_FAMILY_SHIFT = 8;
    static constexpr __u32 PROCESSOR_MODEL_SHIFT = 4;
  } // namespace eax

  namespace ebx {
    // The (fixed) default APIC ID.
    static constexpr __u32 APICID_SHIFT = 24;
    // Bytes flushed when executing CLFLUSH.
    static constexpr __u32 CLFLUSH_SIZE_SHIFT = 8;
    // The logical processor count.
    static constexpr __u32 CPU_COUNT_SHIFT = 16;
  } // namespace ebx

  namespace ecx {
    // DTES64 = 64-bit debug store
    static constexpr __u32 DTES64_SHIFT = 2;
    // MONITOR = Monitor/MWAIT
    static constexpr __u32 MONITOR_SHIFT = 3;
    // CPL Qualified Debug Store
    static constexpr __u32 DS_CPL_SHIFT = 4;
    // 5 = VMX (Virtual Machine Extensions)
    // 6 = SMX (Safer Mode Extensions)
    // 7 = EIST (Enhanced Intel SpeedStep® technology)
    // TM2 = Thermal Monitor 2
    static constexpr __u32 TM2_SHIFT = 8;
    // CNXT_ID = L1 Context ID (L1 data cache can be set to adaptive/shared mode)
    static constexpr __u32 CNXT_ID = 10;
    // SDBG (cpu supports IA32_DEBUG_INTERFACE MSR for silicon debug)
    static constexpr __u32 SDBG_SHIFT = 11;
    static constexpr __u32 FMA_SHIFT = 12;
    // XTPR_UPDATE = xTPR Update Control
    static constexpr __u32 XTPR_UPDATE_SHIFT = 14;
    // PDCM = Perfmon and Debug Capability
    static constexpr __u32 PDCM_SHIFT = 15;
    // 18 = DCA Direct Cache Access (prefetch data from a memory mapped device)
    static constexpr __u32 X2APIC_SHIFT = 21;
    static constexpr __u32 MOVBE_SHIFT = 22;
    static constexpr __u32 TSC_DEADLINE_TIMER_SHIFT = 24;
    static constexpr __u32 OSXSAVE_SHIFT = 27;
    // Cpu is running on a hypervisor.
    static constexpr __u32 HYPERVISOR_SHIFT = 31;
  } // namespace ecx

  namespace edx {
    static constexpr __u32 PSN_SHIFT = 18;  // Processor Serial Number
    static constexpr __u32 DS_SHIFT = 21;   // Debug Store.
    static constexpr __u32 ACPI_SHIFT = 22; // Thermal Monitor and Software Controlled Clock Facilities.
    static constexpr __u32 SS_SHIFT = 27;   // Self Snoop
    static constexpr __u32 HTT_SHIFT = 28;  // Max APIC IDs reserved field is valid
    static constexpr __u32 TM_SHIFT = 29;   // Thermal Monitor.
    static constexpr __u32 PBE_SHIFT = 31;  // Pending Break Enable.
  }                                         // namespace edx
} // namespace leaf_0x1

// Deterministic Cache Parameters Leaf
namespace leaf_0x4 {
  namespace eax {
    static constexpr __u32 CACHE_LEVEL = 5;
    static constexpr __u32 MAX_ADDR_IDS_SHARING_CACHE = 14;
    static constexpr __u32 MAX_ADDR_IDS_IN_PACKAGE = 26;
  } // namespace eax
} // namespace leaf_0x4

// Thermal and Power Management Leaf
namespace leaf_0x6 {
  namespace eax {
    static constexpr __u32 TURBO_BOOST_SHIFT = 1;
  }

  namespace ecx {
    // "Energy Performance Bias" bit.
    static constexpr __u32 EPB_SHIFT = 3;
  } // namespace ecx
} // namespace leaf_0x6

// Structured Extended Feature Flags Enumeration Leaf
namespace leaf_0x7 {
  namespace index0 {
    namespace ebx {
      // 1 = TSC_ADJUST
      static constexpr __u32 SGX_SHIFT = 2;
      static constexpr __u32 BMI1_SHIFT = 3;
      static constexpr __u32 HLE_SHIFT = 4;
      static constexpr __u32 AVX2_SHIFT = 5;
      // FPU Data Pointer updated only on x87 exceptions if 1.
      static constexpr __u32 FPDP_SHIFT = 6;
      // 7 = SMEP (Supervisor-Mode Execution Prevention if 1)
      static constexpr __u32 BMI2_SHIFT = 8;
      // 9 = Enhanced REP MOVSB/STOSB if 1
      // 10 = INVPCID
      static constexpr __u32 INVPCID_SHIFT = 10;
      static constexpr __u32 RTM_SHIFT = 11;
      // Intel® Resource Director Technology (Intel® RDT) Monitoring
      static constexpr __u32 RDT_M_SHIFT = 12;
      // 13 = Deprecates FPU CS and FPU DS values if 1
      // 14 = MPX (Intel® Memory Protection Extensions)
      // RDT = Intel® Resource Director Technology
      static constexpr __u32 RDT_A_SHIFT = 15;
      // AVX-512 Foundation instructions
      static constexpr __u32 AVX512F_SHIFT = 16;
      static constexpr __u32 RDSEED_SHIFT = 18;
      static constexpr __u32 ADX_SHIFT = 19;
      // 20 = SMAP (Supervisor-Mode Access Prevention)
      // 21 & 22 reserved
      // 23 = CLFLUSH_OPT (flushing multiple cache lines in parallel within a single logical processor)
      // 24 = CLWB (Cache Line Write Back)
      // PT = Intel Processor Trace
      static constexpr __u32 PT_SHIFT = 25;
      // AVX512CD = AVX512 Conflict Detection
      static constexpr __u32 AVX512CD_SHIFT = 28;
      // Intel Secure Hash Algorithm Extensions
      static constexpr __u32 SHA_SHIFT = 29;
      // 30 - 32 reserved
    } // namespace ebx

    namespace ecx {
      // 0 = PREFETCHWT1 (move data closer to the processor in anticipation of future use)
      // 1 = reserved
      // 2 = UMIP (User Mode Instruction Prevention)
      // 3 = PKU (Protection Keys for user-mode pages)
      // 4 = OSPKE (If 1, OS has set CR4.PKE to enable protection keys)
      // 5- 16 reserved
      // 21 - 17 = The value of MAWAU used by the BNDLDX and BNDSTX instructions in 64-bit mode.
      static constexpr __u32 RDPID_SHIFT = 22; // Read Processor ID
                                               // 23 - 29 reserved
                                               // SGX_LC = SGX Launch Configuration
      static constexpr __u32 SGX_LC_SHIFT = 30;
      // 31 reserved
    } // namespace ecx
  }   // namespace index0
} // namespace leaf_0x7

namespace leaf_0x80000001 {
  namespace ecx {
    static constexpr __u32 PREFETCH_SHIFT = 8; // 3DNow! PREFETCH/PREFETCHW instructions
    static constexpr __u32 LZCNT_SHIFT = 5;    // advanced bit manipulation
  }                                            // namespace ecx

  namespace edx {
    static constexpr __u32 PDPE1GB_SHIFT = 26; // 1-GByte pages are available if 1.
  }
} // namespace leaf_0x80000001

// Extended Topology Leaf
namespace leaf_0xb {
  static constexpr __u32 LEVEL_TYPE_INVALID = 0;
  static constexpr __u32 LEVEL_TYPE_THREAD = 1;
  static constexpr __u32 LEVEL_TYPE_CORE = 2;
  namespace ecx {
    static constexpr __u32 LEVEL_TYPE_SHIFT = 8; // Shift for setting level type for leaf 11
  }
} // namespace leaf_0xb

namespace kvm {
  static constexpr __u64 MAX_KVM_CPUID_ENTRIES = 80;

  void print_cpuid(kvm &v) {
    auto cpuid2 = v.get_supported_cpuid(MAX_KVM_CPUID_ENTRIES);

    for (__u64 i = 0; i < cpuid2->nent; i++) {
      auto e = cpuid2->entries[i];

      switch (e.function) {
      case 0x0: {
        printf("Vendor    = ");
        print_ebchar4(e.ebx);
        print_ebchar4(e.edx);
        print_ebchar4(e.ecx);
        printf("\n");
        break;
      }
      case 0x1: {
        print_feature(e.edx & (1 << 0), "FPU           (Floating-point Unit on-chip)");
        print_feature(e.edx & (1 << 1), "VME           (Virtual Mode Extension)");
        print_feature(e.edx & (1 << 2), "DE            (Debugging Extension)");
        print_feature(e.edx & (1 << 3), "PSE           (Page Size Extension)");
        print_feature(e.edx & (1 << 4), "TSC           (Time Stamp Counter)");
        print_feature(e.edx & (1 << 5), "MSR           (Model Specific Registers)");
        print_feature(e.edx & (1 << 6), "PAE           (Physical Address Extension)");
        print_feature(e.edx & (1 << 7), "MCE           (Machine Check Exception)");
        print_feature(e.edx & (1 << 8), "CX8           (CMPXCHG8 Instructions)");
        print_feature(e.edx & (1 << 9), "APIC          (On-chip APIC hardware)");
        print_feature(e.edx & (1 << 11), "SEP           (Fast System Call)");
        print_feature(e.edx & (1 << 12), "MTRR          (Memory type Range Registers)");
        print_feature(e.edx & (1 << 13), "PGE           (Page Global Enable)");
        print_feature(e.edx & (1 << 14), "MCA           (Machine Check Architecture)");
        print_feature(e.edx & (1 << 15), "CMOV          (Conditional Move Instruction)");
        print_feature(e.edx & (1 << 16), "PAT           (Page Attribute Table)");
        print_feature(e.edx & (1 << 17), "PSE36         (36bit Page Size Extension");
        print_feature(e.edx & (1 << 18), "PSN           (Processor Serial Number)");
        print_feature(e.edx & (1 << 19), "CLFSH         (CFLUSH Instruction)");
        print_feature(e.edx & (1 << 21), "DS            (Debug Store)");
        print_feature(e.edx & (1 << 22), "ACPI          (Thermal Monitor & Software Controlled Clock)");
        print_feature(e.edx & (1 << 23), "MMX           (Multi-Media Extension)");
        print_feature(e.edx & (1 << 24), "FXSR          (Fast Floating Point Save & Restore)");
        print_feature(e.edx & (1 << 25), "SSE           (Streaming SIMD Extension 1)");
        print_feature(e.edx & (1 << 26), "SSE2          (Streaming SIMD Extension 2)");
        print_feature(e.edx & (1 << 27), "SS            (Self Snoop)");
        print_feature(e.edx & (1 << 28), "HTT           (Hyper Threading Technology)");
        print_feature(e.edx & (1 << 29), "TM            (Thermal Monitor)");
        print_feature(e.edx & (1 << 31), "PBE           (Pend Break Enabled)");
        print_feature(e.ecx & (1 << 0), "SSE3          (Streaming SMD Extension 3)");
        print_feature(e.ecx & (1 << 3), "MW            (Monitor Wait Instruction");
        print_feature(e.ecx & (1 << 4), "CPL           (CPL-qualified Debug Store)");
        print_feature(e.ecx & (1 << 5), "VMX           (Virtual Machine Extensions)");
        print_feature(e.ecx & (1 << 7), "EST           (Enchanced Speed Test)");
        print_feature(e.ecx & (1 << 8), "TM2           (Thermal Monitor 2)");
        print_feature(e.ecx & (1 << 9), "SSSE3         (Supplemental Streaming SIMD Extensions 3)");
        print_feature(e.ecx & (1 << 10), "L1            (L1 Context ID)");
        print_feature(e.ecx & (1 << 12), "FMA3          (Fused Multiply-Add 3-operand Form)");
        print_feature(e.ecx & (1 << 13), "CAE           (Compare And Exchange 16B)");
        print_feature(e.ecx & (1 << 19), "SSE41         (Streaming SIMD Extensions 4.1)");
        print_feature(e.ecx & (1 << 20), "SSE42         (Streaming SIMD Extensions 4.2)");
        print_feature(e.ecx & (1 << 21), "X2APIC        (x2apic)");
        print_feature(e.ecx & (1 << 22), "MOVBE         (MOVBE instruction (big-endian))");
        print_feature(e.ecx & (1 << 23), "POPCNT        (Advanced Bit Manipulation - Bit Population Count Instruction)");
        print_feature(e.ecx & (1 << 25), "AES           (Advanced Encryption Standard)");
        print_feature(e.ecx & (1 << 28), "AVX           (Advanced Vector Extensions)");
        print_feature(e.ecx & (1 << 30), "RDRAND        (Random Number Generator)");
        break;
      }
      case 0x7: {
        print_feature(e.ebx & (1 << 5), "AVX2          (Advanced Vector Extensions 2)");
        print_feature(e.ebx & (1 << 3), "BMI1          (Bit Manipulations Instruction Set 1)");
        print_feature(e.ebx & (1 << 8), "BMI2          (Bit Manipulations Instruction Set 2)");
        print_feature(e.ebx & (1 << 19), "ADX           (Multi-Precision Add-Carry Instruction Extensions)");
        print_feature(e.ebx & (1 << 16), "AVX512F       (512-bit extensions to Advanced Vector Extensions Foundation)");
        print_feature(e.ebx & (1 << 26), "AVX512PFI     (512-bit extensions to Advanced Vector Extensions Prefetch Instructions)");
        print_feature(e.ebx & (1 << 27), "AVX512ERI     (512-bit extensions to Advanced Vector Extensions Exponential and Reciprocal Instructions)");
        print_feature(e.ebx & (1 << 28), "AVX512CDI     (512-bit extensions to Advanced Vector Extensions Conflict Detection Instructions)");
        print_feature(e.ebx & (1 << 29), "SHA           (Secure Hash Algorithm)");
        break;
      }
      case 0x80000000: {
        printf("Vendor    = ");
        print_ebchar4(e.ebx);
        print_ebchar4(e.edx);
        print_ebchar4(e.ecx);
        printf("\n");
        break;
      }
      case 0x80000001: {
        print_feature(e.edx & (1 << 29), "X64           (64-bit Extensions/Long mode)");

        print_feature(e.ecx & (1 << 0), "LAHFSAHF      (LAHF and SAHF instruction support in 64-bit mode)");
        print_feature(e.ecx & (1 << 1), "CmpLegacy     (core multi-processing legacy mode)");
        print_feature(e.ecx & (1 << 2), "SVM           (Secure Virtual Machine)");
        print_feature(e.ecx & (1 << 5), "LZCNT         (Advanced Bit Manipulation - Leading Zero Bit Count Instruction)");
        print_feature(e.ecx & (1 << 6), "SSE4A         (Streaming SIMD Extensions 4a)");
        print_feature(e.ecx & (1 << 7), "MISALIGNSSE   (Misaligned SSE mode)");
        print_feature(e.ecx & (1 << 8), "3DNOWPREFETCH (PREFETCH and PREFETCHW instruction support)");

        print_feature(e.ecx & (1 << 10), "IBS           (Instruction Based Sampling)");
        print_feature(e.ecx & (1 << 11), "XOP           (Extended Operations)");

        print_feature(e.ecx & (1 << 13), "WDT           (Watchdog Timer Support)");

        print_feature(e.ecx & (1 << 15), "LWP           (Light Weight Profiling Support)");
        print_feature(e.ecx & (1 << 16), "FMA4          (Fused Multiply-Add 4-operand Form)");

        print_feature(e.ecx & (1 << 21), "TBM           (Trailing Bit Manipulation Instruction)");
        break;
      }
      case 0x80000002: {
        printf("Processor = ");
        print_ebchar4(e.eax);
        print_ebchar4(e.ebx);
        print_ebchar4(e.ecx);
        print_ebchar4(e.edx);
        break;
      }
      case 0x80000003: {
        print_ebchar4(e.eax);
        print_ebchar4(e.ebx);
        print_ebchar4(e.ecx);
        print_ebchar4(e.edx);
        break;
      }
      case 0x80000004: {
        print_ebchar4(e.eax);
        print_ebchar4(e.ebx);
        print_ebchar4(e.ecx);
        print_ebchar4(e.edx);
        printf("\n");
        break;
      }
      case 0x80000008: {
        printf("Core Count = %d\n", e.ecx & 0xf);
        break;
      }
      default:
        fmt::print("kvm::cpuid function {:#x} eax {:#x} ebx {:#x} ecx {:#x} edx {:#x}\n", e.function, e.eax, e.ebx, e.ecx, e.edx);
      }
    }
  }

  void cpuid_native(
      __u32 function,
      __u32 *eax,
      __u32 *ebx,
      __u32 *ecx,
      __u32 *edx) {

    /* Get our native cpuid. */
    asm volatile("cpuid;"
                 : "=a"(*eax),
                   "=b"(*ebx),
                   "=c"(*ecx),
                   "=d"(*edx)
                 : "a"(function));
  }

  void filter_cpuid_copy(struct kvm_cpuid2 *cpuid) {

    for (__u64 i = 0; i < cpuid->nent; i++) {
      struct kvm_cpuid_entry2 *entry = &cpuid->entries[i];

      switch (entry->function) {
      case 0x0:
        /* Vendor name */
        unsigned int signature[3];
        memcpy(signature, "LKVMLKVMLKVM", 12);
        entry->ebx = signature[0];
        entry->ecx = signature[1];
        entry->edx = signature[2];
        break;
      case 0x1:
        /* Set X86_FEATURE_HYPERVISOR */
        if (entry->index == 0)
          entry->ecx |= (1 << 31);
        break;
      case 0x6:
        /* Clear X86_FEATURE_EPB */
        entry->ecx = entry->ecx & ~(1 << 3);
        break;
      case 0xa: { /* Architectural Performance Monitoring */
        union cpuid10_eax {
          struct {
            unsigned int version_id : 8;
            unsigned int num_counters : 8;
            unsigned int bit_width : 8;
            unsigned int mask_length : 8;
          } split;
          unsigned int full;
        } eax;

        /*
        * If the host has perf system running,
        * but no architectural events available
        * through kvm pmu -- disable perf support,
        * thus guest won't even try to access msr
        * registers.
        */
        if (entry->eax) {
          eax.full = entry->eax;
          if (eax.split.version_id != 2 || !eax.split.num_counters)
            entry->eax = 0;
        }
        break;
      }
      default:
        /* Keep the CPUID function as -is */
        break;
      };
    }
  }

  void filter_cpuid(struct kvm_cpuid2 *cpuid) {
    unsigned int signature[3];

    for (__u64 i = 0; i < cpuid->nent; i++) {
      struct kvm_cpuid_entry2 *e = &cpuid->entries[i];

      switch (e->function) {
      case 0x0:
        unsigned int signature[3];
        memcpy(signature, "GenuineIntel", 12);
        e->ebx = signature[0];
        e->edx = signature[1];
        e->ecx = signature[2];
        break;
      case 0x1: {
        // X86 hypervisor feature
        e->ecx |= 1 << leaf_0x1::ecx::HYPERVISOR_SHIFT;

        // Make sure that HTT is disabled
        e->edx &= ~(1 << leaf_0x1::edx::HTT_SHIFT);
        break;
      }
      case 0x4: {
        auto cache_level = (e->eax >> leaf_0x4::eax::CACHE_LEVEL) & 0b111;
        switch (cache_level) {
        // L1 & L2 Cache
        case 1:
        case 2: {
          // Set the maximum addressable IDS sharing the data cache to zero
          // when you only have 1 vcpu because there are no other threads on
          // the machine to share the data/instruction cache
          // This sets EAX[25:14]
          e->eax &= !(0b111111111111 << leaf_0x4::eax::MAX_ADDR_IDS_SHARING_CACHE);
          break;
        }
        // L3 Cache
        case 3: {
          // Set the maximum addressable IDS sharing the data cache to zero
          // when you only have 1 vcpu because there are no other logical cores on
          // the machine to share the data/instruction cache
          // This sets EAX[25:14]
          e->eax &= !(0b111111111111 << leaf_0x4::eax::MAX_ADDR_IDS_SHARING_CACHE);
          break;
        }
        }

        // Maximum number of addressable IDs for processor cores in the physical package
        // should be the same on all cache levels
        // This sets EAX[31:26]
        e->eax &= !(0b111111 << leaf_0x4::eax::MAX_ADDR_IDS_IN_PACKAGE);
        break;
      }
      case 0x6: {
        // Disable Turbo Boost
        e->eax &= !(1 << leaf_0x6::eax::TURBO_BOOST_SHIFT);
        // Clear X86 EPB feature->  No frequency selection in the hypervisor.
        e->ecx &= !(1 << leaf_0x6::ecx::EPB_SHIFT);
        break;
      }
      case 0xA: {
        // Architectural Performance Monitor Leaf
        // Disable PMU
        e->eax = 0;
        e->ebx = 0;
        e->ecx = 0;
        e->edx = 0;
        break;
      }
      case 0xB: {
        // Hide the actual topology of the underlying host
        switch (e->index) {
        case 0: {
          // No APIC ID at the next level, set EAX to 0
          e->eax = 0;
          // Set the numbers of logical processors to 1
          e->ebx = 1;
          // There are no hyperthreads for 1 VCPU, set the level type = 2 (Core)
          e->ecx = leaf_0xb::LEVEL_TYPE_CORE << leaf_0xb::ecx::LEVEL_TYPE_SHIFT;
          break;
        }
        case 1: {
          // Core Level Processor Topology; index = 1
          e->eax = LEAFBH_INDEX1_APICID_SHIFT;
          // For 1 vCPU, this level is invalid
          e->ebx = 0;
          // ECX[7:0] = e->index; ECX[15:8] = 0 (Invalid Level)
          e->ecx = e->index | (leaf_0xb::LEVEL_TYPE_INVALID << leaf_0xb::ecx::LEVEL_TYPE_SHIFT);
          break;
        }
        default: {
          // Core Level Processor Topology; index >=2
          // No other levels available; This should already be set to correctly,
          // and it is added here as a "re-enforcement" in case we run on
          // different hardware
          e->eax = 0;
          e->ebx = 0;
          e->ecx = e->index;
          break;
        }
        }
        // EDX bits 31..0 contain x2APIC ID of current logical processor
        // x2APIC increases the size of the APIC ID from 8 bits to 32 bits
        //e.edx = cpu_id as u32;
        break;
      }
      }
    }
  }
} // namespace kvm