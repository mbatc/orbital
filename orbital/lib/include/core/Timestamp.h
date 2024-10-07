#pragma once

#include "Core.h"

namespace bfc {
  class BFC_API Timestamp {
  public:
    Timestamp(int64_t length = 0);

    static constexpr int64_t PerSecond = 1000000; // microseconds

    double micros() const;
    double mills() const;
    double secs() const;
    double mins() const;
    double hours() const;

    static Timestamp fromNanos(int64_t len);
    static Timestamp fromMicros(int64_t len);
    static Timestamp fromMillis(int64_t len);
    static Timestamp fromSecs(int64_t len);
    static Timestamp fromMins(int64_t len);
    static Timestamp fromHours(int64_t len);
    static Timestamp now();

    int64_t length = 0;
  };
}
