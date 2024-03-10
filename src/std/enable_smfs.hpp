#pragma once

namespace play {
template <bool copy, bool copy_assign, bool move, bool move_assign, typename Tag = void>
struct EnableCopyMove {};

template <typename Tag>
struct EnableCopyMove<false, true, true, true, Tag> {
  constexpr EnableCopyMove() noexcept = default;
  constexpr EnableCopyMove(EnableCopyMove const&) noexcept = delete;
  constexpr EnableCopyMove(EnableCopyMove&&) noexcept = default;
  EnableCopyMove& operator=(EnableCopyMove const&) noexcept = default;
  EnableCopyMove& operator=(EnableCopyMove&&) noexcept = default;
};

template <typename Tag>
struct EnableCopyMove<true, false, true, true, Tag> {
  constexpr EnableCopyMove() noexcept = default;
  constexpr EnableCopyMove(EnableCopyMove const&) noexcept = default;
  constexpr EnableCopyMove(EnableCopyMove&&) noexcept = default;
  EnableCopyMove& operator=(EnableCopyMove const&) noexcept = delete;
  EnableCopyMove& operator=(EnableCopyMove&&) noexcept = default;
};

template <typename Tag>
struct EnableCopyMove<true, true, false, true, Tag> {
  constexpr EnableCopyMove() noexcept = default;
  constexpr EnableCopyMove(EnableCopyMove const&) noexcept = default;
  constexpr EnableCopyMove(EnableCopyMove&&) noexcept = delete;
  EnableCopyMove& operator=(EnableCopyMove const&) noexcept = default;
  EnableCopyMove& operator=(EnableCopyMove&&) noexcept = default;
};

template <typename Tag>
struct EnableCopyMove<true, true, true, false, Tag> {
  constexpr EnableCopyMove() noexcept = default;
  constexpr EnableCopyMove(EnableCopyMove const&) noexcept = default;
  constexpr EnableCopyMove(EnableCopyMove&&) noexcept = default;
  EnableCopyMove& operator=(EnableCopyMove const&) noexcept = default;
  EnableCopyMove& operator=(EnableCopyMove&&) noexcept = delete;
};

template <typename Tag>
struct EnableCopyMove<false, false, true, true, Tag> {
  constexpr EnableCopyMove() noexcept = default;
  constexpr EnableCopyMove(EnableCopyMove const&) noexcept = delete;
  constexpr EnableCopyMove(EnableCopyMove&&) noexcept = default;
  EnableCopyMove& operator=(EnableCopyMove const&) noexcept = delete;
  EnableCopyMove& operator=(EnableCopyMove&&) noexcept = default;
};

template <typename Tag>
struct EnableCopyMove<false, true, false, true, Tag> {
  constexpr EnableCopyMove() noexcept = default;
  constexpr EnableCopyMove(EnableCopyMove const&) noexcept = delete;
  constexpr EnableCopyMove(EnableCopyMove&&) noexcept = delete;
  EnableCopyMove& operator=(EnableCopyMove const&) noexcept = default;
  EnableCopyMove& operator=(EnableCopyMove&&) noexcept = default;
};

template <typename Tag>
struct EnableCopyMove<false, true, true, false, Tag> {
  constexpr EnableCopyMove() noexcept = default;
  constexpr EnableCopyMove(EnableCopyMove const&) noexcept = delete;
  constexpr EnableCopyMove(EnableCopyMove&&) noexcept = default;
  EnableCopyMove& operator=(EnableCopyMove const&) noexcept = default;
  EnableCopyMove& operator=(EnableCopyMove&&) noexcept = delete;
};

template <typename Tag>
struct EnableCopyMove<true, false, false, true, Tag> {
  constexpr EnableCopyMove() noexcept = default;
  constexpr EnableCopyMove(EnableCopyMove const&) noexcept = default;
  constexpr EnableCopyMove(EnableCopyMove&&) noexcept = delete;
  EnableCopyMove& operator=(EnableCopyMove const&) noexcept = delete;
  EnableCopyMove& operator=(EnableCopyMove&&) noexcept = default;
};

template <typename Tag>
struct EnableCopyMove<true, false, true, false, Tag> {
  constexpr EnableCopyMove() noexcept = default;
  constexpr EnableCopyMove(EnableCopyMove const&) noexcept = default;
  constexpr EnableCopyMove(EnableCopyMove&&) noexcept = default;
  EnableCopyMove& operator=(EnableCopyMove const&) noexcept = delete;
  EnableCopyMove& operator=(EnableCopyMove&&) noexcept = delete;
};

template <typename Tag>
struct EnableCopyMove<true, true, false, false, Tag> {
  constexpr EnableCopyMove() noexcept = default;
  constexpr EnableCopyMove(EnableCopyMove const&) noexcept = default;
  constexpr EnableCopyMove(EnableCopyMove&&) noexcept = delete;
  EnableCopyMove& operator=(EnableCopyMove const&) noexcept = default;
  EnableCopyMove& operator=(EnableCopyMove&&) noexcept = delete;
};

template <typename Tag>
struct EnableCopyMove<false, false, false, true, Tag> {
  constexpr EnableCopyMove() noexcept = default;
  constexpr EnableCopyMove(EnableCopyMove const&) noexcept = delete;
  constexpr EnableCopyMove(EnableCopyMove&&) noexcept = delete;
  EnableCopyMove& operator=(EnableCopyMove const&) noexcept = delete;
  EnableCopyMove& operator=(EnableCopyMove&&) noexcept = default;
};

template <typename Tag>
struct EnableCopyMove<false, false, true, false, Tag> {
  constexpr EnableCopyMove() noexcept = default;
  constexpr EnableCopyMove(EnableCopyMove const&) noexcept = delete;
  constexpr EnableCopyMove(EnableCopyMove&&) noexcept = default;
  EnableCopyMove& operator=(EnableCopyMove const&) noexcept = delete;
  EnableCopyMove& operator=(EnableCopyMove&&) noexcept = delete;
};

template <typename Tag>
struct EnableCopyMove<false, true, false, false, Tag> {
  constexpr EnableCopyMove() noexcept = default;
  constexpr EnableCopyMove(EnableCopyMove const&) noexcept = delete;
  constexpr EnableCopyMove(EnableCopyMove&&) noexcept = delete;
  EnableCopyMove& operator=(EnableCopyMove const&) noexcept = default;
  EnableCopyMove& operator=(EnableCopyMove&&) noexcept = delete;
};

template <typename Tag>
struct EnableCopyMove<true, false, false, false, Tag> {
  constexpr EnableCopyMove() noexcept = default;
  constexpr EnableCopyMove(EnableCopyMove const&) noexcept = default;
  constexpr EnableCopyMove(EnableCopyMove&&) noexcept = delete;
  EnableCopyMove& operator=(EnableCopyMove const&) noexcept = delete;
  EnableCopyMove& operator=(EnableCopyMove&&) noexcept = delete;
};

template <typename Tag>
struct EnableCopyMove<false, false, false, false, Tag> {
  constexpr EnableCopyMove() noexcept = default;
  constexpr EnableCopyMove(EnableCopyMove const&) noexcept = delete;
  constexpr EnableCopyMove(EnableCopyMove&&) noexcept = delete;
  EnableCopyMove& operator=(EnableCopyMove const&) noexcept = delete;
  EnableCopyMove& operator=(EnableCopyMove&&) noexcept = delete;
};

} // namespace play
