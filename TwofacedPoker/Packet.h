#pragma once

#include <WinSock2.h>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

constexpr std::uint32_t MAX_PACKET_SIZE = 1024;

std::optional<std::string> ReceivePacket(SOCKET socket); 
bool SendPacket(SOCKET socket, std::string_view message);