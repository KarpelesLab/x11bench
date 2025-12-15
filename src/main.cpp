#include "display.hpp"
#include "image.hpp"
#include "capture.hpp"
#include "compare.hpp"
#include "tests/test_base.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstring>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <thread>

namespace fs = std::filesystem;

// ANSI color codes
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"

struct Options {
    bool regenerate = false;
    bool verbose = false;
    bool list_only = false;
    bool save_failures = false;
    std::string reference_dir = "reference";
    std::string filter;
    std::string display_name;
};

void print_usage(const char* program) {
    std::cout << "Usage: " << program << " [options]\n"
              << "\nOptions:\n"
              << "  -h, --help           Show this help message\n"
              << "  -l, --list           List all available tests\n"
              << "  -r, --regenerate     Regenerate reference images\n"
              << "  -v, --verbose        Verbose output\n"
              << "  -f, --filter PATTERN Run only tests matching pattern\n"
              << "  -d, --display NAME   X11 display to connect to\n"
              << "  --ref-dir DIR        Directory for reference images (default: reference)\n"
              << "  --save-failures      Save captured images on test failures\n"
              << std::endl;
}

Options parse_args(int argc, char* argv[]) {
    Options opts;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            exit(0);
        } else if (arg == "-l" || arg == "--list") {
            opts.list_only = true;
        } else if (arg == "-r" || arg == "--regenerate") {
            opts.regenerate = true;
        } else if (arg == "-v" || arg == "--verbose") {
            opts.verbose = true;
        } else if (arg == "--save-failures") {
            opts.save_failures = true;
        } else if ((arg == "-f" || arg == "--filter") && i + 1 < argc) {
            opts.filter = argv[++i];
        } else if ((arg == "-d" || arg == "--display") && i + 1 < argc) {
            opts.display_name = argv[++i];
        } else if (arg == "--ref-dir" && i + 1 < argc) {
            opts.reference_dir = argv[++i];
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            print_usage(argv[0]);
            exit(1);
        }
    }

    return opts;
}

bool matches_filter(const std::string& name, const std::string& filter) {
    if (filter.empty()) return true;
    return name.find(filter) != std::string::npos;
}

