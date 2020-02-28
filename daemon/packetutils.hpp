#pragma once
#include <utility>
#include <string>
#include <cstdint>
#include <functional>
#include <cstring>
#include <vector>
#include <sstream>

namespace packetutils
{
	std::tuple<int8_t, std::string> parse_packet(const std::string& packet);
	std::string assemble_packet(int8_t packettype, const std::string& content);
    std::vector<std::string> split_string(const std::string& str, const char delimiter);
};

