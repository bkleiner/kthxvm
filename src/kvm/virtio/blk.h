#pragma once

#include <vector>

#include <fmt/format.h>

#include <asm/types.h>

#include <linux/virtio_blk.h>
#include <linux/virtio_config.h>

#include <vring_def.h>

#include "device.h"

namespace kvm::virtio {

  constexpr char DISK_ID[] = "kthxvmkthxvmkthxvmdisk";

  class blk : public queue_device<1> {
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
      config.size_max = 32768;
      config.seg_max = queue::QUEUE_SIZE_MAX / 2;
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
      return (1UL << VIRTIO_BLK_F_SIZE_MAX) |
             (1UL << VIRTIO_BLK_F_SEG_MAX) |
             (1UL << VIRTIO_RING_F_EVENT_IDX);
    }

    __u32 config_generation() {
      return generation;
    }

    bool update(__u8 *ptr) {
      if (!q().notify) {
        return false;
      }
      q().notify = false;

      queue::descriptor_elem_t *next = q().next(ptr);
      if (next == nullptr) {
        return false;
      }

      __u32 len = 0;
      __u32 desc_start = q().avail_id(ptr);

      req_header *hdr = reinterpret_cast<req_header *>(ptr + next->addr);
      switch (hdr->type) {
      case VIRTIO_BLK_T_IN: {
        next = &q().desc(ptr)->ring[next->next];
        //fmt::print("kvm::virtio::blk read at {:#x} ({})\n", hdr->sector * 512, next->len);

        file.seekg(hdr->sector * 512);
        while (next->flags & VRING_DESC_F_NEXT) {
          file.read((char *)(ptr + next->addr), next->len);
          len += next->len;
          next = &q().desc(ptr)->ring[next->next];
        }

        *(ptr + next->addr) = VIRTIO_BLK_S_OK;
        len += 1;
        break;
      }

      case VIRTIO_BLK_T_OUT: {
        next = &q().desc(ptr)->ring[next->next];
        //fmt::print("kvm::virtio::blk write at {:#x} ({})\n", hdr->sector * 512, next->len);

        file.seekp(hdr->sector * 512);
        while (next->flags & VRING_DESC_F_NEXT) {
          file.write((char *)(ptr + next->addr), next->len);
          file.flush();
          len += next->len;
          next = &q().desc(ptr)->ring[next->next];
        }

        *(ptr + next->addr) = VIRTIO_BLK_S_OK;
        len += 1;
        break;
      }

      case VIRTIO_BLK_T_GET_ID: {
        next = &q().desc(ptr)->ring[next->next];
        memcpy((char *)(ptr + next->addr), DISK_ID, next->len);
        len += next->len;

        next = &q().desc(ptr)->ring[next->next];
        *(ptr + next->addr) = VIRTIO_BLK_S_OK;
        len += 1;
        break;
      }

      default:
        fmt::print("kvm::virtio::blk unhandled request {}\n", hdr->type);
        return false;
      }

      return (q().add_used(ptr, desc_start, len) - 1) == q().avail(ptr)->used_event;
    }

  private:
    std::fstream file;

    virtio_blk_config config;
    __u32 generation = 0;
  };

} // namespace kvm::virtio