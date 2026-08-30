#pragma once

#include <memory>
#include <string>
#include <WinSock2.h>

class ClientConnection;
class RoomRegistry;

void ConnectClient(std::shared_ptr<ClientConnection> connection, std::shared_ptr<RoomRegistry> roomRegistry);

class ClientEventHandler
{
public:
    ClientEventHandler(std::shared_ptr<ClientConnection> connection,std::shared_ptr<RoomRegistry> roomRegistry);

    bool handleMessage(const std::string& message);
    void Disconnect();

private:
    bool sendMessage(std::string message);

    void Handle_Get_Chatting_Room();
    void Handle_Exit_Room();
    void Handle_Create_Chatting_Room(
        const std::string& roomName
    );
    void Handle_Join_Chatting_Room(
        const std::string& roomNumberText
    );
    void Handle_User_Update();
    void Handle_Login();

    std::shared_ptr<RoomRegistry> roomRegistry;
    std::shared_ptr<ClientConnection> connection;

    SOCKET socket;
    int userNumber;
    int roomNumber;
    std::string ID;
};