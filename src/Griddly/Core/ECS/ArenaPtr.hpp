#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

namespace griddly {

// ArenaPtr: an address-independent "pointer" into an ECS arena. It stores
// only an offset from the arena base, so it stays valid when the arena is
// copied/cloned to a different address -- swapping the arena base updates
// every ArenaPtr automatically, with no rebinding of the objects holding them.
template <typename T>
struct ArenaPtr {
  static constexpr uint32_t NULL_OFFSET = 0xFFFFFFFFu;
  uint32_t offset = NULL_OFFSET;

  bool isNull() const { return offset == NULL_OFFSET; }
  bool operator==(const ArenaPtr& o) const { return offset == o.offset; }
  bool operator!=(const ArenaPtr& o) const { return offset != o.offset; }
};

// ArenaProxy: write-through reference to a T inside an arena, resolved
// through a world pointer that is supplied at resolve time (e.g. from the
// executing action's grid), NOT stored in the object. Supports the
// `*ref = v` / `*ref += v` / `int32_t x = *ref` patterns that the behaviour
// code uses with variable pointers.
template <typename T>
class ArenaProxy {
 public:
  ArenaProxy(const std::byte* base, ArenaPtr<T> ptr) : base_(base), ptr_(ptr) {}

  T& operator*() { return *reinterpret_cast<T*>(const_cast<std::byte*>(base_) + ptr_.offset); }
  const T& operator*() const { return *reinterpret_cast<const T*>(base_ + ptr_.offset); }
  operator T() const { return **this; }
  ArenaProxy& operator=(const T& v) { **this = v; return *this; }
  ArenaProxy& operator+=(const T& v) { **this += v; return *this; }
  ArenaProxy& operator-=(const T& v) { **this -= v; return *this; }

 private:
  const std::byte* base_;
  ArenaPtr<T> ptr_;
};

// IntVar: a variable handle that is either a local fallback value or an
// arena-relative handle (offset + base). Because the handle is an OFFSET,
// relocating the arena only requires updating the base -- every IntVar
// then reads/writes the new arena automatically, with no rebinding of
// individual pointers. Supports the `*v`, `*v = x`, `*v += x`, `int32_t x = *v`
// syntax used throughout the behaviour code, so it is a drop-in replacement
// for std::shared_ptr<int32_t> variable storage.
class IntVar {
 public:
  IntVar() : cell_(std::make_shared<int32_t>(0)) {}
  static IntVar nullHandle() { IntVar v; v.cell_ = nullptr; return v; }  // compares equal to nullptr
  explicit IntVar(int32_t v) : cell_(std::make_shared<int32_t>(v)) {}
  IntVar(std::shared_ptr<int32_t> p) : cell_(std::move(p)) {}  // implicit: brace-init maps from shared_ptr

  // Bind this handle to an arena slot. The offset stays valid if the arena
  // is copied elsewhere; only the base needs updating on relocation. Once
  // bound, all access goes to the arena (copies included -- offsets alias).
  void bind(const std::byte* base, ArenaPtr<int32_t> ptr) {
    base_ = base;
    ref_ = ptr;
  }
  bool isBound() const { return !ref_.isNull() && base_ != nullptr; }

  // For grid-global variables: share the grid's existing shared_ptr storage.
  void setShared(std::shared_ptr<int32_t> p) { cell_ = std::move(p); }

  int32_t& operator*() {
    if (isBound()) return *reinterpret_cast<int32_t*>(const_cast<std::byte*>(base_) + ref_.offset);
    return *cell_;
  }
  const int32_t& operator*() const {
    if (isBound()) return *reinterpret_cast<const int32_t*>(base_ + ref_.offset);
    return *cell_;
  }
  operator int32_t() const { return **this; }
  IntVar& operator=(int32_t v) { **this = v; return *this; }
  bool operator==(std::nullptr_t) const { return !isBound() && !cell_; }
  bool operator!=(std::nullptr_t) const { return isBound() || cell_ != nullptr; }

 private:
  std::shared_ptr<int32_t> cell_;  // shared on copy (pre-bind fallback / globals)
  ArenaPtr<int32_t> ref_{};
  const std::byte* base_ = nullptr;
};

// Variable map used by Object/ObjectVariable: name -> arena handle.
using VariableMap = std::unordered_map<std::string, IntVar>;

// Ambient active arena base. ArenaRef resolves against this, so swapping
// the active arena (one assignment) retargets every ArenaRef everywhere --
// no per-object rebinding at all.
inline const std::byte*& activeArenaBase() {
  static thread_local const std::byte* base = nullptr;
  return base;
}

// ArenaRef: a pure offset handle with NO stored base. Dereferences through
// activeArenaBase(), so changing the active arena address automatically
// moves every ArenaRef to the new arena.
template <typename T>
struct ArenaRef {
  static constexpr uint32_t NULL_OFFSET = 0xFFFFFFFFu;
  uint32_t offset = NULL_OFFSET;

  bool isNull() const { return offset == NULL_OFFSET; }

  T& operator*() { return *reinterpret_cast<T*>(const_cast<std::byte*>(activeArenaBase()) + offset); }
  const T& operator*() const { return *reinterpret_cast<const T*>(activeArenaBase() + offset); }
  operator T() const { return **this; }
  ArenaRef& operator=(const T& v) { **this = v; return *this; }
};

}  // namespace griddly
