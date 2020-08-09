#pragma once

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

extern "C" {
#define class clazz
#include <linux/virtio_net.h>
#undef class
}

#include "kvm/util.h"

#include "device.h"

namespace kvm::virtio {

  class net : public queue_device<VIRTIO_ID_NET, 3> {
  public:
    static constexpr __u8 rx_queue = 0;
    static constexpr __u8 tx_queue = 1;
    static constexpr __u8 ctrl_queue = 2;

    net() {
      //tap = create_tap("tap0");
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
             1UL << VIRTIO_NET_F_HOST_TSO6 |
             1UL << VIRTIO_NET_F_GUEST_TSO4 |
             1UL << VIRTIO_NET_F_GUEST_TSO6 |
             1UL << VIRTIO_NET_F_CTRL_VQ;
    }

    __u32 config_generation() {
      return generation;
    }

    bool update_rx(__u8 *ptr, queue &q) {
      queue::descriptor_elem_t *next = q.next(ptr);
      if (next == nullptr) {
        return false;
      }

      __u32 len = 0;
      __u32 desc_start = q.avail_id(ptr);

      virtio_net_hdr *hdr = reinterpret_cast<virtio_net_hdr *>(ptr + next->addr);

      q.add_used(ptr, desc_start, len);
      return true;
    }

    bool update_tx(__u8 *ptr, queue &q) {
      queue::descriptor_elem_t *next = q.next(ptr);
      if (next == nullptr) {
        return false;
      }

      __u32 len = 0;
      __u32 desc_start = q.avail_id(ptr);

      virtio_net_hdr *hdr = reinterpret_cast<virtio_net_hdr *>(ptr + next->addr);

      q.add_used(ptr, desc_start, len);
      return true;
    }

    bool update_ctrl(__u8 *ptr, queue &q) {
      queue::descriptor_elem_t *next = q.next(ptr);
      if (next == nullptr) {
        return false;
      }

      __u32 len = 0;
      __u32 desc_start = q.avail_id(ptr);

      q.add_used(ptr, desc_start, len);
      return true;
    }

    bool update(__u8 *ptr) {
      update_rx(ptr, q(rx_queue));
      update_tx(ptr, q(tx_queue));
      update_ctrl(ptr, q(ctrl_queue));
      return false;
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

      int offload = TUN_F_CSUM | TUN_F_TSO4 | TUN_F_TSO6;
      if (ioctl(fd, TUNSETOFFLOAD, offload) < 0)
        ioctl_err("TUNSETOFFLOAD");

      return fd;
    }

    int tap;

    virtio_net_config config;
    __u32 generation = 0;
  };

} // namespace kvm::virtio