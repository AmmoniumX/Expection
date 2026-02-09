# Expection

Expection is a tiny, C++23 header-only library for compile-time polymorphic error handling. It allows you to write a function once, and handle "erroneous" states however the end-user desires, either by throwing exceptions on error, or returning an std::unexpected. The aim of Expection is to give full control to both library developers and end users of how they want to handle errors, and have a simple way to toggle between them without having to refactor every function to do so. The error "dispatching" logic is handled entirely at compile time: there are no runtime costs such as by wrapping std::expected with an exception or vice versa. On error, you either construct and return an std::unexpected value, or you throw an exception, not both.

## Installation

Expection requires minimum C++23 support.

To get it, simply add `Expection.hpp` to your project and include it.

## Brief API Summary

### Types
- `Policy::Expected` - Returns `std::expected<T, E>`
- `Policy::Exceptions` - Returns `T`, throws on error
- `ResultType<T, E, P>` - Resolves to the appropriate return type

### Functions
- `make_failure<Result, Error, Policy>(args...)` - Constructs failure state

## Example

For an example, let's say we want to "improve" an existing function like such:

```cpp
double divide(int a, int b); // throws on error. But what if user wants std::expected?
```

Using Expection is simple, there are multiple ways that it can construct exceptions or expected return types, one of the simplest is to add an "static auto exception()" method to your Error class:

```cpp
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
```

Now, you can use this Error class when returning a failure:
```cpp
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
  return success<Result, Error>(val); // or just return val; effectively the same
}
```
When you call `make_failure<Result, Error, P>(args...)`, Expection will either `throw Error::exception(args...)` if the policy is set to Exceptions, or `return std::unexpected<Error>(args...)` in the case of Expected.

`Expection::DefaultPolicy` is assigned via the `EXPECTION_DEFAULTPOLICY` macro, set to `Exceptions` by default. To change it simply define it to something, such as by adding `-DEXPECTION_DEFAULTPOLICY=Expected` to your compile commands.

To use this library, you can do any of the following:

```cpp
#include <print>

int main() {
    // Default policy is exceptions unless macro is defined to something else
    auto success = divide_by(1, 2);

    try {
        auto error = divide_by(1, 0);
    } catch (std::exception &ex) {
        std::println("Caught: {}", ex.what());
    }

    // The user can specify any policy (overriding whatever the default one is) according to their needs
    using Expection::Policy;
    auto ret1 = divide_by<Policy::Expected>(1, 2); // returns std::expected<double, DivideByError>

    auto ret2 = divide_by<Policy::Exceptions>(1, 2); // returns double, may throw if erroneous
}
```

That's it! Now, you have a single (templated) function `divide_by`, which can either return `std::expected` values or throw exceptions based on the compile-time error handling policy.

## Limitations

Naturally, for Expection to be able to "dispatch" the right error handling at compile time, your functions must become templated functions taking a `Expected::Policy` as a parameter. However, if this is the only template parameter, you can make use of explicit template instantiations in case you don't want to define your functions inside headers. 

If you are using this for distributing dynamically linked libraries, it will effectively double the binary size of your function definitions. To counter this, you could just write your functions taking a non-templated `Expection::DefaultPolicy`: this way, it's up to the one compiling your library to choose whether they want to keep the default policy as exceptions, or add a compile option to redefined it to expected as shown above.
