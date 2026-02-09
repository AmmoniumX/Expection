#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <string>
#include <utility>

#include "Expection.hpp"

// Example Error type and required utilities
// NOTE: This is checking *all* the possible conversion methods from Expection,
// Realistically, you only need to implement one of them
// Suggested implementation: `static auto exception(args...)` inside Error class

struct DivideByError {
  enum class Kind { DivideByZero };

  Kind kind;

  static constexpr char const *err_to_str(Kind kind) {
    switch (kind) {
    case Kind::DivideByZero:
      return "Division by Zero";

    default:
      std::unreachable();
    };
  }

  auto str() { return err_to_str(kind); }

  // In-place exception construction
  static auto exception(Kind k) { return std::runtime_error(err_to_str(k)); }

  // Convert to exception
  auto exception() { return std::runtime_error(err_to_str(kind)); }
};

struct DivideByErrorFunctor {
  static auto unexpected(DivideByError::Kind k) {
    return std::unexpected<DivideByError>(k);
  }

  static auto exception(DivideByError::Kind k) {
    return std::runtime_error(DivideByError::err_to_str(k));
  }
};

using namespace Expection;

// Testing class
// NOTE: we are only using "FailureMethod" as a template parameter for our unit
// tests, For actual use of the library, you'd just choose one and only have
// Policy as the template parameter
enum class FailureMethod { InPlace, Functor, Callable, Conversion };

template <FailureMethod F, Policy P = DefaultPolicy>
auto divide_by(int numerator, int denominator)
    -> ResultType<double, DivideByError, P> {
  using Result = double;
  using Error = DivideByError;

  if (denominator == 0) {
    if constexpr (F == FailureMethod::InPlace) {
      return make_failure<Result, Error, P>(DivideByError::Kind::DivideByZero);
    } else if constexpr (F == FailureMethod::Functor) {
      return make_failure<Result, Error, DivideByErrorFunctor, P>(
          DivideByError::Kind::DivideByZero);
    } else if constexpr (F == FailureMethod::Callable) {
      auto make_unexpected = [](DivideByError::Kind k) {
        return std::unexpected<DivideByError>(k);
      };

      auto make_exception = [](DivideByError::Kind k) {
        return std::runtime_error(DivideByError::err_to_str(k));
      };
      return make_failure<Result, Error, P>(make_unexpected, make_exception,
                                            DivideByError::Kind::DivideByZero);
    } else if constexpr (F == FailureMethod::Conversion) {
      auto err = DivideByError{DivideByError::Kind::DivideByZero};
      return failure<Result, P>(err);
    }
  }

  return success<Result, Error, P>(static_cast<Result>(numerator) /
                                   denominator);
}

TEST_CASE_TEMPLATE(
    "divide_by with Expected policy", F,
    std::integral_constant<FailureMethod, FailureMethod::InPlace>,
    std::integral_constant<FailureMethod, FailureMethod::Functor>,
    std::integral_constant<FailureMethod, FailureMethod::Callable>,
    std::integral_constant<FailureMethod, FailureMethod::Conversion>) {
  constexpr auto FailMethod = F::value;

  SUBCASE("success case") {
    auto result = divide_by<FailMethod, Policy::Expected>(1, 2);
    REQUIRE(result.has_value());
    CHECK(result.value() == doctest::Approx(0.5));
  }

  SUBCASE("failure case") {
    auto result = divide_by<FailMethod, Policy::Expected>(1, 0);
    REQUIRE_FALSE(result.has_value());
    CHECK(std::string(result.error().str()) == "Division by Zero");
  }
}

TEST_CASE_TEMPLATE(
    "divide_by with Exceptions policy", F,
    std::integral_constant<FailureMethod, FailureMethod::InPlace>,
    std::integral_constant<FailureMethod, FailureMethod::Functor>,
    std::integral_constant<FailureMethod, FailureMethod::Callable>,
    std::integral_constant<FailureMethod, FailureMethod::Conversion>) {
  constexpr auto FailMethod = F::value;

  SUBCASE("success case") {
    auto result = divide_by<FailMethod, Policy::Exceptions>(1, 2);
    CHECK(result == doctest::Approx(0.5));
  }

  SUBCASE("failure case") {
    bool exception_thrown = false;
    std::string exception_message;
    try {
      divide_by<FailMethod, Policy::Exceptions>(1, 0);
    } catch (const std::runtime_error &ex) {
      exception_thrown = true;
      exception_message = ex.what();
    }
    REQUIRE(exception_thrown);
    CHECK(exception_message == "Division by Zero");
  }
}

TEST_CASE_TEMPLATE(
    "divide_by with default policy", F,
    std::integral_constant<FailureMethod, FailureMethod::InPlace>,
    std::integral_constant<FailureMethod, FailureMethod::Functor>,
    std::integral_constant<FailureMethod, FailureMethod::Callable>,
    std::integral_constant<FailureMethod, FailureMethod::Conversion>) {
  constexpr auto FailMethod = F::value;

  SUBCASE("success case") {
    auto result = divide_by<FailMethod>(1, 2);
    CHECK(result == doctest::Approx(0.5));
  }

  SUBCASE("failure case") {
    bool exception_thrown = false;
    std::string exception_message;
    try {
      divide_by<FailMethod>(1, 0);
    } catch (const std::runtime_error &ex) {
      exception_thrown = true;
      exception_message = ex.what();
    }
    REQUIRE(exception_thrown);
    CHECK(exception_message == "Division by Zero");
  }
}
