#pragma once

#include <array>

#include <asm/types.h>
#include <linux/kvm.h>
#include <linux/const.h>

namespace kvm
{
  namespace gdt {

    __u64 entry(__u16 flags, __u32 base, __u32 limit) {
      return ((((base)  & _AC(0xff000000,ULL)) << (56-24)) |
      (((flags) & _AC(0x0000f0ff,ULL)) << 40) |     
      (((limit) & _AC(0x000f0000,ULL)) << (48-16)) |
      (((base)  & _AC(0x00ffffff,ULL)) << 16) |     
      (((limit) & _AC(0x0000ffff,ULL))));
    }

    __u64 get_base(__u64 entry) {
      return ((((entry) & 0xFF00000000000000) >> 32)
        | (((entry) & 0x000000FF00000000) >> 16)
        | (((entry) & 0x00000000FFFF0000) >> 16));
    }

    __u32 get_limit(__u64 entry) {
      return ((((entry) & 0x000F000000000000) >> 32) | ((entry) & 0x000000000000FFFF));
    }

    __u8 get_g(__u64 entry) {
      return ((entry & 0x0080000000000000) >> 55);
    }

    __u8 get_db(__u64 entry) {
      return ((entry & 0x0040000000000000) >> 54);
    }

    __u8 get_l(__u64 entry) {
      return ((entry & 0x0020000000000000) >> 53);
    }

    __u8 get_avl(__u64 entry) {
      return ((entry & 0x0010000000000000) >> 52);
    }

    __u8 get_p(__u64 entry) {
      return ((entry & 0x0000800000000000) >> 47);
    }

    __u8 get_dpl(__u64 entry) {
      return ((entry & 0x0000600000000000) >> 45);
    }

    __u8 get_s(__u64 entry) {
      return ((entry & 0x0000100000000000) >> 44);
    }

    __u8 get_type(__u64 entry) {
      return ((entry & 0x00000F0000000000) >> 40);
    }

    struct kvm_segment segment(__u8 index, __u64 entry) {
      struct kvm_segment seg {
        base: get_base(entry),
        limit: get_limit(entry),
        selector: __u16(index * 8),
        type: get_type(entry),
        present: get_p(entry),
        dpl: get_dpl(entry),
        db: get_db(entry),
        s: get_s(entry),
        l: get_l(entry),
        g: get_g(entry),
        avl: get_avl(entry),
        unusable: __u8(get_p(entry) == 0 ? 1 : 0),
        padding: 0,
      };
      return seg;
    }

    static constexpr __u8 NULL_ENTRY = 0;
    static constexpr __u8 CODE_ENTRY = 1;
    static constexpr __u8 DATA_ENTRY = 2;
    static constexpr __u8 TSS_ENTRY  = 3;

    std::array<__u64, 4> table() {
      return std::array<__u64, 4>{
          entry(0, 0, 0),            // NULL
          entry(0xa09b, 0, 0xfffff), // CODE
          entry(0xc093, 0, 0xfffff), // DATA
          entry(0x808b, 0, 0xfffff), // TSS
      };
    }
  }
}