#pragma once

#include <fstream>
#include <string>
#include <vector>

#include <string.h>

#include <asm/types.h>
#include <elf.h>

#include <fmt/format.h>

namespace elf {
  int load(__u8 *memory, const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open())
      throw std::runtime_error(fmt::format("could not open file {}", filename));

    std::vector<char> ehdr_buf(sizeof(Elf64_Ehdr));
    file.read(ehdr_buf.data(), ehdr_buf.size());

    Elf64_Ehdr *header = (Elf64_Ehdr *)ehdr_buf.data();
    if (memcmp(header->e_ident, ELFMAG, SELFMAG) != 0)
      throw std::runtime_error("not an elf file");

    std::vector<char> phdr_buf(header->e_phentsize * header->e_phnum);
    file.seekg(header->e_phoff);
    file.read(phdr_buf.data(), phdr_buf.size());

    Elf64_Phdr *phdr = reinterpret_cast<Elf64_Phdr *>(phdr_buf.data());
    for (int i = 0; i < header->e_phnum; i++) {
      if (phdr[i].p_type != PT_LOAD)
        continue;

      if (phdr[i].p_filesz > phdr[i].p_memsz)
        return -EINVAL;

      if (!phdr[i].p_filesz)
        return -EINVAL;

      fmt::print("elf: loading segment {:#x} at {:#x} (p_memsz {})\n", phdr[i].p_offset, phdr[i].p_paddr, phdr[i].p_memsz);
      file.seekg(phdr[i].p_offset);
      file.read((char *)memory + phdr[i].p_paddr, phdr[i].p_filesz);
    }

    fmt::print("elf: loaded binary with entry {:#x}\n", header->e_entry);
    return header->e_entry;
  }
} // namespace elf