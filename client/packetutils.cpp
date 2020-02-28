#include "packetutils.hpp"

std::tuple<int8_t, std::string> packetutils::parse_packet(const std::string& packet)
{
        if(packet.size() < 2)
                return {};

        int8_t packet_type = *(int8_t*)&packet[0];
        std::string packet_contents = std::string(packet, 1, packet.size() - 1);
        return std::make_tuple(packet_type, packet_contents);
}

std::string packetutils::assemble_packet(int8_t packet_type, const std::string& content)
{
        std::string packet(content.size() + 1, '\0');
        *(int8_t*)&packet[0] = packet_type;
        memcpy(&packet[1], &content[0], content.size());
        return packet;
}

