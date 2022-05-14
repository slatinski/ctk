#pragma once

#include <iosfwd>
#include <string>

namespace ctk { namespace impl {
    
    auto ignore_expected() -> void;

    namespace test {

        auto trim(const std::string& str) -> std::string;
        auto s2s(const std::string& str, size_t n) -> std::string;
        auto d2s(double x) -> std::string;

        class input_txt
        {
            std::ifstream ifs;

            public:

            input_txt();
            auto next() -> std::string;
        };

    } /* namespace test */

} /* namespace impl */ } /* namespace ctk */

