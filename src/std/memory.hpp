#pragma once

#include <functional>

namespace play {
template <typename T, typename... Args>
inline void construct_at(T* p, Args&&... args) {
  new ((void*)p) T(std::forward<Args>(args)...);
}
} // namespace play
