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
    void broadcast(T const & event) {
      auto * pFunction = m_handlers.tryGet(TypeID<T>());
      if (pFunction == nullptr) {
        return;
      }

      (*pFunction)(&event);
    }

    template<typename HandlerT>
    EventListener & on(HandlerT func) {
      using ArgList = typename bfc::function_type<HandlerT>::ArgList;
      return onDeduced(std::forward<HandlerT>(func), ArgList());
    }

    template<typename ClassT, typename EventType>
    EventListener & on(void (ClassT::* func)(EventType), ClassT * pThis) {
      return on<std::decay_t<EventType>>([pThis, func](EventType e) { (pThis->*func)(e); });
    }

    template<typename ClassT, typename EventType>
    EventListener & on(void (ClassT::*func)(EventType) const, ClassT const * pThis) {
      return on<std::decay_t<EventType>>([pThis, func](EventType e) { (pThis->*func)(e); });
    }

    template<typename T>
    EventListener & on(std::function<void(T const &)> const & handler) {
      // Create and register handler
      m_handlers.add(TypeID<T>(), [handler](void const * pEvent) { handler(*(T const *)pEvent); });

      return *this;
    }

    void detach();

    Events * getOwner() const;

  private:
    template<typename HandlerT, typename EventType>
    EventListener & onDeduced(HandlerT func, bfc::arg_list<EventType>) {
      return on<std::decay_t<EventType>>(std::forward<HandlerT>(func));
    }

    int64_t  m_index  = -1;
    Events * m_pOwner = nullptr;

    Map<type_index, std::function<void(void const *)>> m_handlers;
  };
} // namespace bfc
