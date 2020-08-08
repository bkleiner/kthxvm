#pragma once

#include <vector>

#include <fmt/format.h>

#include <asm/types.h>
#include <linux/virtio_blk.h>
#include <linux/virtio_config.h>

#include "device.h"

namespace kvm::virtio {

  class blk : public device {
  public:
    struct req_header {
      __u32 type;
      __u32 reserved;
      __u64 sector;
    };

    blk(std::string filename)
        : file(filename, std::ios::binary | std::ios::in | std::ios::out) {

      if (!file.is_open())
        throw std::runtime_error(fmt::format("could not open file {}", filename));

      // 512 blocks
      file.seekg(0, std::ios::end);
      config.capacity = file.tellg() / 512;
    }

    std::vector<__u8> read(__u64 offset, __u32 size) {
      std::vector<__u8> buf(size);

      if ((offset + size) > sizeof(virtio_blk_config)) {
        fmt::print("kvm::virtio::blk invalid config read at {:#x}\n", offset);
        return buf;
      }

      memcpy(buf.data(), (uint8_t *)(&config) + offset, size);
      return buf;
    }

    void write(__u8 *data, __u64 offset, __u32 size) {
      if ((offset + size) > sizeof(virtio_blk_config)) {
        fmt::print("kvm::virtio::blk invalid config write at {:#x}\n", offset);
        return;
      }

      memcpy((uint8_t *)(&config) + offset, data, size);
      generation++;
    }

    __u32 device_id() {
      return VIRTIO_ID_BLOCK;
    }

    __u32 features() {
      return 0;
    }

    __u32 config_generation() {
      return generation;
    }

    bool update(queue &q, __u8 *ptr) {
      queue::descriptor_elem_t *next = q.next(ptr);
      if (next == nullptr) {
        return false;
      }

      __u32 len = 0;
      __u32 desc_start = q.avail_id(ptr);
      req_header *hdr = reinterpret_cast<req_header *>(ptr + next->addr);
      switch (hdr->type) {
      case VIRTIO_BLK_T_IN: {
        //fmt::print("kvm::virtio::blk read at {:#x}\n", hdr->sector * 512);
        next = &q.descriptors(ptr)->ring[next->next];

        file.seekg(hdr->sector * 512);
        file.read((char *)(ptr + next->addr), next->len);
        len += next->len;

        next = &q.descriptors(ptr)->ring[next->next];
        *(ptr + next->addr) = VIRTIO_BLK_S_OK;
        len += 1;
        break;
      }

      case VIRTIO_BLK_T_OUT: {
        //fmt::print("kvm::virtio::blk write at {:#x}\n", hdr->sector * 512);
        next = &q.descriptors(ptr)->ring[next->next];

        file.seekp(hdr->sector * 512);
        file.write((char *)(ptr + next->addr), next->len);
        len += next->len;

        next = &q.descriptors(ptr)->ring[next->next];
        *(ptr + next->addr) = VIRTIO_BLK_S_OK;
        len += 1;
        break;
      }

      default:
        fmt::print("kvm::virtio::blk unhandled request {}\n", hdr->type);
        break;
      }

      q.add_used(ptr, desc_start, len);
      return true;
    }

  private:
    std::fstream file;

    virtio_blk_config config;
    __u32 generation = 0;
  };

} // namespace kvm::virtio