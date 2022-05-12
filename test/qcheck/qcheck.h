#pragma once

#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <functional>
#include <utility>
#include <random>
#include <queue>
#include <cassert>
#include <iostream>
#include <iomanip>

namespace qcheck {

class random_source
{
    std::mt19937 engine;
    std::random_device device;

public:

    explicit random_source(std::random_device::result_type fixed_seed = 0) {
        const auto initial{ fixed_seed != 0 ? fixed_seed : device() };
        std::cerr << "using random seed " << initial << "\n";
        engine.seed(initial);
    }

    template<typename T>
    auto uniform_dist(T x, T y) -> T {
        static_assert(std::is_integral<T>::value);
        static_assert(1 < sizeof(T));
        std::uniform_int_distribution<T> dist(x, y);
        return dist(engine);
    }

    // keep in mind the limitations presented in https://hal.archives-ouvertes.fr/hal-03282794/document
    // "Drawing random floating-point numbers from an interval"
    auto uniform_dist(float x, float y) -> float {
        std::uniform_real_distribution<float> dist(x, y);
        return dist(engine);
    }

    auto uniform_dist(double x, double y) -> double {
        std::uniform_real_distribution<double> dist(x, y);
        return dist(engine);
    }
};


template<typename T>
auto choose(T x, T y, random_source& rnd) -> T {
    static_assert(std::is_integral<T>::value);
    static_assert(1 < sizeof(T));
    return rnd.uniform_dist(x, y);
}

auto choose(float x, float y, random_source& rnd) -> float {
    return rnd.uniform_dist(x, y);
}

auto choose(double x, double y, random_source& rnd) -> double {
    return rnd.uniform_dist(x, y);
}

auto choose(uint8_t x, uint8_t y, random_source& rnd) -> uint8_t {
    const uint16_t x16{ x };
    const uint16_t y16{ y };
    return static_cast<uint8_t>(choose(x16, y16, rnd));
}

auto choose(int8_t x, int8_t y, random_source& rnd) -> int8_t {
    const int16_t x16{ x };
    const int16_t y16{ y };
    return static_cast<int8_t>(choose(x16, y16, rnd));
}

// chooses an element from the interval [-size, size]
template<typename T>
auto choose_signed(size_t size, random_source& rnd, T n) -> T {
    static_assert(std::is_signed<T>::value);
    constexpr const auto int_max{ std::numeric_limits<T>::max() };
    constexpr const size_t uint_max{ static_cast<size_t>(int_max) };

    if (uint_max < size) {
        n = int_max;
    }
    else {
        n = static_cast<T>(size);
    }

    return choose(static_cast<T>(-n), n, rnd);
}

// chooses an element from the interval [0, size]
template<typename T>
auto choose_unsigned(size_t size, random_source& rnd, T n) -> T {
    static_assert(std::is_unsigned<T>::value);
    constexpr const auto int_max{ std::numeric_limits<T>::max() };

    if (int_max < size) {
        n = int_max;
    }
    else {
        n = static_cast<T>(size);
    }

    return choose(T{ 0 }, n, rnd);
}


auto gen(size_t, random_source& rnd, bool) -> bool {
    return choose(0, 1, rnd) == 0;
}

auto gen(size_t size, random_source& rnd, int8_t type_tag) -> int8_t {
    return choose_signed(size, rnd, type_tag);
}

auto gen(size_t size, random_source& rnd, uint8_t type_tag) -> uint8_t {
    return choose_unsigned(size, rnd, type_tag);
}

auto gen(size_t size, random_source& rnd, int16_t type_tag) -> int16_t {
    return choose_signed(size, rnd, type_tag);
}

auto gen(size_t size, random_source& rnd, uint16_t type_tag) -> uint16_t {
    return choose_unsigned(size, rnd, type_tag);
}

auto gen(size_t size, random_source& rnd, int32_t type_tag) -> int32_t {
    return choose_signed(size, rnd, type_tag);
}

auto gen(size_t size, random_source& rnd, uint32_t type_tag) -> uint32_t {
    return choose_unsigned(size, rnd, type_tag);
}

auto gen(size_t size, random_source& rnd, int64_t type_tag) -> int64_t {
    return choose_signed(size, rnd, type_tag);
}

auto gen(size_t size, random_source& rnd, uint64_t type_tag) -> uint64_t {
    return choose_unsigned(size, rnd, type_tag);
}

template<typename T>
auto gen(size_t n, random_source& rnd, std::vector<T>) -> std::vector<T> {
    std::vector<T> xs;

    if (n == 0) {
        return xs;
    }

    xs.reserve(n);
    for (size_t i{ 0 }; i < n; ++i) {
        xs.push_back(static_cast<T>(gen(n, rnd, T{})));
    }

    return xs;
}

auto gen(size_t n, random_source& rnd, std::string) -> std::string {
    using T = std::string::value_type;

    std::string xs;
    const auto ys{ gen(n, rnd, std::vector<T>{}) };
    xs.resize(ys.size());
    std::copy(begin(ys), end(ys), begin(xs));
    return xs;
}



template<typename T>
class make_vectors
{
    random_source* _random;

public:

    make_vectors(random_source& rnd, T)
    : _random{ &rnd } {
    }

