// Copyright 2013 Chrisname. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY CHRISNAME ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL CHRISNAME OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
// OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// The views and conclusions contained in the software and documentation are
// those of the authors and should not be interpreted as representing official
// policies, either expressed or implied, of Chrisname.
#include <cppscan.hpp>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static std::vector<std::string> split(const std::string& string,
                                      char delimiter = ' ',
                                      bool allow_empty = false)
{
    std::vector<std::string> tokens;
    std::stringstream sstream(string);
    std::string token;
    while (std::getline(sstream, token, delimiter)) {
        if (allow_empty || token.size() > 0)
            tokens.push_back(token);
    }
    return tokens;
}

static int string_to_int(const std::string& string)
{
    std::stringstream sstream(string);
    int i;
    sstream >> i;
    return i;
}

template <typename T>
static void swap(T& a, T& b)
{
    T c = a;
    a = b;
    b = c;
}

template <typename T>
static std::vector<T> range(T min, T max)
{
    if (min > max)
        swap(min, max);
    if (min == max)
        return std::vector<T>(1, min);
    std::vector<T> values;
    for (; min <= max; ++min)
        values.push_back(min);
    return values;
}

static std::vector<int> parse_ports_list(const std::string& list)
{
    std::vector<int> ports;
    for (const std::string& token : split(list, ',')) {
        std::vector<std::string> strrange = split(token, '-');
        switch (strrange.size()) {
            case 0: ports.push_back(string_to_int(token));       break;
            case 1: ports.push_back(string_to_int(strrange[0])); break;
            case 2:
            {
                int min = string_to_int(strrange[0]),
                    max = string_to_int(strrange[1]);
                for (int port : range(min, max))
                    ports.push_back(port);
                break;
            }
            default:
                break;
        }
    }
    return ports;
}

template <typename T>
static T maximum(const std::vector<T>& values)
{
    T max = values[0];
    for (T value : values) {
        if (value > max)
            max = value;
    }
    return max;
}

template <typename T>
static size_t digits(T value)
{
    size_t count = (value < 0) ? 1 : 0;
    if (value == 0)
        return 0;
    do {
        ++count;
    } while (value /= 10);
    return count;
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " address port(s)\n"
                  << "Examples:\n"
                  << "\t" << argv[0] << " 127.0.0.1 80\n"
                  << "\t" << argv[0] << " localhost 80,8080\n"
                  << "\t" << argv[0] << " 192.0.43.10 0-65535\n"
                  << "\t" << argv[0] << " example.com 0-21,80,8080"
                  << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::string address = argv[1];
    std::vector<int> ports = parse_ports_list(std::string(argv[2]));
    size_t width = digits(maximum(ports));
    std::cout << "cppscan: Scanning " << address << "...\n";
    for (int port : ports) {
        bool open = cppscan::port_scanner(address, port).is_open();
        if (open)
            std::cout << std::setw(width) << port << " : OPEN\n";
    }
    std::cout << std::flush;
    return 0;
}
