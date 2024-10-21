#pragma once
#include <string>
#include <winsock2.h>
std::string ReceivePacket(SOCKET clientSocket);
void SendPacket(SOCKET clientSocket, const std::string& message);