    auto operator()(size_t n) -> std::vector<T> {
        return gen(n, *_random, std::vector<T>{});
    }
};

class make_strings
{
    random_source* _random;

public:

    make_strings(random_source& rnd)
    : _random{ &rnd } {
    }

    auto operator()(size_t n) -> std::string {
        return gen(n, *_random, std::string{});
    }
};



struct stats
{
    size_t successfull;
    size_t rejected;
    size_t trivial;
    std::vector<std::string> classes;

    explicit stats(size_t n)
    : successfull{ 0 }
    , rejected{ 0 }
    , trivial{ 0 } {
        classes.reserve(n);
    }
};

auto cerr_start(const std::string& property) -> void {
    std::cerr << "\n=== " << property << " ===\n";
}

auto cerr_passed(const stats& data, size_t iterations) -> void {
    std::cerr << "+++ OK, passed " << data.successfull << " tests";

    const double all{ static_cast<double>(iterations) };
    const double rejected{ static_cast<double>(data.rejected) };
    const double rejected_percent{ std::round(rejected / all * 100.0) };
    if (1 <= rejected_percent) {
        std::cerr << ", rejected " << rejected << "/" << all << " (" << rejected_percent << "%)";
    }

    const double processed{ static_cast<double>(data.classes.size()) };
    const double trivial{ static_cast<double>(data.trivial) };
    const double trivial_percent{ std::round(trivial / processed * 100.0) };
    if (1 <= trivial_percent) {
        std::cerr << ", trivial " << trivial << "/" << processed << " (" << trivial_percent << "%)";
    }
    std::cerr << "\n";

    auto sorted{ data.classes };
    std::sort(begin(sorted), end(sorted));
    auto categories{ sorted };
    auto last{ std::unique(begin(categories), end(categories)) };
    auto first{ begin(categories) };

    using stat_bin = std::pair<double, std::string>; // percentage, class
    std::vector<stat_bin> xs;
    xs.reserve(static_cast<size_t>(std::distance(first, last)));
    while (first != last) {
        if (first->empty()) { // default
            ++first;
            continue;
        }

        const auto[f, l]{ std::equal_range(begin(sorted), end(sorted), *first) };
        const double amount{ static_cast<double>(std::distance(f, l)) };
        const double percent{ std::round(amount / processed * 100.0) };
        xs.emplace_back(percent, *first);
        ++first;
    }

    const auto reverse_percentage_order = [](const stat_bin& x, const stat_bin& y) -> bool { return y.first < x.first; };

    std::sort(begin(xs), end(xs), reverse_percentage_order);
    for (const auto& x : xs) {
        std::cerr << std::setw(3) << x.first << "% " << x.second << "\n";
    }
}

template<typename P, typename T>
auto cerr_falsified(const P& property, const std::vector<T>& xs, size_t n) -> void {
    std::cerr << "*** Failed! Falsifiable (after " << n << " tests):\n";
    property.print(std::cerr, xs.back());
    std::cerr << "\n";
}

auto cerr_exhausted(const stats& data) -> void {
    std::cerr << "*** Gave up! Passed only " << data.successfull << " tests\n";
}

template<typename P, typename T>
auto cerr_exception(const P& property, const std::vector<T>& xs, const std::string& msg) -> void {
    std::cerr << "*** Failed! Exception '" << msg << "'\n";
    if (xs.empty()) {
        return;
    }

    property.print(std::cerr, xs.back());
    std::cerr << "\n";
}


template<typename T>
struct result
{
    bool successful;
    std::vector<T> generated;

    operator bool() const {
        return successful;
    }
};

template<typename P, typename G>
auto check(const std::string& name, P property, G generate, size_t n = 100) -> result<decltype(generate(0))> {
    stats data{ n };
    cerr_start(name);

    size_t max_n{ n * 5 };
    if (max_n < n) {
        max_n = std::numeric_limits<size_t>::max();
    }

    using T = decltype(generate(0));
    std::vector<T> xs;
    xs.reserve(n);

    try {
        for (size_t i{ 0 }; i < max_n; ++i) {
            xs.push_back(generate(i));
            const T& x{ xs.back() };

            if (!property.accepts(x)) {
                ++data.rejected;
                continue;
            }
            data.classes.push_back(property.classify(x));

            if (property.trivial(x)) {
                ++data.trivial;
            }

            if (!property.holds(x)) {
                cerr_falsified(property, xs, i);
                return { false, xs };
            }
            ++data.successfull;

            if (data.successfull == n) {
                break;
            }
        }
    }
    catch(const std::exception& e) {
        cerr_exception(property, xs, e.what());
        return { false, xs };
    }

    if (data.successfull != n) {
        cerr_exhausted(data);
        return { false, xs };
    }

    cerr_passed(data, xs.size());
    return { true, xs };
}


template<typename T>
struct arguments
{
    using argument_type = T;

    virtual ~arguments() = default;

    virtual auto accepts(const T&) const -> bool {
        return true;
    }

    virtual auto trivial(const T&) const -> bool {
        return false;
    }

    virtual auto holds(const T&) const -> bool {
        return false;
    }

    virtual auto classify(const T&) const -> std::string {
        return {};
    }

    virtual auto print(std::ostream& os, const T&) const -> std::ostream& {
        os << "*** print is not implemented for this property\n";
        return os;
    }
};

} // namespace qcheck

