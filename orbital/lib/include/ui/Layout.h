#pragma once

#include "../core/String.h"
#include "../math/MathTypes.h"

namespace bfc {
  namespace ui {
    class BFC_API ScopedID {
    public:
      ScopedID(int64_t id);
      ScopedID(String const & id);
      ~ScopedID();
    };

    using WidgetCallback = std::function<void(Vec2i size)>;
    class BFC_API HStack {
    public:
      /// Create a HStack area.
      HStack(Vec2i size);
      /// Calculate layout and draw the UI.
      ~HStack();
      HStack & add(WidgetCallback const & callback);
      HStack & operator+=(WidgetCallback const & callback);
    private:
      Vec2i                  m_size;
      Vector<WidgetCallback> m_items;
    };

    class BFC_API VStack {
    public:
      /// Create a HStack area.
      VStack(Vec2i size);
      /// Calculate layout and draw the UI.
      ~VStack();
      VStack & add(WidgetCallback const & callback);
      VStack & operator+=(WidgetCallback const & callback);

    private:
      Vec2i                  m_size;
      Vector<WidgetCallback> m_items;
    };
  }
}
