#pragma once

#include <array>
#include <vector>

#include <asm/types.h>

namespace kvm::device {
  class rtc {
  public:
    /*
    * MC146818 RTC registers
    */
    static constexpr __u8 RTC_SECONDS = 0x00;
    static constexpr __u8 RTC_SECONDS_ALARM = 0x01;
    static constexpr __u8 RTC_MINUTES = 0x02;
    static constexpr __u8 RTC_MINUTES_ALARM = 0x03;
    static constexpr __u8 RTC_HOURS = 0x04;
    static constexpr __u8 RTC_HOURS_ALARM = 0x05;
    static constexpr __u8 RTC_DAY_OF_WEEK = 0x06;
    static constexpr __u8 RTC_DAY_OF_MONTH = 0x07;
    static constexpr __u8 RTC_MONTH = 0x08;
    static constexpr __u8 RTC_YEAR = 0x09;
    static constexpr __u8 RTC_CENTURY = 0x32;

    static constexpr __u8 RTC_REG_A = 0x0A;
    static constexpr __u8 RTC_REG_B = 0x0B;
    static constexpr __u8 RTC_REG_C = 0x0C;
    static constexpr __u8 RTC_REG_D = 0x0D;

    std::vector<char> read(__u8 offset, __u8 size) {
      time_t ti;
      time(&ti);

      struct tm *tm = gmtime(&ti);

      std::vector<char> buf(size);

      switch (idx) {
      case RTC_SECONDS:
        buf[0] = bin2bcd(tm->tm_sec);
        break;
      case RTC_MINUTES:
        buf[0] = bin2bcd(tm->tm_min);
        break;
      case RTC_HOURS:
        buf[0] = bin2bcd(tm->tm_hour);
        break;
      case RTC_DAY_OF_WEEK:
        buf[0] = bin2bcd(tm->tm_wday + 1);
        break;
      case RTC_DAY_OF_MONTH:
        buf[0] = bin2bcd(tm->tm_mday);
        break;
      case RTC_MONTH:
        buf[0] = bin2bcd(tm->tm_mon + 1);
        break;
      case RTC_YEAR: {
        int year = tm->tm_year + 1900;
        buf[0] = bin2bcd(year % 100);
        break;
      }
      case RTC_CENTURY: {
        int year = tm->tm_year + 1900;
        buf[0] = bin2bcd(year / 100);
        break;
      }
      default:
        buf[0] = cmos[idx];
        break;
      }

      return buf;
    }

    void write(char *data, __u8 offset, __u8 size) {
      switch (idx) {
      case RTC_REG_C:
      case RTC_REG_D:
        /* Read-only */
        break;
      default:
        cmos[idx] = data[0];
        break;
      }
    }

    void write_index(char *data, __u8 offset, __u8 size) {
      idx = data[0] & ~(1UL << 7);
    }

  private:
    inline __u8 bin2bcd(__u32 val) {
      return ((val / 10) << 4) + val % 10;
    }

    __u8 idx = 0;
    std::array<__u8, 128> cmos;
  };
} // namespace kvm::device
