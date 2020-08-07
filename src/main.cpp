#include "elf/elf.h"

#include "kvm/apic.h"
#include "kvm/cpuid.h"
#include "kvm/msr.h"
#include "kvm/regs.h"

#include <asm/bootparam.h>
#include <asm/e820.h>

#define KERNEL_BOOT_FLAG_MAGIC 0xaa55
#define KERNEL_HDR_MAGIC 0x53726448
#define KERNEL_LOADER_OTHER 0xff
#define KERNEL_MIN_ALIGNMENT_BYTES 0x1000000

int binary_load(char *memory, const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open())
    throw std::runtime_error(fmt::format("could not open file {}", filename));

  std::vector<char> buf((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  memcpy(memory, buf.data(), buf.size());
  return 0;
}

int main(int argc, char **argv) {
  std::string cmdline = "console=uart,io,0x3f8 virtio_mmio.device=4K@0xd0000000:5 reboot=k panic=1 pci=off";
  __u64 memory_size = 1024 * 1024 * 1024;

  kvm::kvm kvm;
  kvm::vm vm(kvm, 1, memory_size);

  auto cpuid2 = kvm.get_supported_cpuid(kvm::MAX_KVM_CPUID_ENTRIES);
  kvm::filter_cpuid(cpuid2);
  vm.get_vcpu().set_cpuid2(cpuid2);

  int entry = elf::load(vm.memory_ptr(), "guest/vmlinux");
  // int entry = elf::load(vm.memory_ptr(), "guest/guest.elf");
  // int entry = binary_load(vm.memory_ptr(), "guest/guest64.img");

  kvm::setup_msrs(vm);
  kvm::setup_regs(vm, entry, kvm::BOOT_STACK_POINTER, kvm::ZERO_PAGE_START);
  kvm::setup_fpu(vm);
  kvm::setup_long_mode(vm);
  kvm::setup_lapic(vm);

  struct boot_params *boot = reinterpret_cast<struct boot_params *>(vm.memory_ptr() + kvm::ZERO_PAGE_START);
  memset(boot, 0, sizeof(struct boot_params));

  memcpy(vm.memory_ptr() + kvm::CMDLINE_START, cmdline.data(), cmdline.size());

  boot->e820_table[boot->e820_entries].addr = 0;
  boot->e820_table[boot->e820_entries].size = kvm::EBDA_START;
  boot->e820_table[boot->e820_entries].type = E820_RAM;
  boot->e820_entries += 1;

  boot->e820_table[boot->e820_entries].addr = kvm::HIMEM_START;
  boot->e820_table[boot->e820_entries].size = memory_size - kvm::HIMEM_START;
  boot->e820_table[boot->e820_entries].type = E820_RAM;
  boot->e820_entries += 1;

  boot->hdr.type_of_loader = KERNEL_LOADER_OTHER;
  boot->hdr.boot_flag = KERNEL_BOOT_FLAG_MAGIC;
  boot->hdr.header = KERNEL_HDR_MAGIC;
  boot->hdr.cmd_line_ptr = kvm::CMDLINE_START;
  boot->hdr.cmdline_size = cmdline.size();
  boot->hdr.kernel_alignment = KERNEL_MIN_ALIGNMENT_BYTES;

  return vm.run(false);
}