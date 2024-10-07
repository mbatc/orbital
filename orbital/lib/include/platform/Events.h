#pragma once

#include "../core/Map.h"
#include "../core/Pool.h"
#include "../core/String.h"
#include "../core/typeindex.h"

#include <functional>
#include <memory>
#include <typeindex>

namespace bfc {
  class EventListener;

  class BFC_API Events {
    friend EventListener;

  public:
    Events(StringView const & debugName);

    ~Events();

    static void update();

    template<typename T, typename... Args>
    void broadcast(T const & event) {
      for (EventListener * pListener : m_listeners)
        pListener->broadcast<T>(event);

      for (Events * pChildEvents : m_children)
        pChildEvents->broadcast<T>(event);
    }

    std::shared_ptr<EventListener> addListener();

    /// Set the parent events system.
    /// This system will receive any events broadcast to the parent system.
    /// @param pEvents The parent system to receive events from.
    void listenTo(Events * pEvents);
    bool stopListening(Events * pEvents);
    void clearListening();

    int64_t getNumListeningTo() const;

    Events * getListeningTo(int64_t index) const;

  private:
    String m_debugName;

    Pool<EventListener *> m_listeners; // Accept single events

    Vector<Events *> m_listening; // Event systems forwarding to this system
    Vector<Events *> m_children;  // Event systems to forward events to
  };

  class BFC_API EventListener {
  public:
    EventListener(Events * pEvents, int64_t index);
    ~EventListener();

    template<typename T>
    bool broadcast(T const & event) {
      auto * pFunction = m_handlers.tryGet(TypeID<T>());
      return pFunction != nullptr && (*pFunction)(&event);
    }

    template<typename T>
    EventListener & on(std::function<bool(T const &)> const & handler) {
      // Create and register handler
      m_handlers.add(TypeID<T>(), [handler](void const * pEvent) { return handler(*(T const *)pEvent); });

      return *this;
    }

    void detach();

    Events * getOwner() const;

  private:
    int64_t  m_index  = -1;
    Events * m_pOwner = nullptr;

    Map<type_index, std::function<bool(void const *)>> m_handlers;
  };
} // namespace bfc
