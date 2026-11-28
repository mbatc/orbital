#pragma once

namespace engine {
  enum class EntityID : uint64_t { Invalid = 0 };

  constexpr EntityID InvalidEntity = EntityID::Invalid;
}
