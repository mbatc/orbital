#pragma once

#include "../core/Reflect.h"
#include "../core/String.h"
#include "Context.h"

#include <any>
#include <optional>

namespace bfc {
  namespace ui {
    template<typename T>
    struct InputTraits {
      inline static constexpr ImGuiDataType dataType = ImGuiDataType_COUNT;
      inline static constexpr int64_t       width    = 1;
      inline static String                  format   = "";
    };

    // TODO: Implement in Input()
    // struct TextInputCallbacks {
    //   /// Called to auto-complete the text input (tab is pressed).
    //   /// Modify the input parameter.
    //   std::function<void(String *)>  completion;
    //
    //   /// Filter individual characters (modify or discard).
    //   /// You may modify the input character.
    //   /// Return true to keep.
    //   /// Return false to discard.
    //   std::function<bool(wchar_t *)> filter;
    //
    //   /// Called to get history.
    //   /// Modify the first parameter.
    //   /// If the second parameter is 1 the up arrow was pressed.
    //   /// If the second parameter is -1 the down arrow was pressed.
    //   std::function<void(String *, int)> history;
    // };

    /// Get the size of the content in the current window.
    BFC_API Vec2 GetContentSize();

    BFC_API bool Input(String const & name, String * pValue, ImGuiInputTextFlags flags = 0 /*, TextInputCallbacks const & callbacks = TextInputCallbacks()*/);

    BFC_API bool Input(String const & name, bool * pValue);

    BFC_API bool Input(String const & name, Colour<RGBf32> * pValue);

    BFC_API bool Input(String const & name, Colour<RGBAf32> * pValue);

    template<typename Format>
    bool Input(String const & name, Colour<Format> * pValue) {
      if (pValue->hasA) {
        Colour<RGBAf32> displayFormat = *pValue;
        if (!Input(name, &displayFormat))
          return false;
        *pValue = displayFormat;
      } else {
        Colour<RGBf32> displayFormat = *pValue;
        if (!Input(name, &displayFormat))
          return false;
        *pValue = displayFormat;
      }
      return true;
    }

    BFC_API bool Input(String const & name, ImGuiDataType dataType, void * pValue, char const * format, int64_t count = 1);

    BFC_API bool Slider(String const & name, ImGuiDataType dataType, void * pValue, void const * pMin, void const * pMax, char const * format,
                        int64_t count = 1);

    BFC_API void Text(String const & text);

    BFC_API void Label(String const & text);

    /// Draw a horizontal separator.
    BFC_API void Separator();

    namespace detail {
      struct BFC_API InputVisitor {
        template<typename T>
        void operator()(char const * name, T & member) {
          *pChanged |= Input(name, &member);
        }

        bool * pChanged = nullptr;
      };
    } // namespace detail

    template<typename T>
    bool Input(String const & name, T * pValue) {
      if constexpr (InputTraits<T>::dataType != ImGuiDataType_COUNT) {
        return Input(name, InputTraits<T>::dataType, pValue, InputTraits<T>::format.c_str(), InputTraits<T>::width);
      } else if constexpr (has_reflect_v<T>) {
        ImGui::Text(name.c_str());
        ImGui::PushID(name.c_str());
        bool changed = false;
        reflect<T>().visit(pValue, detail::InputVisitor{&changed});
        ImGui::PopID();
        return changed;
      } else {
        static_assert(false, "Unable to draw input for type. No input trait or reflection is implemented");
      }
    }

    template<typename T>
    bool Slider(String const & name, T * pValue, T const & min, T const & max) {
      return Slider(name, InputTraits<T>::dataType, pValue, &min, &max, InputTraits<T>::format.c_str(), InputTraits<T>::width);
    }

    struct FilterSettings {
      bool ignoreCase      = true;
      bool matchWhitespace = false;
    };

    /// An input that can be used as a search filter.
    BFC_API bool FilterInput(String const & name, String * pFilter, FilterSettings * pSettings);

    /// Apply a filter to some text.
    /// @retval true  The text matched the filter.
    /// @retval false The text did not match the filter.
    BFC_API bool FilterText(String const & text, String const & filter, FilterSettings settings = {});

    BFC_API bool BeginCollapsingChild(String const & name, float width, float maxHeight, bool * pVisible = nullptr, bool defaultOpen = false);

    BFC_API void EndCollapsingChild();

    BFC_API bool BeginDragDropSource(ImGuiDragDropFlags flags = ImGuiDragDropFlags_None);
    BFC_API void EndDragDropSource();

