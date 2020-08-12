#pragma once

#define VIRTIO_NET_NO_LEGACY

#include <algorithm>
#include <vector>

#include <fmt/format.h>

#include <fcntl.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <linux/if_tun.h>
#include <net/if.h>

#include <asm/types.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>

extern "C" {
#define class clazz
#include <linux/virtio_net.h>
#undef class
}

#include "kvm/util.h"

#include "device.h"

namespace kvm::virtio {

  static int exec_script(const char *script, const char *tap_name) {
    pid_t pid = fork();
    if (pid == 0) {
      execl(script, script, tap_name, NULL);
      _exit(1);
    } else {
      int status;
      waitpid(pid, &status, 0);
      if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        return -1;
      }
    }
    return 0;
  }

  class net : public queue_device<VIRTIO_ID_NET, 3> {
  public:
    static constexpr __u8 rx_queue = 0;
    static constexpr __u8 tx_queue = 1;
    static constexpr __u8 ctrl_queue = 2;

    net(::kvm::interrupt *irq, __u8 *ptr)
        : queue_device<VIRTIO_ID_NET, 3>(irq, ptr) {
      tap = create_tap("tap0");
      memcpy(config.mac, default_mac, 6);

      run_tx_thread = std::thread(&net::run_tx, this);
      run_rx_thread = std::thread(&net::run_rx, this);
    }

    std::vector<__u8> read(__u64 offset, __u32 size) {
      std::vector<__u8> buf(size);

      if ((offset + size) > sizeof(virtio_net_config)) {
        fmt::print("kvm::virtio::net invalid config read at {:#x}\n", offset);
        return buf;
      }

      memcpy(buf.data(), (uint8_t *)(&config) + offset, size);
      return buf;
    }

    void write(__u8 *data, __u64 offset, __u32 size) {
      if ((offset + size) > sizeof(virtio_net_config)) {
        fmt::print("kvm::virtio::net invalid config write at {:#x}\n", offset);
        return;
      }

      memcpy((uint8_t *)(&config) + offset, data, size);
      generation++;
    }

    __u32 features() {
      return 1UL << VIRTIO_NET_F_MAC |
             1UL << VIRTIO_NET_F_CSUM |
             1UL << VIRTIO_NET_F_HOST_TSO4 |
             1UL << VIRTIO_NET_F_GUEST_TSO4 |
             1UL << VIRTIO_NET_F_HOST_TSO6 |
             1UL << VIRTIO_NET_F_GUEST_TSO6 |
             1UL << VIRTIO_NET_F_HOST_UFO |
             1UL << VIRTIO_NET_F_GUEST_UFO;
    }

    __u32 config_generation() {
      return generation;
    }

    void update_rx(queue &q) {
      queue::descriptor_elem_t *next = q.next();
      if (next == nullptr) {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        return;
      }

      if (!::kvm::poll_fd_in(tap, -1)) {
        return;
      }

      const __u64 buffer_len = 65550 + sizeof(virtio_net_hdr_v1);
      static __u8 buffer[buffer_len];

      ssize_t size = ::read(tap, buffer, buffer_len);
      if (size < 0) {
        auto msg = errno_msg("tap read");
        ioctl_warn("tap read");
      }

      __u32 len = 0;
      __u32 written = 0;
      __u32 desc_start = q.avail_id();
      while (true) {
        size_t write_len = std::min(__u32(size - len), next->len);
        memcpy(q.translate<__u8>(next->addr), buffer + len, write_len);
        len += write_len;

        if (len == size)
          break;

        if (!(next->flags & VRING_DESC_F_NEXT))
          break;

        next = &q.desc()->ring[next->next];
      }

      q.add_used(desc_start, len);
      irq->set_level(true);
    }

    void update_tx(queue &q) {
      queue::descriptor_elem_t *next = q.next();
      if (next == nullptr) {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        return;
      }

      __u32 len = 0;
      __u32 written = 0;
      __u32 desc_start = q.avail_id();

      const __u64 buffer_len = 65550 + sizeof(virtio_net_hdr_v1);
      static __u8 buffer[buffer_len];

      while (true) {
        memcpy(buffer + written, q.translate<__u8>(next->addr), next->len);
        written += next->len;

        if (!(next->flags & VRING_DESC_F_NEXT))
          break;

        next = &q.desc()->ring[next->next];
      }

      ssize_t ret = ::write(tap, buffer, written);
      if (ret < 0) {
        auto msg = errno_msg("tap write");
        ioctl_warn("tap write");
      }
      len += ret;

      q.add_used(desc_start, len);
      irq->set_level(true);
    }

    void update_ctrl(__u8 *ptr, queue &q) {
      queue::descriptor_elem_t *next = q.next();
      if (next == nullptr) {
        return;
      }

      __u32 len = 0;
      __u32 desc_start = q.avail_id();

      q.add_used(desc_start, len);
      irq->set_level(true);
    }

    void run_rx() {
      while (true) {
        update_rx(q(rx_queue));
      }
    }

    void run_tx() {
      while (true) {
        update_tx(q(tx_queue));
      }
    }

    void update(__u8 *ptr) {

      //update_ctrl(ptr, q(ctrl_queue));
    }

  private:
    int create_tap(const char *name) {
      const char *tap_file = "/dev/net/tun";
      int fd = open(tap_file, O_RDWR);

      struct ifreq req = {};
      memset(&req, 0, sizeof(struct ifreq));

      req.ifr_flags = IFF_TAP | IFF_NO_PI | IFF_VNET_HDR;
      strncpy(req.ifr_name, name, sizeof(req.ifr_name));

      if (ioctl(fd, TUNSETIFF, &req) < 0)
        ioctl_err("TUNSETIFF");

      int offload = TUN_F_CSUM | TUN_F_TSO4 | TUN_F_TSO6 | TUN_F_UFO;
      if (ioctl(fd, TUNSETOFFLOAD, offload) < 0)
        ioctl_err("TUNSETOFFLOAD");

      int hdr_len = sizeof(virtio_net_hdr_v1);
      if (ioctl(fd, TUNSETVNETHDRSZ, &hdr_len) < 0)
        ioctl_err("TUNSETVNETHDRSZ");

      if (exec_script(".vscode/setup_tap.sh", name) < 0)
        ioctl_err("exec_script");

      return fd;
    }

    int tap;

    virtio_net_config config;
    __u32 generation = 0;

    const __u8 default_mac[6] = {0x02, 0x15, 0x15, 0x15, 0x15, 0x15};

    std::thread run_tx_thread;
    std::thread run_rx_thread;
  };

} // namespace kvm::virtio