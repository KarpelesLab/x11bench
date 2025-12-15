#pragma once

#include "../display.hpp"
#include "../image.hpp"
#include <memory>
#include <string>
#include <vector>

namespace x11bench {

class TestBase {
public:
    virtual ~TestBase() = default;

    // Test identification
    virtual std::string name() const = 0;
    virtual std::string description() const = 0;

    // Window dimensions for this test
    virtual uint32_t width() const { return 256; }
    virtual uint32_t height() const { return 256; }

    // Render the test pattern
    virtual void render(Display& display) = 0;

    // Tolerance for comparison (0 = exact match, higher = more fuzzy)
    virtual int tolerance() const { return 0; }

    // Optional: percentage of pixels allowed to differ
    virtual double allowed_diff_percent() const { return 0.0; }
};

// Factory function type for creating tests
using TestFactory = std::unique_ptr<TestBase>(*)();

// Test registration
struct TestInfo {
    std::string name;
    TestFactory factory;
};

// Get all registered tests
std::vector<TestInfo>& get_test_registry();

// Register a test
void register_test(const std::string& name, TestFactory factory);

// Macro for easy test registration
#define REGISTER_TEST(TestClass) \
    static struct TestClass##Registrar { \
        TestClass##Registrar() { \
            register_test(#TestClass, []() -> std::unique_ptr<TestBase> { \
                return std::make_unique<TestClass>(); \
            }); \
        } \
    } TestClass##registrar_instance;

} // namespace x11bench
