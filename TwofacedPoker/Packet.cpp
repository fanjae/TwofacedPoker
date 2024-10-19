#include "Packet.h"
#include <iostream>
#include <WinSock2.h>
std::string ReceivePacket(SOCKET clientSocket)
{
	char buffer[1025];
	uint32_t Packet_length = 0;
	int byte_length = recv(clientSocket, reinterpret_cast<char*>(&Packet_length), sizeof(Packet_length), 0);
	int errorCode = errno;
	char errorMessage[256];

	if (byte_length == 0)
	{
		std::cerr << "Recive : Connection closed." << strerror_s(errorMessage,sizeof(errorMessage),errorCode) << std::endl;
		return "";
	}
	if (byte_length < 0)
	{
		std::cerr << "Recive : Error" << strerror_s(errorMessage, sizeof(errorMessage), errorCode) << std::endl;
		return "";
	}

	Packet_length = ntohl(Packet_length);
	if (Packet_length > 1024)
	{
		std::cerr << "Packet too large. Packet_length value changed." << std::endl;
		Packet_length = 1024;
	}

	byte_length = recv(clientSocket, buffer, Packet_length, 0);
	if (byte_length <= 0)
	{
		std::cerr << "Received Error." << std::endl;
		return "";
	}
	buffer[Packet_length] = '\0';

	std::string test = buffer;
	std::cout << "[System] Receive " << Packet_length << " " << test << std::endl;
	
	return test;
}

void SendPacket(SOCKET clientSocket, const std::string& message)
{
	uint32_t Packet_length = htonl(message.size());

	send(clientSocket, reinterpret_cast<char*>(&Packet_length), sizeof(Packet_length), 0);

	std::cout << "[System] : Send message_size : " << message.size() << "message : " << message << std::endl;
	
	send(clientSocket, message.c_str(), message.size(), 0);
}