int main(int argc, char* argv[]) {
    Options opts = parse_args(argc, argv);

    auto& tests = x11bench::get_test_registry();

    if (tests.empty()) {
        std::cerr << "No tests registered!" << std::endl;
        return 1;
    }

    // Sort tests by name for consistent ordering
    std::sort(tests.begin(), tests.end(),
              [](const x11bench::TestInfo& a, const x11bench::TestInfo& b) {
                  return a.name < b.name;
              });

    // List tests if requested
    if (opts.list_only) {
        std::cout << "Available tests (" << tests.size() << "):\n";
        for (const auto& test_info : tests) {
            auto test = test_info.factory();
            std::cout << "  " << test->name() << " - " << test->description() << "\n";
        }
        return 0;
    }

    // Create reference directory if needed
    if (!fs::exists(opts.reference_dir)) {
        fs::create_directories(opts.reference_dir);
    }

    // Connect to X display
    x11bench::Display display;
    if (!display.connect(opts.display_name)) {
        std::cerr << "Failed to connect to X display" << std::endl;
        return 1;
    }

    if (opts.verbose) {
        std::cout << "Connected to X display\n";
        std::cout << "XRender support: " << (display.has_xrender() ? "yes" : "no") << "\n";
    }

    // Run tests
    int passed = 0;
    int failed = 0;
    int skipped = 0;

    std::cout << "\n" << COLOR_BOLD << "Running X11 visual tests" << COLOR_RESET << "\n";
    std::cout << std::string(60, '=') << "\n\n";

    for (const auto& test_info : tests) {
        if (!matches_filter(test_info.name, opts.filter)) {
            skipped++;
            continue;
        }

        auto test = test_info.factory();
        std::string ref_path = opts.reference_dir + "/" + test->name() + ".png";

        std::cout << std::left << std::setw(35) << test->name() << " ";
        std::cout.flush();

        // Create window for this test
        display.destroy_window();
        if (!display.create_window(test->width(), test->height(), "x11bench - " + test->name())) {
            std::cout << COLOR_RED << "[ERROR]" << COLOR_RESET << " Failed to create window\n";
            failed++;
            continue;
        }

        display.show_window();

        // Wait for the window to be mapped and exposed
        if (!display.wait_for_expose(2000)) {
            if (opts.verbose) {
                std::cout << COLOR_YELLOW << "[WARN]" << COLOR_RESET << " Expose timeout\n";
            }
        }

        // Render the test pattern
        test->render(display);

        // Ensure all drawing commands are sent and processed
        display.flush();
        display.sync(false);

        // Additional sync to ensure X server has fully processed rendering
        // This is necessary because XSync only ensures commands are received,
        // not that they have been fully rasterized
        display.sync(false);

        // Small delay for complex rendering operations to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        display.sync(false);

        // Capture window content
        x11bench::Image captured;
        try {
            captured = x11bench::Capture::capture_window(display);
        } catch (const std::exception& e) {
            std::cout << COLOR_RED << "[ERROR]" << COLOR_RESET << " " << e.what() << "\n";
            failed++;
            continue;
        }

        // Handle reference image
        if (opts.regenerate || !fs::exists(ref_path)) {
            // Generate/regenerate reference
            if (captured.save_png(ref_path)) {
                std::cout << COLOR_BLUE << "[GENERATED]" << COLOR_RESET;
                if (opts.regenerate) {
                    std::cout << " (regenerated)";
                }
                std::cout << "\n";
                passed++;
            } else {
                std::cout << COLOR_RED << "[ERROR]" << COLOR_RESET << " Failed to save reference\n";
                failed++;
            }
        } else {
            // Compare with reference
            x11bench::Image reference;
            if (!reference.load_png(ref_path)) {
                std::cout << COLOR_RED << "[ERROR]" << COLOR_RESET << " Failed to load reference\n";
                failed++;
                continue;
            }

            x11bench::CompareResult result;
            if (test->allowed_diff_percent() > 0) {
                result = x11bench::Compare::fuzzy_percent(reference, captured,
                                                          test->allowed_diff_percent());
            } else {
                result = x11bench::Compare::fuzzy(reference, captured, test->tolerance());
            }

            if (result.match) {
                std::cout << COLOR_GREEN << "[PASS]" << COLOR_RESET;
                if (opts.verbose && result.different_pixels > 0) {
                    std::cout << " (" << result.different_pixels << " pixels within tolerance)";
                }
                std::cout << "\n";
                passed++;
            } else {
                std::cout << COLOR_RED << "[FAIL]" << COLOR_RESET << " " << result.message << "\n";
                failed++;

                if (opts.save_failures) {
                    std::string fail_path = opts.reference_dir + "/" + test->name() + "_fail.png";
                    std::string diff_path = opts.reference_dir + "/" + test->name() + "_diff.png";

                    captured.save_png(fail_path);

                    auto diff = x11bench::Compare::generate_diff(reference, captured, test->tolerance());
                    diff.save_png(diff_path);

                    if (opts.verbose) {
                        std::cout << "    Saved failure: " << fail_path << "\n";
                        std::cout << "    Saved diff: " << diff_path << "\n";
                    }
                }
            }
        }
    }

    display.destroy_window();
    display.disconnect();

    // Summary
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << COLOR_BOLD << "Summary:" << COLOR_RESET << "\n";
    std::cout << "  " << COLOR_GREEN << "Passed:  " << passed << COLOR_RESET << "\n";
    std::cout << "  " << COLOR_RED << "Failed:  " << failed << COLOR_RESET << "\n";
    if (skipped > 0) {
        std::cout << "  " << COLOR_YELLOW << "Skipped: " << skipped << COLOR_RESET << "\n";
    }
    std::cout << "  Total:   " << (passed + failed + skipped) << "\n";

    return failed > 0 ? 1 : 0;
}
