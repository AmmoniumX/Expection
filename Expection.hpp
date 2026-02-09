#include <concepts>
#include <expected>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

namespace Expection {

enum class Policy { Exceptions, Expected };

#ifndef EXPECTION_DEFAULTPOLICY
#define EXPECTION_DEFAULTPOLICY Exceptions
#endif

inline constexpr Policy DefaultPolicy = Policy::EXPECTION_DEFAULTPOLICY;

// Meta-function to determine the Return Type
template <typename R, typename E = std::monostate, Policy P = DefaultPolicy>
using ResultType =
    std::conditional_t<P == Policy::Exceptions, R, std::expected<R, E>>;

// === Success Helpers ===
template <typename R, typename E = std::monostate, Policy P = DefaultPolicy>
constexpr auto success(R val) -> ResultType<R, E, P> {

  if constexpr (P == Policy::Exceptions) {
    // Return by value, allowing move-construction from the forward
    return val;
  } else {
    // Wrap the value in std::expected
    return std::expected<R, E>(val);
  }
}

// Void specialization
template <typename E = std::monostate, Policy P = DefaultPolicy>
constexpr auto success() -> ResultType<void, E, P> {
  if constexpr (P == Policy::Exceptions) {
    return;
  } else {
    return std::expected<void, E>{};
  }
}

// === Failure Helpers ===

// "Functor" based helpers
template <typename T, typename Unexpected, typename... Args>
concept ErrorFunctor = requires(Args &&...args) {
  {
    T::exception(std::forward<Args>(args)...)
  } -> std::derived_from<std::exception>;

  {
    T::unexpected(std::forward<Args>(args)...)
  } -> std::same_as<std::unexpected<Unexpected>>;
};

// "Functor" based overload: it takes one functor as template parameter,
// defining static exception() and unexpected() methods
template <typename R, typename E, typename Functor, Policy P = DefaultPolicy,
          typename... Args>
  requires ErrorFunctor<Functor, E, Args...>
constexpr auto make_failure(Args &&...args) -> ResultType<R, E, P> {
  if constexpr (P == Policy::Exceptions) {
    throw Functor::exception(std::forward<Args>(args)...);
  } else {
    return Functor::unexpected(std::forward<Args>(args)...);
  }
}

// Callable based approach
template <typename T, typename... Args>
concept ExceptionCallable = requires(T &&t, Args &&...args) {
  { t(std::forward<Args>(args)...) } -> std::derived_from<std::exception>;
};

template <typename T, typename Error, typename... Args>
concept UnexpectedCallable = requires(T &&t, Args &&...args) {
  { t(std::forward<Args>(args)...) } -> std::same_as<std::unexpected<Error>>;
};

// "Makes" a failure object by directly calling the appropriate callable with
// arguments
template <typename Result, typename Error, Policy P = DefaultPolicy,
          typename Unexpected, typename Exception, typename... Args>
  requires UnexpectedCallable<Unexpected, Error, Args...> &&
           ExceptionCallable<Exception, Args...>
constexpr auto make_failure(Unexpected &&unexpected, Exception &&exception,
                            Args &&...args) -> ResultType<Result, Error, P> {
  if constexpr (P == Policy::Exceptions) {
    throw exception(std::forward<Args>(args)...);
  } else {
    return unexpected(std::forward<Args>(args)...);
  }
}

// Static constructor based approach
template <typename T, typename... Args>
concept ExceptionConstructable = requires(Args &&...args) {
  {
    T::exception(std::forward<Args>(args)...)
  } -> std::derived_from<std::exception>;
};

// "Makes" a failure object by directly calling a constructor with arguments
template <typename Result, typename Error, Policy P = DefaultPolicy,
          typename... Args>
  requires ExceptionConstructable<Error, Args...>
constexpr auto make_failure(Args &&...args) -> ResultType<Result, Error, P> {
  if constexpr (P == Policy::Exceptions) {
    throw Error::exception(std::forward<Args>(args)...);
  } else {
    return std::unexpected(Error{std::forward<Args>(args)...});
  }
}

// Conversion based approach
template <typename T>
concept ExceptionConvertible = requires(T &&t) {
  { t.exception() } -> std::derived_from<std::exception>;
};

// This one doesn't "make" a failure, it accepts an existing "Error" object with
// a .exception() conversion
template <typename Result, Policy P = DefaultPolicy, ExceptionConvertible E>
constexpr auto failure(E &&error) -> ResultType<Result, std::decay_t<E>, P> {
  if constexpr (P == Policy::Exceptions) {
    throw error.exception();
  } else {
    return std::unexpected<std::decay_t<E>>(std::forward<E>(error));
  }
}
} // namespace Expection
