#include "ui/Layout.h"
#include "ui/Context.h"

namespace bfc {
  namespace ui {
    ScopedID::ScopedID(int64_t id) {
      ImGui::PushID((int)id);
    }

    ScopedID::ScopedID(String const & id) {
      ImGui::PushID(id.c_str());
    }

    ScopedID::~ScopedID() {
      ImGui::PopID();
    }

    HStack::HStack(Vec2i size)
      : m_size(size) {}

    HStack::~HStack() {
      int64_t count = m_items.size();
    }

    HStack & HStack::operator+=(WidgetCallback const & callback) {
      return add(callback);
    }

    HStack & HStack::add(WidgetCallback const & callback) {
      m_items.pushBack(callback);
      return *this;
    }

    VStack::VStack(Vec2i size)
      : m_size(size) {}

    VStack::~VStack() {

    }

    VStack & VStack::add(WidgetCallback const & callback) {
      m_items.pushBack(callback);
      return *this;
    }

    VStack & VStack::operator+=(WidgetCallback const & callback) {
      return add(callback);
    }
  } // namespace ui
} // namespace bfc
