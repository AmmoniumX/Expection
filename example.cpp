#include <print>

#include "Expection.hpp"

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
};

template <Expection::Policy P = Expection::DefaultPolicy>
auto divide_by(int numerator, int denominator)
    -> Expection::ResultType<double, DivideByError, P> {
  using Expection::make_failure, Expection::success;
  using Result = double;
  using Error = DivideByError;

  if (denominator == 0) {
    return make_failure<Result, Error, P>(DivideByError::Kind::DivideByZero);
  }

  auto val = static_cast<double>(numerator) / denominator;
  return success<Result, Error>(val);
}

int main() {
  // Default policy is exceptions unless macro is defined to something else
  auto success = divide_by(1, 2);
  std::println("{}", success);

  try {
    auto error = divide_by(1, 0);
    std::println("{}", error);
  } catch (std::exception &ex) {
    std::println("Caught: {}", ex.what());
  }

  // If you want, you can explicitly specify the erorr handling policy you want
  // at the call site

  using Expection::Policy;
  // returns std::expected<double, DivideByError>
  auto ret1 = divide_by<Policy::Expected>(1, 0);
  if (ret1.has_value()) {
    std::println("{}", ret1.value());
  } else {
    std::println("Unexpected: {}", ret1.error().str());
  }

  try {
    // returns double, may throw if erroneous
    auto ret2 = divide_by<Policy::Exceptions>(1, 0);
    std::println("{}", ret2);
  } catch (std::exception &ex) {
    std::println("Caught: {}", ex.what());
  }
}
