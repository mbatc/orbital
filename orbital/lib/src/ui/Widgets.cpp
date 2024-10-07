#include "ui/Widgets.h"
#include "core/Map.h"

namespace bfc {
  namespace ui {
    namespace detail {
      struct TextInputData {
        // TextInputCallbacks callbacks;
        Vector<char> buffer;
      };

      int ResizeTextInputBuffer(ImGuiInputTextCallbackData * data) {
        TextInputData * pInputData = (TextInputData *)data->UserData;
        switch (data->EventFlag) {
        case ImGuiInputTextFlags_CallbackResize:
          pInputData->buffer.resize(data->BufSize, 0);
          data->Buf     = pInputData->buffer.data();
          break;
        // case ImGuiInputTextFlags_CallbackHistory:
        // case ImGuiInputTextFlags_CallbackCompletion:
        // case ImGuiInputTextFlags_CallbackCharFilter:
        }
        return 0;
      }
    } // namespace detail

    Vec2 GetContentSize() {
      return ImGui::GetCurrentWindow()->ContentSize;
    }

    bool Input(String const & name, String * pValue, ImGuiInputTextFlags flags /*, TextInputCallbacks const & callbacks*/) {
      detail::TextInputData data;
      data.buffer.resize(std::max(pValue->length() * 2, 256ll), 0);
      memcpy(data.buffer.data(), pValue->c_str(), pValue->length());
      bool ret = ImGui::InputText(name.c_str(), data.buffer.data(), (int64_t)data.buffer.size(), flags | ImGuiInputTextFlags_CallbackResize,
                                  detail::ResizeTextInputBuffer, &data);
      *pValue  = data.buffer.data();
      return ret;
    }

    bool Input(String const& name, bool* pValue) {
      return ImGui::Checkbox(name.c_str(), pValue);
    }

    bool Input(String const & name, ImGuiDataType dataType, void * pValue, char const * format, int64_t count) {
      return ImGui::InputScalarN(name.c_str(), dataType, pValue, (int)count, 0, 0, format, 0);
    }
    
    void Text(String const& text) {
      ImGui::Text(text.c_str());
    }

    void Label(String const& text) {
      ImGui::Text(text.c_str());
    }

    void Separator() {
      ImGui::Separator();
    }

    bool Slider(String const & name, ImGuiDataType dataType, void * pValue, void const * pMin, void const * pMax, char const * format, int64_t count) {
      return ImGui::SliderScalarN(name.c_str(), dataType, pValue, (int)count, pMin, pMax, format, 1.0f);
    }

    bool FilterInput(String const& name, String* pFilter, FilterSettings* pSettings) {
      return Input(name, pFilter);
    }

    bool FilterText(String const& text, String const& filter, FilterSettings settings) {
      if (settings.matchWhitespace) {
        return text.compare(filter, settings.ignoreCase);
      } else {
        static Vector<StringView> separators = { " ", "\n", "\t" };
        for (StringView const & part : filter.split(separators.getView(), true)) {
          if (text.find(part)) {
            if (text.find(filter)) {
              return true;
            }
          }
        }

        return false;
      }
    }

    struct CollapsingChildContext {
      float contentHeight = 0;
      float height        = 1.0f;
      float maxHeight     = 0;
      bool  isOpen        = false;
      bool  closed        = true;
    };

    static Map<uint32_t, CollapsingChildContext> g_collapsingChild;
    static Vector<ImGuiID>                       g_collapsingChildIDStack;

    bool BeginCollapsingChild(String const & name, float width, float maxHeight, bool * pVisible /*= nullptr*/,
                              bool defaultOpen /*= false*/) {
      ImGuiID id = ImGui::GetID(name.c_str());
      g_collapsingChildIDStack.pushBack(id);

      CollapsingChildContext & ctx = g_collapsingChild[id];
      ctx.maxHeight                = maxHeight;

      bool isVisible = ImGui::BeginChild(name.c_str(), {width, ctx.height}, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
      bool open      = ImGui::CollapsingHeader(name.c_str(), pVisible, defaultOpen);

      if (isVisible)
        ctx.isOpen = open;

      ctx.contentHeight = ImGui::GetItemRectMax().y - ImGui::GetWindowPos().y;
      ctx.closed &= !ctx.isOpen;
      return !ctx.closed;
    }

    void EndCollapsingChild() {
      ImGuiID                  id  = g_collapsingChildIDStack.back();
      CollapsingChildContext & ctx = g_collapsingChild[id];

      if (ctx.isOpen)
        ctx.contentHeight = GetContentSize().y + ImGui::GetStyle().WindowPadding.y;

      ImGui::EndChild();

      ctx.height += float((ctx.contentHeight - ctx.height) * glm::clamp<double>(ImGui::GetIO().DeltaTime * 17, 0, 1));
      ctx.height = glm::max(ctx.height, 1.0f);
      if (ctx.maxHeight != 0)
        ctx.height = glm::min(ctx.height, ctx.maxHeight);

      if (!ctx.isOpen && abs(ctx.height - ctx.contentHeight) < 0.5f)
        ctx.closed = true;

      g_collapsingChildIDStack.popBack();
    }

    bool BeginDragDropSource(ImGuiDragDropFlags flags) {
      return ImGui::BeginDragDropSource(flags);
    }

    void EndDragDropSource() {
      return ImGui::EndDragDropSource();
    }

    bool BeginDragDropTarget() {
      return ImGui::BeginDragDropTarget();
    }

    void EndDragDropTarget() {
      return ImGui::EndDragDropTarget();
    }

    static Map<String, std::any> g_dndPayloads;

    bool SetDragDropPayload(String const & type, std::any payload) {
      if (!ImGui::SetDragDropPayload(type.c_str(), 0, 0, 0))
        return false;
      g_dndPayloads[type] = payload;
      return true;
    }

    std::optional<std::any> AcceptDragDropPayload(String const & type, ImGuiDragDropFlags flags) {
      if (ImGuiPayload const * pPayload = ImGui::AcceptDragDropPayload(type.c_str(), flags))
        return g_dndPayloads.getOr(type, {});
      return std::optional<std::any>();
    }
  } // namespace ui
} // namespace bfc
