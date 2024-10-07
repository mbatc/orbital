#pragma once

#include "StringView.h"
#include "Vector.h"

namespace bfc {
  /// You can use this class so split many strings with minimal allocations.
  /// It re-uses the internal buffer for each split.
  class BFC_API StringSplitter {
  public:
    /// Result of the last split operation is written to this member.
    Vector<StringView> tokens;

    Vector<StringView> const & operator()(StringView const & str, Span<StringView> const& delimList, const bool dropEmpty);
    Vector<StringView> const & operator()(StringView const & str, StringView const & delim, const bool dropEmpty);
    Vector<StringView> const & operator()(StringView const & str, Span<StringView> const & delimList, SplitFlags flags = SplitFlags_DropEmpty);
    Vector<StringView> const & operator()(StringView const & str, StringView const & delim, SplitFlags flags = SplitFlags_DropEmpty);
  };
}
