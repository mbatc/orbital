#pragma once

#include "../core/URI.h"
#include "../core/Map.h"

namespace bfc {
  BFC_API bool readINI(URI const & uri, Map<String, String> * pContent);
  BFC_API bool writeINI(URI const & uri, const Map<String, String> &content);
}
