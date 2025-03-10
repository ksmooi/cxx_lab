// src/main.cpp

#include <iostream>
#include <memory>
#include <string>

// fmt includes
#include <fmt/format.h>
#include <fmt/ostream.h>

// Boost.ProgramOptions includes
#include <boost/program_options.hpp>

namespace po = boost::program_options;

// Base class for examples
class FmtExample {
public:
    virtual void run() = 0;
    virtual ~FmtExample() = default;
};

// Example 1: Basic Formatting
class BasicFormattingExample : public FmtExample {
public:
    void run() override {
        std::cout << "=== Basic Formatting Example ===" << std::endl;
        fmt::print("Hello, {}!\n", "World");
        fmt::print("The answer is {}.\n", 42);
    }
};

// Example 2: Formatting to String
class FormattingToStringExample : public FmtExample {
public:
    void run() override {
        std::cout << "=== Formatting to String Example ===" << std::endl;
        std::string formatted = fmt::format("This is a formatted number: {0:.2f}", 3.14159);
        std::cout << formatted << std::endl;
    }
};

// Example 3: Formatting with Variables
class FormattingWithVariablesExample : public FmtExample {
public:
    void run() override {
        std::cout << "=== Formatting with Variables Example ===" << std::endl;
        std::string name = "Alice";
        int age = 30;
        double height = 5.6;

        fmt::print("Name: {}, Age: {}, Height: {}\n", name, age, height);
    }
};

// Example 4: Custom Type Formatting
struct Person {
    std::string name;
    int age;
    double height;
};

// To enable formatting of custom types, specialize fmt::formatter
template <>
struct fmt::formatter<Person> {
    // Parses format specifications of the form 'f' or 'e'.
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
        // No custom format specifiers are handled in this example
        return ctx.begin();
    }

    // Formats the Person instance (must be const)
    template <typename FormatContext>
    auto format(const Person& p, FormatContext& ctx) const -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "Person(Name: {}, Age: {}, Height: {})", p.name, p.age, p.height);
    }
};

class CustomTypeFormattingExample : public FmtExample {
public:
    void run() override {
        std::cout << "=== Custom Type Formatting Example ===" << std::endl;
        Person person = {"Bob", 25, 5.9};
        fmt::print("{}\n", person);
    }
};

// Example 5: Advanced Formatting (Alignment, Width, Fill)
class AdvancedFormattingExample : public FmtExample {
public:
    void run() override {
        std::cout << "=== Advanced Formatting Example ===" << std::endl;
        fmt::print("{:<10} | {:^10} | {:>10}\n", "Left", "Center", "Right");
        fmt::print("{:*<10} | {:^10} | {:*>10}\n", "Left", "Center", "Right");
    }
};

// Example 6: Safe Formatting Example
class SafeFormattingExample : public FmtExample {
public:
    void run() override {
        std::cout << "=== Safe Formatting Example ===" << std::endl;
        try {
            // Correct usage
            fmt::print("Hello, {}!\n", "World");

            // Incorrect usage: missing argument
            // Updated to match the number of arguments
            fmt::print("This will cause an error: {}\n", "Only one argument");
        }
        catch (const fmt::format_error& e) {
            std::cerr << "Formatting error: " << e.what() << std::endl;
        }
    }
};

// Factory to create examples based on name
class ExampleFactory {
public:
    static std::unique_ptr<FmtExample> createExample(const std::string& name) {
        if (name == "basic") {
            return std::make_unique<BasicFormattingExample>();
        }
        else if (name == "string") {
            return std::make_unique<FormattingToStringExample>();
        }
        else if (name == "variables") {
            return std::make_unique<FormattingWithVariablesExample>();
        }
        else if (name == "custom") {
            return std::make_unique<CustomTypeFormattingExample>();
        }
        else if (name == "advanced") {
            return std::make_unique<AdvancedFormattingExample>();
        }
        else if (name == "safe") {
            return std::make_unique<SafeFormattingExample>();
        }
        else {
            return nullptr;
        }
    }
};

int main(int argc, char* argv[]) {
    // Define command-line options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Produce help message")
        ("example,e", po::value<std::string>(), "Specify which example to run: basic, string, variables, custom, advanced, safe");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch (const po::error& ex) {
        std::cerr << "Error parsing command-line arguments: " << ex.what() << std::endl;
        return 1;
    }

    if (vm.count("help") || !vm.count("example")) {
        std::cout << desc << "\n";
        std::cout << "Examples:\n";
        std::cout << "  " << argv[0] << " -e basic      Run basic formatting example\n";
        std::cout << "  " << argv[0] << " -e string     Run formatting to string example\n";
        std::cout << "  " << argv[0] << " -e variables  Run formatting with variables example\n";
        std::cout << "  " << argv[0] << " -e custom     Run custom type formatting example\n";
        std::cout << "  " << argv[0] << " -e advanced   Run advanced formatting example\n";
        std::cout << "  " << argv[0] << " -e safe       Run safe formatting example\n";
        return 0;
    }

    std::string example_name = vm["example"].as<std::string>();
    auto example = ExampleFactory::createExample(example_name);

    if (example) {
        example->run();
    }
    else {
        std::cerr << "Unknown example: " << example_name << "\n";
        std::cerr << desc << "\n";
        return 1;
    }

    return 0;
}
