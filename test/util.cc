#include "test/util.h"
#include "exception.h"
#include <sstream>
#include <iomanip>
#include <cassert>

namespace ctk { namespace impl {
    using namespace ctk::api::v1;
    
    auto ignore_expected() -> void {
        try {
            throw;
        }
        // thrown by stl
        catch (const std::bad_alloc& e) {
            std::cerr << " " << e.what() << "\n"; // internal library limitation. preventable by input size reduction.
        }
        catch (const std::length_error& e) {
            std::cerr << " " << e.what() << "\n"; // internal library limitation. preventable by input size reduction.
        }
        /*
        catch (const std::ios_base::failure& e) {
            std::cerr << " " << e.what() << "\n"; // string io
        }
        */
        catch (const std::invalid_argument& e) {
            std::cerr << " " << e.what() << "\n"; // garbage data. stol, stod
        }
        catch (const std::out_of_range& e) {
            std::cerr << " " << e.what() << "\n"; // garbage data. stol, stod
        }
        // thrown by ctk
        catch (const ctk_limit& e) {
            std::cerr << " " << e.what() << "\n"; // internal library limitation. preventable by input size reduction.
        }
        catch (const ctk_data& e) {
            std::cerr << " " << e.what() << "\n"; // garbage data
        }

        // not expected
        catch (const ctk_bug& e) {
            std::cerr << " " << e.what() << "\n"; // bug in this library
            throw;
        }
    }

        
    namespace test {

std::vector<ptrdiff_t> divisors(ptrdiff_t n) {
    assert(n >= 0);

	const ptrdiff_t range{ static_cast<ptrdiff_t>(sqrt(double(n))) + 1 };

	std::vector<ptrdiff_t> result;
	result.reserve(static_cast<size_t>(range));

	for (ptrdiff_t i{1}; i < range; ++i) {
		const auto x{ std::div(n, i) };
		if (x.rem == 0) {
			result.push_back(i);
			result.push_back(x.quot);
		}
	}

	std::sort(std::begin(result), std::end(result));
	const auto last{ std::unique(std::begin(result), std::end(result)) };
	result.resize(size_t(std::distance(std::begin(result), last)));

	std::reverse(std::begin(result), std::end(result));

	return result;
}


auto trim(const std::string& str) -> std::string {
    static const std::string whitespace{ " \t\n\r" };

    typedef std::string::size_type size_type;

    const size_type first{ str.find_first_not_of(whitespace) };
    if (first == std::string::npos)
        return std::string{}; // no content

    const size_type last_pos{ str.find_last_not_of(whitespace) };
    if (last_pos < first + 1) {
        return std::string{};
    }

    const size_type length{ last_pos - first + 1 };
    return str.substr(first, length);
}


auto s2s(const std::string& str, size_t n) -> std::string {
    if (str.size() < n) {
        const size_t missing{ n - str.size() };
        std::string spaces(missing, ' ');
        return spaces + str;
    }

    return str.substr(str.size() - n, n);
}

auto d2s(double x) -> std::string {
    std::ostringstream os0;
    os0 << std::fixed << std::setprecision(2) << x;

    std::ostringstream os1;
    os1 << std::setw(7) << os0.str();
    return os1.str();
}



} /* namespace test */ } /* namespace impl */ } /* namespace ctk */
