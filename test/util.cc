#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "test/util.h"
#include "exception.h"

namespace ctk { namespace impl {
    using namespace ctk::api::v1;
    
    auto ignore_expected() -> void {
        try {
            throw;
        }
        // thrown by stl
        catch (const std::bad_alloc&) { }
        catch (const std::length_error&) { }
        catch (const std::invalid_argument&) { }
        catch (const std::out_of_range&) { }
        // thrown by ctk
        catch (const CtkLimit&) { }
        catch (const CtkData&) { }

        // not expected
        catch (const std::exception&) {
            throw;
        }
    }

        
    namespace test {

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


        input_txt::input_txt()
        : ifs{ "input.txt" } {
            if (!ifs.is_open()) {
                std::cerr << "no input.txt in the current working directory\n";
            }
        }

        auto input_txt::next() -> std::string {
            std::string fname;
            if (std::getline(ifs, fname)) {
                return trim(fname);
            }

            return std::string{};
        }

    } /* namespace test */
} /* namespace impl */ } /* namespace ctk */

