#include "ClientHandle.h"
#include "ClientConnection.h"
#include "../Common/Constants.h"
#include "../Game/GameManager.h"
#include "../Room/RoomManager.h"
#include "../Room/RoomRegistry.h"
#include "../Protocol/Packet.h"
#include "../Protocol/ClientCommandParser.h"

#include <atomic>
#include <exception>
#include <iostream>
#include <optional>
#include <system_error>
#include <utility>



namespace
{
    // 방 관리와 관계없는 클라이언트 번호만 유지
    std::atomic<int> nextClientNumber{ 1 };
}

ClientEventHandler::ClientEventHandler(std::shared_ptr<ClientConnection> clientConnection,std::shared_ptr<RoomRegistry> registry) : roomRegistry(std::move(registry)), connection(std::move(clientConnection)), socket(connection->GetSocket()), userNumber(-1), roomNumber(-1)
{
}

// 직접 send() 하지 않고 연결별 송신 큐에 넣어 패킷 전송 순서 보장
bool ClientEventHandler::sendMessage(std::string message)
{
    if (!connection)
    {
        return false;
    }

    if (!connection->Enqueue(std::move(message)))
    {
        std::cerr << "[Network] Failed to queue message. Socket: " << socket << '\n';
        return false;
    }

    return true;
}

bool ClientEventHandler::handleMessage(const std::string& message)
{
    // 원본 문자열을 명령 종류와 payload 형태로 분리하여 이벤트 처리
    const ParsedClientCommand command = ClientCommandParser::parse(message);

    switch (command.type)
    {
        case ClientCommandType::GET_ROOMS:
            Handle_Get_Chatting_Room();
            break;

        case ClientCommandType::EXIT_ROOM:
            Handle_Exit_Room();
            break;

        case ClientCommandType::LOGIN:
            Handle_Login();
            break;

        case ClientCommandType::USER_UPDATE:
            Handle_User_Update();
            break;

        case ClientCommandType::CREATE_ROOM:
            Handle_Create_Chatting_Room(command.payload);
            break;

        case ClientCommandType::JOIN_ROOM:
            Handle_Join_Chatting_Room(command.payload);
            break;

        case ClientCommandType::CLOSE_SOCKET:
            return false;

        case ClientCommandType::ROOM_EVENT:
        {
            // 방 이벤트는 현재 입장한 방의 RoomManager만 처리
            const auto roomManager = roomRegistry->findRoom(roomNumber);

            if (!roomManager)
            {
                sendMessage(NOT_EXIST_ROOM);
                return false;
            }

            roomManager->Handle_Room_Event(socket,command.payload);

            break;
        }

        case ClientCommandType::GAME_EVENT:
        {
            // 게임 이벤트는 현재 입장한 방과 함께 생성된 GameManager에 전달
            const auto gameManager = roomRegistry->findGame(roomNumber);

            if (!gameManager)
            {
                sendMessage(NOT_EXIST_ROOM);
                return false;
            }

            gameManager->Handle_Game_Event(socket,command.payload);
            break;
        }

        case ClientCommandType::CHAT:
        {
            // 발신자 ID를 서버에서 붙여 클라이언트가 다른 ID를 사칭하게 못하게 처리
            const auto roomManager = roomRegistry->findRoom(roomNumber);

            if (!roomManager)
            {
                sendMessage(NOT_EXIST_ROOM);
                return false;
            }

            const std::string chatMessage = "[" + ID + "]" + command.payload;
            roomManager->broadcast_Message(chatMessage,socket,TargetType::ALL);

            break;
        }
    }

    return true;
}

void ClientEventHandler::Handle_Get_Chatting_Room()
{
    // 입장 가능한 방 목록을 클라이언트가 해석하는 번호 이름 형식으로 직렬화
    std::string roomList;

    const std::vector<RoomSummary> availableRooms = roomRegistry->listAvailable();

    for (const RoomSummary& room : availableRooms)
    {
        roomList += std::to_string(room.roomNumber);
        roomList += ' ';
        roomList += room.roomName;
        roomList += '\n';
    }

    if (roomList.empty())
    {
        roomList = NO_ROOM;
    }

    sendMessage(roomList);
}

void ClientEventHandler::Handle_Exit_Room()
{
    // 이후 상태를 먼저 초기화, 기존 방 번호는 지역 변수에 보과
    const int exitingRoomNumber = roomNumber;

    if (exitingRoomNumber < 0)
    {
        return;
    }

    const auto context = roomRegistry->find(exitingRoomNumber);

    // 중복 퇴장 요청을 막기 위해 먼저 상태 변경
    roomNumber = -1;

    if (!context || !context->roomManager)
    {
        return;
    }

    const auto roomManager = context->roomManager;
    const auto gameManager = context->gameManager;

    try
    {
        bool removed = false;

        if (gameManager)
        {
            // 게임 중 퇴장까지 처리할 수 있도록 GameMAnager를 우선 경유
            removed = gameManager->handlePlayerExit(userNumber,ID,socket);
        }
        else
        {
            // 비정상 RoomContext에 대한 방어 코드
            removed = roomManager->removeUser(userNumber,ID,socket,false);
        }

        if (!removed)
        {
            return;
        }

        // 빈 방인 경우 Registry에서 제거
        roomRegistry->eraseIfEmpty(exitingRoomNumber,roomManager);
    }
    catch (const std::exception& exception)
    {
        std::cerr << "[Room] Failed to exit room. Error: " << exception.what() << '\n';
    }
    catch (...)
    {
        std::cerr << "[Room] Unknown exception while exiting room.\n";
    }
}

