#include "platform/Events.h"

namespace bfc {
  Events::Events(StringView const & debugName)
    : m_debugName(debugName) {}

  Events::~Events() {
    // Clear children
    clearListening();

    // Detach listeners from this event system
    for (EventListener * pListener : m_listeners) {
      pListener->detach();
    }

    Vector<Events *> children;
    std::swap(children, m_children);
    for (Events * pChild : children) {
      pChild->stopListening(this);
    }
    m_children.clear();
  }

  std::shared_ptr<EventListener> Events::addListener() {
    int64_t index = m_listeners.emplace();

    std::shared_ptr<EventListener> pListener = std::make_shared<EventListener>(this, index);

    m_listeners[index] = pListener.get();
    return pListener;
  }

  void Events::listenTo(Events * pEvents) {
    if (m_listening.contains(pEvents)) {
      return;
    }
    m_listening.pushBack(pEvents);
    pEvents->m_children.pushBack(this);
  }

  bool Events::stopListening(Events * pEvents) {
    if (!m_listening.eraseValue(pEvents)) {
      return false;
    }
    pEvents->m_children.eraseValue(this);
    return true;
  }

  void Events::clearListening() {
    for (Events * pEvents : m_listening) {
      pEvents->m_children.eraseValue(this);
    }
    m_listening.clear();
  }

  int64_t Events::getNumListeningTo() const {
    return m_listening.size();
  }

  Events * Events::getListeningTo(int64_t index) const {
    return m_listening[index];
  }

  EventListener::EventListener(Events * pOwner, int64_t index)
    : m_pOwner(pOwner)
    , m_index(index) {}

  EventListener::~EventListener() {
    if (m_pOwner != nullptr) {
      m_pOwner->m_listeners.erase(m_index);
    }
  }

  void EventListener::detach() {
    m_pOwner = nullptr;
    m_index  = -1;
  }

  Events * EventListener::getOwner() const {
    return m_pOwner;
  }
} // namespace bfc
