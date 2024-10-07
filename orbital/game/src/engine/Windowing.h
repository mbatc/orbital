//#pragma once
//
//#include "Subsystem.h"
//#include "platform/Window.h"
//#include "render/GraphicsDevice.h"
//
//namespace bfc {
//  class Window;
//}
//
//namespace engine {
//  using WindowHandle = int64_t;
//  class Windowing : public Subsystem {
//  public:
//    Windowing();
//
//    WindowHandle createWindow(bfc::StringView title, bool show = true, std::shared_ptr<bfc::Events> pEvents = nullptr);
//    void      remove(WindowHandle handle);
//
//    WindowHandle getMainWindow();
//
//    bfc::Ref<bfc::Events> getEvents(WindowHandle handle);
//
//    bfc::Ref<bfc::EventListener> getListener(WindowHandle handle);
//
//    bfc::GraphicsResource getRenderTarget(bfc::GraphicsDevice *pDevice, WindowHandle handle);
//    bfc::GraphicsResource createRenderTarget(bfc::GraphicsDevice *pDevice, WindowHandle handle);
//
//    WindowHandle findHandle(bfc::platform::Window* pWindow);
//  };
//}
