#pragma once

#include <exception>
#include <iostream>

namespace fourier::aurora_sdk::testing {

class TestHarness {
public:
    void check(
        bool condition,
        const char* expression,
        const char* file,
        int line) {
        if (condition) {
            return;
        }
        ++failures_;
        std::cerr << file << ":" << line << ": CHECK(" << expression
                  << ") failed\n";
    }

    template <typename Exception, typename Callable>
    void checkThrows(
        Callable&& callable,
        const char* expression,
        const char* file,
        int line) {
        try {
            callable();
        } catch (const Exception&) {
            return;
        } catch (const std::exception& error) {
            ++failures_;
            std::cerr << file << ":" << line << ": " << expression
                      << " threw a different std::exception: "
                      << error.what() << "\n";
            return;
        } catch (...) {
            ++failures_;
            std::cerr << file << ":" << line << ": " << expression
                      << " threw a non-standard exception\n";
            return;
        }
        ++failures_;
        std::cerr << file << ":" << line << ": " << expression
                  << " did not throw\n";
    }

    [[nodiscard]] int result() const {
        if (failures_ == 0) {
            std::cout << "all checks passed\n";
            return 0;
        }
        std::cerr << failures_ << " check(s) failed\n";
        return 1;
    }

private:
    int failures_ = 0;
};

}  // namespace fourier::aurora_sdk::testing

#define SDK_CHECK(harness, expression) \
    (harness).check(                    \
        static_cast<bool>(expression),  \
        #expression,                    \
        __FILE__,                       \
        __LINE__)

#define SDK_CHECK_THROWS(harness, exception_type, expression) \
    (harness).checkThrows<exception_type>(                    \
        [&] { static_cast<void>(expression); },               \
        #expression,                                          \
        __FILE__,                                             \
        __LINE__)
