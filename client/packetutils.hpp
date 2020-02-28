#include <utility>
#include <string>
#include <cstdint>
#include <functional>
#include <cstring>

namespace packetutils
{
	std::tuple<int8_t, std::string> parse_packet(const std::string& packet);
	std::string assemble_packet(int8_t packettype, const std::string& content);
};

