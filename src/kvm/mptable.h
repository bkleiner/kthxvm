#pragma once

#include <linux/kernel.h>
#include <linux/types.h>

#include <apic.h>
#include <mpspec_def.h>

#include "kvm.h"

#define MPTABLE_SIG_FLOATING "_MP_"
#define MPTABLE_OEM "KVMCPU00"
#define MPTABLE_PRODUCTID "0.1         "
#define MPTABLE_PCIBUSTYPE "PCI   "
#define MPTABLE_ISABUSTYPE "ISA   "

#define MPTABLE_STRNCPY(d, s) memcpy(d, s, sizeof(d))

namespace kvm {
  static constexpr __u64 MPTABLE_START = 0x9fc00;
  static constexpr __u64 MPTABLE_MAX_SIZE = (32 << 20);

  static unsigned int gen_cpu_flag(unsigned int cpu, unsigned int ncpu) {
    /* sets enabled/disabled | BSP/AP processor */
    return ((cpu < ncpu) ? CPU_ENABLED : 0) |
           ((cpu == 0) ? CPU_BOOTPROCESSOR : 0x00);
  }

  static unsigned int mpf_checksum(unsigned char *mp, int len) {
    unsigned int sum = 0;

    while (len--)
      sum += *mp++;

    return sum & 0xFF;
  }

  void setup_mptable(vm &v, __u32 ncpus = 1) {
    std::vector<__u8> buf(MPTABLE_MAX_SIZE);

    struct mpc_table *mpc_table = reinterpret_cast<struct mpc_table *>(buf.data());

    MPTABLE_STRNCPY(mpc_table->signature, MPC_SIGNATURE);
    MPTABLE_STRNCPY(mpc_table->oem, MPTABLE_OEM);
    MPTABLE_STRNCPY(mpc_table->productid, MPTABLE_PRODUCTID);

    mpc_table->spec = 4;
    mpc_table->lapic = APIC_ADDR(0);
    mpc_table->oemcount = ncpus;

    /*
	   * CPUs enumeration. Technically speaking we should
	   * ask either host or HV for apic version supported
	   * but for a while we simply put some random value
	   * here.
	   */
    struct mpc_cpu *mpc_cpu = reinterpret_cast<struct mpc_cpu *>(&mpc_table[1]);
    for (__u32 i = 0; i < ncpus; i++) {
      mpc_cpu->type = MP_PROCESSOR;
      mpc_cpu->apicid = i;
      mpc_cpu->apicver = KVM_APIC_VERSION;
      mpc_cpu->cpuflag = gen_cpu_flag(i, ncpus);
      mpc_cpu->cpufeature = 0x600;  /* some default value */
      mpc_cpu->featureflag = 0x201; /* some default value */
      mpc_cpu++;
    }

    void *last_addr = (void *)mpc_cpu;
    __u32 nentries = ncpus;

    /*
	   * ISA bus.
	   * FIXME: Some callback here to obtain real number
	   * of ISQ buses present in system.
	   */
    struct mpc_bus *mpc_bus = reinterpret_cast<struct mpc_bus *>(last_addr);
    const __u32 isabusid = 0;
    mpc_bus->type = MP_BUS;
    mpc_bus->busid = isabusid;
    MPTABLE_STRNCPY(mpc_bus->bustype, MPTABLE_PCIBUSTYPE);

    last_addr = (void *)&mpc_bus[1];
    nentries++;

    /*
	   * IO-APIC chip.
	   */
    __u32 ioapicid = ncpus + 1;
    struct mpc_ioapic *mpc_ioapic = reinterpret_cast<struct mpc_ioapic *>(last_addr);
    mpc_ioapic->type = MP_IOAPIC;
    mpc_ioapic->apicid = ioapicid;
    mpc_ioapic->apicver = KVM_APIC_VERSION;
    mpc_ioapic->flags = MPC_APIC_USABLE;
    mpc_ioapic->apicaddr = IOAPIC_ADDR(0);

    last_addr = (void *)&mpc_ioapic[1];
    nentries++;

    /*
	   * Local IRQs assignment (LINT0, LINT1)
	   */
    struct mpc_intsrc *mpc_intsrc = reinterpret_cast<struct mpc_intsrc *>(last_addr);
    mpc_intsrc->type = MP_LINTSRC;
    mpc_intsrc->irqtype = mp_ExtINT;
    mpc_intsrc->irqtype = mp_INT;
    mpc_intsrc->irqflag = MP_IRQPOL_DEFAULT;
    mpc_intsrc->srcbus = isabusid;
    mpc_intsrc->srcbusirq = 0;
    mpc_intsrc->dstapic = 0; /* FIXME: BSP apic */
    mpc_intsrc->dstirq = 0;  /* LINT0 */

    last_addr = (void *)&mpc_intsrc[1];
    nentries++;

    mpc_intsrc = reinterpret_cast<struct mpc_intsrc *>(last_addr);
    mpc_intsrc->type = MP_LINTSRC;
    mpc_intsrc->irqtype = mp_NMI;
    mpc_intsrc->irqflag = MP_IRQPOL_DEFAULT;
    mpc_intsrc->srcbus = isabusid;
    mpc_intsrc->srcbusirq = 0;
    mpc_intsrc->dstapic = 0; /* FIXME: BSP apic */
    mpc_intsrc->dstirq = 1;  /* LINT1 */

    last_addr = (void *)&mpc_intsrc[1];
    nentries++;

    __u64 real_mpf_intel = __ALIGN_KERNEL((unsigned long)last_addr - (unsigned long)mpc_table, 16);
    struct mpf_intel *mpf_intel = reinterpret_cast<struct mpf_intel *>((unsigned long)mpc_table + real_mpf_intel);

    MPTABLE_STRNCPY(mpf_intel->signature, MPTABLE_SIG_FLOATING);
    mpf_intel->length = 1;
    mpf_intel->specification = 4;
    mpf_intel->physptr = MPTABLE_START;
    mpf_intel->checksum = -mpf_checksum((unsigned char *)mpf_intel, sizeof(*mpf_intel));

    mpc_table->oemcount = nentries;
    mpc_table->length = (__u8 *)last_addr - (__u8 *)mpc_table;
    mpc_table->checksum = -mpf_checksum((unsigned char *)mpc_table, mpc_table->length);

    __u64 size = (unsigned long)mpf_intel + sizeof(*mpf_intel) - (unsigned long)mpc_table;
    memcpy(v.memory_ptr() + MPTABLE_START, mpc_table, size);
  }
} // namespace kvm