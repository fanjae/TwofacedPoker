#pragma once
#include "Game_Manager.h"
#include "Client_Handle.h"

Game_Manager::Game_Manager(const std::string& roomName) : roomName(roomName), deck() {}
bool Game_Manager::Handle_Game_Event(const SOCKET& socket, const std::string& message)
{
	std::cout << message << std::endl;
	if (message.substr(0, GAME_READY.length()) == GAME_READY)
	{
		Handle_Game_Ready(socket, message.substr(GAME_READY.length()));
	}
	if (message.substr(0, GAME_PRE_CALL.length()) == GAME_PRE_CALL)
	{
		Handle_Game_Pre_Call(socket, message.substr(GAME_PRE_CALL.length()));
	}
	if (message.substr(0, GAME_PRE_LOAD.length()) == GAME_PRE_LOAD)
	{
		Handle_Game_Pre_Load(socket, message.substr(GAME_PRE_LOAD.length()));
	}
	return true;
}

void Game_Manager::Handle_Game_Ready(const SOCKET& socket, const std::string& message)
{
	const auto& rooms = GetChatRooms();
	auto it = rooms.find(this->roomName);
	std::string send_message = GAME_CLIENT_EVENT + message;

	std::cout << send_message << std::endl;
	for (SOCKET target_socket : it->second)
	{
		if (socket != target_socket)
		{
			send(target_socket, send_message.c_str(), send_message.length(), 0);
		}
	}
}
void Game_Manager::Handle_Game_Pre_Call(const SOCKET& socket, const std::string& message)
{
	const auto& rooms = GetChatRooms();
	const auto& room_counts = GetRoomCounts();

	auto targetRoom = rooms.find(this->roomName);
	auto targetRoom_count = room_counts.find(this->roomName);
	std::string join_message;
	for (SOCKET target_socket : targetRoom->second)
	{
		if (socket != target_socket && targetRoom_count->second > 1)
		{
			join_message = GAME_CLIENT_EVENT + GAME_PRE_LOAD_PLAYER;
			std::cout << join_message << std::endl;
			send(target_socket, join_message.c_str(), join_message.length(), 0);
		}
	}
}

void Game_Manager::Handle_Game_Pre_Load(const SOCKET& socket, const std::string& message)
{
	const auto& rooms = GetChatRooms();
	auto it = rooms.find(this->roomName);
	std::string send_message = "";

	std::cout << "Message :" << message << std::endl;
	std::cout << "Cut : " << message.substr(0, 3) <<std::endl;
	if (message.substr(0, 3) == "ID ")
	{
		send_message = GAME_CLIENT_EVENT + GAME_PRE_LOAD_ID_DONE + message.substr(3);
		std::cout << send_message << std::endl;
	}
	else if (message.substr(0, 5) == "READY")
	{
		send_message = GAME_CLIENT_EVENT + GAME_PRE_LOAD_READY_DONE;
		std::cout << send_message << std::endl;
	}
	else if (message.substr(0, 4) == "Done")
	{
		send_message = GAME_CLIENT_EVENT + GAME_PRE_LOAD_DONE_DONE;
		std::cout << send_message.length() << std::endl;
		std::cout << send_message << std::endl;
	}

	for (SOCKET target_socket : it->second)
	{
		if (socket != target_socket)
		{
			send(target_socket, send_message.c_str(), send_message.length(), 0);
		}
	}

}