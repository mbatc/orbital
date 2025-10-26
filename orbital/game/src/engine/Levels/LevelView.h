#pragma once

#include "LevelComponents.h"
#include "core/Core.h"
#include <array>
#include <tuple>

namespace engine {
  template<bool IsConst, typename IndexSeq, typename... Components>
  class LevelViewImpl {};

  template<bool IsConst, int64_t... Indices, typename... Components>
  class LevelViewImpl<IsConst, std::integer_sequence<int64_t, Indices...>, Components...> {
    using Type = LevelViewImpl<IsConst, std::integer_sequence<int64_t, Indices...>, Components...>;

  public:
    template<typename T>
    using ManagerPtr = bfc::conditional_const_t<IsConst, LevelComponentStorage<T>>*;
    template<typename T>
    using ComponentT = bfc::conditional_const_t<IsConst, T>;

    class Iterator {
    public:
      Iterator(Type const * pView, int64_t index)
        : m_pView(pView)
        , m_index(index) {
        skipInvalidIndices();
      }

      Iterator & operator++() {
        if (m_pView == nullptr || m_index < 0 || m_index >= m_pView->maxSize()) {
          return *this;
        }

        ++m_index;
        skipInvalidIndices();

        return *this;
      }

      Iterator & operator++(int) {
        Iterator ret = *this;
        ::       operator++();
        return ret;
      }

      bool operator==(Iterator const & rhs) const {
        return m_index == rhs.m_index && m_pView == rhs.m_pView;
      }

      bool operator!=(Iterator const & rhs) const {
        return !this->operator==(rhs);
      }

      auto operator*() const {
        return m_pView->get(m_pView->entityAt(m_index));
      }

      EntityID entity() const {
        return m_pView->entityAt(m_index);
      }

    private:
      void skipInvalidIndices() {
        while (m_index < m_pView->maxSize() && !m_pView->contains(m_pView->entityAt(m_index))) {
          ++m_index;
        }
      }

      int64_t m_index = 0;
      Type const *  m_pView = nullptr;
    };

    LevelViewImpl(ManagerPtr<Components>... pManagers)
      : m_managers(pManagers...)
      , m_pIterator(nullptr) {
      if (((pManagers == nullptr) || ...))
        return;

      std::array<bfc::conditional_const_t<IsConst, ILevelComponentStorage> *, sizeof...(Components)> itr = {pManagers...};
      for (auto const & pManager : itr) {
        if (m_pIterator == nullptr || pManager->size() < m_pIterator->size()) {
          m_pIterator = pManager;
        }
      }
    }

    /// Try get reference to all components in this view for `entityID`.
    auto get(EntityID const & entityID) const {
      return std::tie(std::get<Indices>(m_managers)->get(entityID)...);
    }

    /// Try get a pointer to all components in this view for `entityID`.
    auto tryGet(EntityID const & entityID) const {
      return std::make_tuple(std::get<Indices>(m_managers)->tryGet(entityID)...);
    }

    /// The maximum size of this view.
    int64_t maxSize() const {
      return m_pIterator != nullptr ? m_pIterator->size() : 0;
    }

    /// Test if an entity is contained in this view.
    bool contains(EntityID const & entityID) const {
      return m_pIterator != nullptr && (std::get<Indices>(m_managers)->exists(entityID) && ...);
    }

    EntityID entityAt(int64_t index) const {
      return m_pIterator->entities().at(index);
    }

    Iterator begin() const {
      return Iterator(this, 0);
    }

    Iterator end() const {
      return Iterator(this, maxSize());
    }

  protected:
    std::tuple<ManagerPtr<Components>...>                       m_managers;
    bfc::conditional_const_t<IsConst, ILevelComponentStorage> * m_pIterator;
  };

  template<typename... Components>
  using LevelViewMutable = LevelViewImpl<false, std::make_integer_sequence<int64_t, sizeof...(Components)>, Components...>;
  template<typename... Components>
  using LevelView = LevelViewImpl<true, std::make_integer_sequence<int64_t, sizeof...(Components)>, Components...>;
} // namespace engine
