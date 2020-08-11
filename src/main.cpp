#include "kvm/vmm.h"

int main(int argc, char **argv) {
  kvm::vmm vmm;
  return vmm.start("guest/new-vmlinux", "guest/debian.ext4");
}