    BFC_API bool BeginDragDropTarget();
    BFC_API void EndDragDropTarget();

    BFC_API bool SetDragDropPayload(String const & type, std::any payload);

    BFC_API std::optional<std::any> AcceptDragDropPayload(String const & type, ImGuiDragDropFlags flags = ImGuiDragDropFlags_None);

    template<typename T>
    struct DragDropPayloadTraits {
      inline static String id = String::format("type#", (uint64_t)TypeID<T>().hash_code());
    };

    /// Set a drag and drop payload.
    /// The type of payload is based on `T`
    template<typename T>
    bool SetDragDropPayload(T const & payload) {
      return SetDragDropPayload(DragDropPayloadTraits<T>::id, payload);
    }

    /// Accept a drag and drop payload of type `T`.
    template<typename T>
    std::optional<T> AcceptDragDropPayload(ImGuiDragDropFlags flags = ImGuiDragDropFlags_None) {
      std::optional<std::any> value = AcceptDragDropPayload(DragDropPayloadTraits<T>::id, flags);
      return value ? std::any_cast<T>(*value) : std::optional<T>{};
    }

    template<>
    struct InputTraits<int64_t> {
      inline static constexpr ImGuiDataType dataType = ImGuiDataType_S64;
      inline static constexpr int64_t       width    = 1;
      inline static String                  format   = "%lld";
    };

    template<>
    struct InputTraits<int32_t> {
      inline static constexpr ImGuiDataType dataType = ImGuiDataType_S32;
      inline static constexpr int64_t       width    = 1;
      inline static String                  format   = "%d";
    };

    template<>
    struct InputTraits<int16_t> {
      inline static constexpr ImGuiDataType dataType = ImGuiDataType_S16;
      inline static constexpr int64_t       width    = 1;
      inline static String                  format   = "%hd";
    };

    template<>
    struct InputTraits<int8_t> {
      inline static constexpr ImGuiDataType dataType = ImGuiDataType_S8;
      inline static constexpr int64_t       width    = 1;
      inline static String                  format   = "%hhd";
    };

    template<>
    struct InputTraits<uint64_t> {
      inline static constexpr ImGuiDataType dataType = ImGuiDataType_U64;
      inline static constexpr int64_t       width    = 1;
      inline static String                  format   = "%llu";
    };

    template<>
    struct InputTraits<uint32_t> {
      inline static constexpr ImGuiDataType dataType = ImGuiDataType_U32;
      inline static constexpr int64_t       width    = 1;
      inline static String                  format   = "%du";
    };

    template<>
    struct InputTraits<uint16_t> {
      inline static constexpr ImGuiDataType dataType = ImGuiDataType_U16;
      inline static constexpr int64_t       width    = 1;
      inline static String                  format   = "%hd";
    };

    template<>
    struct InputTraits<uint8_t> {
      inline static constexpr ImGuiDataType dataType = ImGuiDataType_U8;
      inline static constexpr int64_t       width    = 1;
      inline static String                  format   = "%hhd";
    };

    template<>
    struct InputTraits<float> {
      inline static constexpr ImGuiDataType dataType = ImGuiDataType_Float;
      inline static constexpr int64_t       width    = 1;
      inline static String                  format   = "%f";
    };

    template<>
    struct InputTraits<double> {
      inline static constexpr ImGuiDataType dataType = ImGuiDataType_Double;
      inline static constexpr int64_t       width    = 1;
      inline static String                  format   = "%f";
    };

    template<typename T>
    struct InputTraits<Vector2<T>> {
      inline static constexpr ImGuiDataType dataType = InputTraits<T>::dataType;
      inline static constexpr int64_t       width    = InputTraits<T>::width * 2;
      inline static String                  format   = InputTraits<T>::format;
    };

    template<typename T>
    struct InputTraits<Vector3<T>> {
      inline static constexpr ImGuiDataType dataType = InputTraits<T>::dataType;
      inline static constexpr int64_t       width    = InputTraits<T>::width * 3;
      inline static String                  format   = InputTraits<T>::format;
    };

    template<typename T>
    struct InputTraits<Vector4<T>> {
      inline static constexpr ImGuiDataType dataType = InputTraits<T>::dataType;
      inline static constexpr int64_t       width    = InputTraits<T>::width * 4;
      inline static String                  format   = InputTraits<T>::format;
    };
  } // namespace ui
} // namespace bfc
