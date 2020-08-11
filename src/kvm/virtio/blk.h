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

  class blk : public queue_device<VIRTIO_ID_BLOCK, 1> {
  public:
    struct req_header {
      __u32 type;
      __u32 reserved;
      __u64 sector;
    };

    blk(::kvm::interrupt *irq, __u8 *ptr, std::string filename)
        : queue_device<VIRTIO_ID_BLOCK, 1>(irq, ptr)
        , file(filename, std::ios::binary | std::ios::in | std::ios::out) {

      if (!file.is_open())
        throw std::runtime_error(fmt::format("could not open file {}", filename));

      // 512 blocks
      file.seekg(0, std::ios::end);
      config.capacity = file.tellg() / 512;
      config.size_max = 32768;
      config.seg_max = queue::QUEUE_SIZE_MAX / 2;

      run_thread = std::thread(&blk::run, this);
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

    __u32 features() {
      return (1UL << VIRTIO_BLK_F_SIZE_MAX) |
             (1UL << VIRTIO_BLK_F_SEG_MAX) |
             (1UL << VIRTIO_RING_F_EVENT_IDX);
    }

    __u32 config_generation() {
      return generation;
    }

    void run() {
      while (true) {
        queue::descriptor_elem_t *next = q().next();
        if (next == nullptr) {
          //return;
          continue;
        }

        __u32 len = 0;
        __u32 desc_start = q().avail_id();

        req_header *hdr = q().translate<req_header>(next->addr);
        switch (hdr->type) {
        case VIRTIO_BLK_T_IN: {
          next = &q().desc()->ring[next->next];
          //fmt::print("kvm::virtio::blk read at {:#x} ({})\n", hdr->sector * 512, next->len);

          file.seekg(hdr->sector * 512);
          while (next->flags & VRING_DESC_F_NEXT) {
            file.read(q().translate<char>(next->addr), next->len);
            len += next->len;
            next = &q().desc()->ring[next->next];
          }

          *q().translate<__u8>(next->addr) = VIRTIO_BLK_S_OK;
          len += 1;
          break;
        }

        case VIRTIO_BLK_T_OUT: {
          next = &q().desc()->ring[next->next];
          //fmt::print("kvm::virtio::blk write at {:#x} ({})\n", hdr->sector * 512, next->len);

          file.seekp(hdr->sector * 512);
          while (next->flags & VRING_DESC_F_NEXT) {
            file.write(q().translate<char>(next->addr), next->len);
            file.flush();
            len += next->len;
            next = &q().desc()->ring[next->next];
          }

          *q().translate<__u8>(next->addr) = VIRTIO_BLK_S_OK;
          len += 1;
          break;
        }

        case VIRTIO_BLK_T_GET_ID: {
          next = &q().desc()->ring[next->next];
          memcpy(q().translate<char>(next->addr), DISK_ID, next->len);
          len += next->len;

          next = &q().desc()->ring[next->next];
          *q().translate<__u8>(next->addr) = VIRTIO_BLK_S_OK;
          len += 1;
          break;
        }

        default:
          fmt::print("kvm::virtio::blk unhandled request {}\n", hdr->type);
          continue;
          //return;
        }

        const __u16 idx = q().add_used(desc_start, len);

        if ((idx - 1) == q().avail()->used_event) {
          irq->set_level(true);
        }
      }
    }

    void update(__u8 *ptr) override {
    }

  private:
    std::fstream file;

    virtio_blk_config config;
    __u32 generation = 0;

    std::thread run_thread;
  };

} // namespace kvm::virtio