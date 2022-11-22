#ifndef FIXED_STRING_HPP
#define FIXED_STRING_HPP

// Simple fixed-size string library for encoding strings at compile time.
// Works out of the box in templates: e.g., `template <fixed::String Name> class ...`
// Bounds-checking at runtime by default; define NDEBUG (careful!) or FS_NOASSERT to disable

// *** Requires C++20: Fixed strings with deduced size were impossible until class-type non-type template parameters in '20

#include <algorithm> // std::copy_n, std::equal
#include <array>     // std::array
#include <concepts>  // std::unsigned_integral
#include <exception> // std::terminate
#include <utility>   // std::forward, std::move

#define FS_INLINE [[gnu::always_inline]] inline
#define FS_IMPURE [[nodiscard]] FS_INLINE
#define FS_PURE FS_IMPURE constexpr

#ifdef NDEBUG
#if (NDEBUG == 0)
#error "Defining NDEBUG to 0 doesn't in general disable (C libraries vary); undefine it instead (-UNDEBUG)"
#elif (NDEBUG != 1)
#error "If NDEBUG is defined, it must be to 1 (default when the compiler receives only `-DNDEBUG`)"
#endif // NDEBUG == ...
#define FS_ASSERT(CONDITION, ...) static_cast<void>(0)
#elif defined(FS_NOASSERT)
#if (FS_NOASSERT == 0)
#error "Defining FS_NOASSERT to 0 doesn't disable its effects; undefine it instead (-UFS_NOASSERT)"
#elif (FS_NOASSERT != 1)
#error "If FS_NOASSERT is defined, it must be to 1 (default when the compiler receives only `-DFS_NOASSERT`)"
#endif // FS_NOASSERT == ...
#undef FS_NOASSERT
#define FS_ASSERT(CONDITION, ...) static_cast<void>(0)
#else // NDEBUG, FS_NOASSERT
#include <iostream> // std::cerr
namespace fixed {
template <typename... Args>
FS_INLINE
void
safe_print(Args&&... args)
noexcept {
  try {
    (std::cerr << ... << std::forward<Args>(args)) << std::endl;
  } catch (...) {
    try {
      std::cerr << "Fixed-string internal error: safe_print failed\n";
    } catch (...) {}
  }
}
} // namespace fixed
#define FS_ASSERT(CONDITION, ...)                         \
if (not (CONDITION)) {                                    \
  safe_print("Fixed-string assertion failed: ",   \
        __VA_ARGS__, " (`" #CONDITION "` at "             \
        __FILE__ ":", +__LINE__, " evaluated to false)"); \
  std::terminate();                                       \
} else static_cast<void>(0)
#endif // NDEBUG, FS_NOASSERT

