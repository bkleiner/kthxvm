#pragma once

#include <poll.h>
#include <pty.h>
#include <stdexcept>
#include <termios.h>
#include <unistd.h>

#include <asm/types.h>

namespace os {
  class terminal {
  public:
    terminal() {
      if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO))
        throw std::runtime_error("invalid terminal");

      if (tcgetattr(STDIN_FILENO, &term_backup) < 0) {
        throw std::runtime_error("unable to backup termios");
      }

      term = term_backup;
      term.c_iflag &= ~(ICRNL);
      term.c_lflag &= ~(ICANON | ECHO | ISIG);
      if (tcsetattr(STDIN_FILENO, TCSANOW, &term) < 0) {
        throw std::runtime_error("tcsetattr failed");
      }
    }

    ~terminal() {
      tcsetattr(STDIN_FILENO, TCSANOW, &term_backup);
    }

    size_t put(__u8 *data, size_t size) {
      size_t offset = 0;

      while (offset < size) {
        int ret = write(STDOUT_FILENO, data + offset, size);
        if (ret < 0) {
          return size - offset;
        }
        offset += ret;
      }

      return size;
    }

    int get() {
      __u8 c = 0;

      ssize_t ret = -1;
      while (true) {
        ret = read(STDIN_FILENO, &c, 1);
        if ((ret < 0) && ((errno == EAGAIN) || (errno == EINTR)))
          continue;
        break;
      }
      if (ret == 0)
        return -1;

      return c;
    }

    bool readable(int timeout) {
      struct pollfd fd = {
          STDIN_FILENO,
          POLLIN,
          0,
      };
      if (poll(&fd, 1, timeout) <= 0) {
        return false;
      }
      return fd.revents & POLLIN;
    }

  private:
    struct termios term;
    struct termios term_backup;
  };
} // namespace os