void ClientEventHandler::Handle_Create_Chatting_Room(const std::string& roomName)
{
    // 로그인하지 않은 사용자
    if (userNumber < 0)
    {
        sendMessage(NOT_EXIST_ROOM);
        return;
    }

    // 이미 다른 방에 들어가 있는 사용자
    if (roomNumber >= 0)
    {
        sendMessage(EXIST_ROOM);
        return;
    }

    const auto createdRoom = roomRegistry->createAndJoin(roomName,userNumber,ID,connection);

    if (!createdRoom)
    {
        std::cerr << "[Room] Failed to create room.\n";

        sendMessage(NOT_EXIST_ROOM);
        return;
    }

    // 실제 생성과 입장이 끝난 후 상태 변경
    roomNumber = createdRoom->roomNumber;

    std::cout << "[Room] Created. Number: " << roomNumber << ", Name: " << roomName << '\n';

    sendMessage(std::to_string(roomNumber));
}

void ClientEventHandler::Handle_Join_Chatting_Room(const std::string& roomNumberText)
{
    // 로그인하지 않은 사용자
    if (userNumber < 0)
    {
        sendMessage(NOT_EXIST_ROOM);
        return;
    }

    int requestedRoomNumber = -1;
    std::size_t parsedLength = 0;

    try
    {
        requestedRoomNumber = std::stoi(roomNumberText,&parsedLength);

        // "123abc"와 같이 일부만 숫자인 입력 거부
        if (parsedLength != roomNumberText.size())
        {
            sendMessage(NOT_EXIST_ROOM);
            return;
        }
    }
    catch (const std::invalid_argument&)
    {
        sendMessage(NOT_EXIST_ROOM);
        return;
    }
    catch (const std::out_of_range&)
    {
        sendMessage(NOT_EXIST_ROOM);
        return;
    }

    // 방 생성 후 클라이언트가 동일 방으로 JOIN을 다시 요청한 경우
    if (roomNumber == requestedRoomNumber)
    {
        const auto roomManager = roomRegistry->findRoom(requestedRoomNumber);

        if (!roomManager)
        {
            roomNumber = -1;
            sendMessage(NOT_EXIST_ROOM);
            return;
        }

        sendMessage(roomManager->getroomName());
        return;
    }

    // 이미 다른 방에 입장한 상태
    if (roomNumber >= 0)
    {
        sendMessage(EXIST_ROOM);
        return;
    }

    const auto roomManager = roomRegistry->tryJoin(requestedRoomNumber,userNumber,ID,connection);

    if (!roomManager)
    {
        sendMessage(NOT_EXIST_ROOM);
        return;
    }

    // 입장이 성공한 후에만 상태 변경
    roomNumber = requestedRoomNumber;

    sendMessage(roomManager->getroomName());

    roomManager->broadcast_Message(ID + " Joined.",socket,TargetType::OTHERS);
}

void ClientEventHandler::Handle_User_Update()
{
    // 같은 방의 상대방 정보가 필요한 경우에만 사용자 갱신을 요청
    const auto roomManager = roomRegistry->findRoom(roomNumber);

    if (!roomManager)
    {
        std::cerr << "[System] Room not found. Number: " << roomNumber << '\n';
        sendMessage(NOT_EXIST_ROOM);
        return;
    }

    if (roomManager->getroomCount() <= 1)
    {
        std::cerr << "[System] Not enough users. Number: " << roomNumber << '\n';
        return;
    }

    roomManager->userUpdate(socket);
}

void ClientEventHandler::Handle_Login()
{
    // 중복 로그인 요청
    if (userNumber >= 0)
    {
        sendMessage("ID : " + ID);
        return;
    }

    // 번호의 유일성만 필요.
    userNumber = nextClientNumber.fetch_add(1,std::memory_order_relaxed);
    ID = "Guest" + std::to_string(userNumber);

    sendMessage("ID : " + ID);
}

void ConnectClient(std::shared_ptr<ClientConnection> connection,std::shared_ptr<RoomRegistry> roomRegistry)
{
    if (!connection)
    {
        return;
    }

    const SOCKET clientSocket = connection->GetSocket();

    if (clientSocket == INVALID_SOCKET)
    {
        return;
    }

    ClientEventHandler handler(connection,std::move(roomRegistry));

    std::cout << "[System] Client connected. Socket: " << clientSocket << '\n';

    try
    {
        while (true)
        {
            // 길이 헤더를 포함한 패킷 단위로 수신.
            // 연결 종료 뜨는 오류 시 nullopt를 반환
            const std::optional<std::string> message = ReceivePacket(clientSocket);

            if (!message)
            {
                break;
            }

            if (!handler.handleMessage(*message))
            {
                break;
            }
        }
    }
    catch (const std::exception& exception)
    {
        std::cerr << "[System] Client handler exception. Socket: " << clientSocket << ", Error: " << exception.what() << '\n';
    }
    catch (...)
    {
        std::cerr << "[System] Unknown client handler exception. Socket: " << clientSocket << '\n';
    }

    // 연결을 닫기 전에 방에서 제거
    handler.Disconnect();

    // recv/send 종료, 송신 스레드 join, 소켓 닫기
    connection->Stop();

    std::cout << "[System] Client disconnected. Socket: " << clientSocket << '\n';
}

void ClientEventHandler::Disconnect()
{
    // 소켓이 끊겨도 정상 퇴장과 같은 경로를 사용해 방과 게임 상태를 정리
    if (roomNumber >= 0)
    {
        Handle_Exit_Room();
    }
}