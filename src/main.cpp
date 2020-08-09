#include "kvm/vmm.h"

int main(int argc, char **argv) {

  kvm::vmm vmm;

  //return vmm.start("guest/debian-vmlinux");
  //return vmm.start("guest/ubuntu-vmlinux");
  return vmm.start("guest/new-vmlinux");
  //return vmm.start("guest/vmlinux");
  //return vmm.start("guest/guest.elf");
}