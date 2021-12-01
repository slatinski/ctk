#pragma once

#include <random>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <cstdio>
#include <string>


namespace ctk { namespace impl {
    
    auto ignore_expected() -> void;

    namespace test {


struct random_values
{
	std::mt19937 m_mersenne_twister;

	random_values() {
		reseed();
	}

	void reseed() {
		std::random_device random_device;
		const auto seed{ random_device() };
		std::cout << "using random seed " << seed << std::endl;

		m_mersenne_twister.seed(seed);
	}


	template<typename T>
	void fill(T lowest, T highest, std::vector<T> &output, bool include_minmax = true) {
		std::uniform_int_distribution<T> distribution(lowest, highest);

		for (size_t i = 0; i < output.size(); ++i) {
			output[i] = distribution(m_mersenne_twister);
		}

		if (include_minmax && !output.empty()) {
			std::uniform_int_distribution<size_t> dist(0, output.size() - 1);

			output[dist(m_mersenne_twister)] = lowest;
			output[dist(m_mersenne_twister)] = highest;
		}
	}

	void fill(double lowest, double highest, std::vector<double> &output, bool include_minmax = true) {
		std::uniform_real_distribution<double> distribution(lowest, highest);

		for (size_t i = 0; i < output.size(); ++i) {
			output[i] = distribution(m_mersenne_twister);
		}

		if (include_minmax && !output.empty()) {
			std::uniform_int_distribution<size_t> dist(0, output.size() - 1);

			output[dist(m_mersenne_twister)] = lowest;
			output[dist(m_mersenne_twister)] = highest;
		}
	}

	void fill(float lowest, float highest, std::vector<float> &output, bool include_minmax = true) {
		std::uniform_real_distribution<float> distribution(lowest, highest);

		for (size_t i = 0; i < output.size(); ++i) {
			output[i] = distribution(m_mersenne_twister);
		}

		if (include_minmax && !output.empty()) {
			std::uniform_int_distribution<size_t> dist(0, output.size() - 1);

			output[dist(m_mersenne_twister)] = lowest;
			output[dist(m_mersenne_twister)] = highest;
		}
	}

	void fill(uint8_t lowest, uint8_t highest, std::vector<uint8_t> &output, bool include_minmax = true) {
		std::vector<uint16_t> temp(output.size());
		fill<uint16_t>((uint16_t)lowest, (uint16_t)highest, temp, include_minmax);
		std::transform(std::begin(temp), std::end(temp), std::begin(output), [](uint16_t x) { return static_cast<uint8_t>(x); });
	}


	void fill(int8_t lowest, int8_t highest, std::vector<int8_t> &output, bool include_minmax = true) {
		std::vector<int16_t> temp(output.size());
		fill<int16_t>((int16_t)lowest, (int16_t)highest, temp, include_minmax);
		std::transform(std::begin(temp), std::end(temp), std::begin(output), [](int16_t x) { return static_cast<int8_t>(x); });
	}


	template<typename T>
	T pick(T lowest, T highest) {
		std::uniform_int_distribution<T> distribution(lowest, highest);
		return distribution(m_mersenne_twister);
	}

	uint8_t pick(uint8_t lowest, uint8_t highest) {
		return (uint8_t)pick<uint16_t>((uint16_t)lowest, (uint16_t)highest);
	}

	int8_t pick(int8_t lowest, int8_t highest) {
		return (int8_t)pick<int16_t>((int16_t)lowest, (int16_t)highest);
	}
};


auto divisors(ptrdiff_t n) -> std::vector<ptrdiff_t>;
auto trim(const std::string& str) -> std::string;
auto s2s(const std::string& str, size_t n) -> std::string;
auto d2s(double x) -> std::string;


struct input_txt
{
	std::ifstream ifs;

    input_txt()
    : ifs{ "input.txt" } {
        if (!ifs.is_open()) {
            std::cerr << "no input.txt in the current working directory\n";
        }
    }

    auto next() -> std::string {
        std::string fname;
        if (std::getline(ifs, fname)) {
            return trim(fname);
        }

        return std::string{};
    }
};





template<typename I>
auto sum(I first, I last) -> typename std::iterator_traits<I>::value_type {
	using T = typename std::iterator_traits<I>::value_type;
	return std::accumulate(first, last, T{ 0 });
}

template<typename I>
auto average(I first, I last) -> double {
	if (first == last) {
		return 0;
	}

    return sum(first, last) / static_cast<double>(std::distance(first, last));
}

template<typename T>
auto square(T x) -> T {
	return x * x;
}

template<typename I>
auto variance(I first, I last) -> double {
	if (first == last) {
		return 0;
	}

	using T = typename std::iterator_traits<I>::value_type;

	const double mean{ average(first, last) };
	const double sum{ std::accumulate(first, last, 0.0, [mean](double acc, T x) -> double { return acc + square(x - mean); }) };

	const auto length{ std::distance(first, last) };
	return sum / length;
}

template<typename I>
auto standard_deviation(I iter, I end) -> double {
    return std::sqrt(variance(iter, end));
}

} /* namespace test */ } /* namespace impl */ } /* namespace ctk */