namespace fixed {

template <std::size_t N>
FS_PURE static
std::array<char, N + 1>
arr_from_str(char const (&str)[N + 1])
noexcept {
  FS_ASSERT(str[N] == '\0', "String literal must be null-terminated");
  std::array<char, N + 1> arr; // NOLINT(cppcoreguidelines-pro-type-member-init)
  std::copy_n(static_cast<char const*>(str), N + 1, arr.begin());
  return arr;
}

template <std::size_t N>
class String { // NOLINT(altera-struct-pack-align)
  std::array<char, N + 1> _arr; // NOLINT(misc-non-private-member-variables-in-classes)
 public:
  constexpr explicit String() noexcept : _arr{} {} // zero-initialization (solely for use in constant expressions)
  constexpr String(char const (&str)[N + 1]) noexcept : _arr{arr_from_str<N>(str)} {} // NOLINT(google-explicit-constructor)
  constexpr explicit String(std::array<char, N + 1>&& str) noexcept : _arr{std::move(str)} { FS_ASSERT(str[N] == '\0', "char array must be null-terminated"); }
  constexpr explicit String(char c) noexcept requires (N == 1) { _arr[0] = c; _arr[1] = '\0'; }
  constexpr String(String const&) noexcept = default;
  constexpr String(String&&) noexcept = default;
  constexpr String& operator=(String const&) noexcept = default;
  constexpr String& operator=(String&&) noexcept = default;
  constexpr ~String() noexcept = default;
  FS_PURE char const* c_str() const noexcept { return _arr.data(); }
  FS_PURE char operator[](std::size_t i) const noexcept { FS_ASSERT(i < N, "subscript index out of bounds"); return _arr[i]; }
};

//%%%%%%%%%%%%%%%% Deduction guides

String(char) -> String<1>;

template <std::size_t N> requires (N > 0) // can still work on the empty string "" since in that case N=1 (null-terminated)
String(char const (&)[N]) -> String<N - 1>;

//%%%%%%%%%%%%%%%% Non-member functions

template <std::size_t N, std::size_t M>
FS_PURE
bool
streq(String<N> const& lhs, String<M> const& rhs)
noexcept {
  if constexpr (N != M) { return false; }
  return std::equal(lhs.arr.begin(), lhs.arr.end(), rhs.arr.begin());
}

template <std::size_t N, std::size_t M>
FS_PURE
bool
operator==(String<N> const& lhs, String<M> const& rhs)
noexcept {
  return streq(lhs, rhs);
}

template <std::size_t N, std::size_t M>
FS_PURE
String<N + M>
operator+(String<N> const& lhs, String<M> const& rhs)
noexcept {
  String<N + M> rtn{}; // sadly must be zero-initialized to use in constant expressions
  std::copy_n(lhs.arr.begin(), N, rtn.arr.begin());
  std::copy_n(rhs.arr.begin(), M, rtn.arr.begin() + N);
  rtn.arr[N + M] = '\0';
  return rtn;
}

template <std::size_t N, typename T>
requires requires (T const& t) { String{t}; }
FS_PURE
decltype(auto)
operator+(String<N> const& lhs, T const& rhs)
noexcept {
  return lhs + String{rhs};
}

template <std::size_t N, typename T>
requires requires (T const& t) { String{t}; }
FS_PURE
decltype(auto)
operator+(T const& lhs, String<N> const& rhs)
noexcept {
  return String{lhs} + rhs;
}

template <std::unsigned_integral auto x>
inline constexpr unsigned char log10{x ? 1 + log10<x / 10U> : 0};

template <std::unsigned_integral auto x>
FS_PURE static
std::array<char, log10<x> + 2>
array_itoa()
noexcept {
  std::array<char, log10<x> + 2> arr{};
  arr[log10<x> + 1] = '\0';
  unsigned char i{log10<x>};
  for (auto n{x}; n > 0; n /= 10U) { arr[i--] = static_cast<char>(n % 10U + '0'); }
  return arr;
}

template <std::unsigned_integral auto x>
inline constexpr String<log10<x> + 1> itoa{array_itoa<x>()};

} // namespace fixed

//%%%%%%%%%%%%%%%% concepts & type_traits

namespace internal {
template <typename T> struct is_fixed_string { static constexpr bool value{false}; };
template <std::size_t N> struct is_fixed_string<fixed::String<N>> { static constexpr bool value{true}; };
} // namespace internal
template <typename T> inline constexpr bool is_fixed_string{internal::is_fixed_string<T>::value};
template <typename T> concept fixed_string = is_fixed_string<T>;

//%%%%%%%%%%%%%%%% iostream interface

#ifndef FSTRING_DISABLE_IO
#include <iostream>
namespace fixed {
template <std::size_t N>
[[nodiscard]] [[gnu::always_inline]] inline static
std::ostream&
operator<<(std::ostream& os, String<N> const& fs)
noexcept {
  return os << fs.c_str();
}
} // namespace fixed
#endif // FSTRING_DISABLE_IO

#undef FS_PURE
#undef FS_IMPURE
#undef FS_INLINE

#endif // FIXED_STRING_HPP
