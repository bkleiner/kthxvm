#pragma once

#include <fstream>
#include <stdexcept>
#include <string>

#include <fmt/format.h>

namespace kvm {
  std::string errno_msg(std::string ctl) {
    return fmt::format("{}: {} ({})", ctl, strerror(errno), errno);
  }

  void ioctl_err(std::string ctl) {
    throw std::runtime_error(errno_msg(ctl));
  }

  void ioctl_warn(std::string ctl) {
    fmt::print("ioctl_warn {}", errno_msg(ctl));
  }

  int binary_load(char *memory, const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open())
      throw std::runtime_error(fmt::format("could not open file {}", filename));

    std::vector<char> buf((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    memcpy(memory, buf.data(), buf.size());
    return 0;
  }
} // namespace kvm
