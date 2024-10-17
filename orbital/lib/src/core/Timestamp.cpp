#include "core/Timestamp.h"

#include <chrono>

namespace bfc {
  Timestamp::Timestamp(int64_t length)
      : length(length) {}

  double Timestamp::micros() const {
    return (double)length / PerSecond * 1000000;
  }

  double Timestamp::mills() const {
    return (double)length / PerSecond * 1000;
  }

  double Timestamp::secs() const {
    return (double)length / PerSecond;
  }

  double Timestamp::mins() const {
    return (double)length / (PerSecond * 60);
  }

  double Timestamp::hours() const {
    return (double)length / (PerSecond * 60 * 60);
  }

  Timestamp Timestamp::fromNanos(int64_t len) {
    return Timestamp(len * PerSecond / 1000000000);
  }

  Timestamp Timestamp::fromMicros(int64_t len) {
    return Timestamp(len * PerSecond / 1000000);
  }

  Timestamp Timestamp::fromMillis(int64_t len) {
    return Timestamp(len * PerSecond / 1000);
  }

  Timestamp Timestamp::fromSecs(int64_t len) {
    return Timestamp(len * PerSecond);
  }

  Timestamp Timestamp::fromMins(int64_t len) {
    return Timestamp(len * PerSecond * 60);
  }

  Timestamp Timestamp::fromHours(int64_t len) {
    return Timestamp(len * PerSecond * 60 * 60);
  }

  Timestamp Timestamp::now() {
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
  }

  Timestamp Timestamp::operator-(Timestamp const & rhs) const {
    return length - rhs.length;
  }

  Timestamp Timestamp::operator+(Timestamp const & rhs) const {
    return length + rhs.length;
  }

  Timestamp Timestamp::operator/(Timestamp const & rhs) const {
    return length / rhs.length;
  }

  Timestamp Timestamp::operator*(Timestamp const & rhs) const {
    return length * rhs.length;
  }

}
