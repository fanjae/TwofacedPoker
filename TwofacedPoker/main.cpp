#include <iostream>
#include <atomic>

#include <WinSock2.h>
#include <Windows.h>
#include <stdexcept>
#include <string>

#include "Room/RoomRegistry.h"
#include "Network/ClientConnection.h"
#include "Network/ClientWorkerManager.h"

#include <memory>

namespace
{
	// 콘솔 제어 헨들러와 메인 스레드가 함께 확인하는 종료 요청 플래그
	// 복잡한 종료 작업을 수행하지 않고 플래그만 변경
	std::atomic<bool> stopRequested{ false };

	// Ctrl+C 또는 Ctrl+Break 입력을 정상 종료 요청으로 전환
	BOOL WINAPI HandleConsoleControl(DWORD controlType)
	{
		if (controlType == CTRL_C_EVENT	|| controlType == CTRL_BREAK_EVENT)
		{
			stopRequested.store(true,std::memory_order_relaxed);
			return TRUE;
		}

		return FALSE;
	}
}

int main(int argc, char* argv[])
{
	WSADATA wsaData{} ;
	SOCKET serverSocket = INVALID_SOCKET;
	SOCKADDR_IN serverAddr{};

	// 한글을 포함한 서버 로그가 Windows 콘솔에 UTF-8로 출력되도록 설정
	SetConsoleOutputCP(CP_UTF8);

	// 실행 인자는 서버가 수신 대기할 포트 하나만 허용
	if (argc != 2)
	{
		std::cerr << "Usage : " << argv[0] << "  <port>";
		return 1;
	}

	int port = 0;
	std::size_t parsedLength = 0;

	try
	{
		const std::string portText = argv[1];

		// stoi가 실제로 해석한 문자 수를 받아 숫자 뒤의 불필요한 문자열도 검출한다.
		port = std::stoi(portText, &parsedLength);

		if (parsedLength != portText.size())
		{
			std::cerr << "Invalid port: " << portText << '\n';
			return 1;
		}

		if (port < 1 || port > 65535)
		{
			std::cerr << "Port must be between 1 and 65535.\n";
			return 1;
		}
	}
	catch (const std::invalid_argument&)
	{
		std::cerr << "Port must be a number.\n";

		return 1;
	}
	catch (const std::out_of_range&)
	{
		std::cerr << "Port is out of range.\n";

		return 1;
	}


	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cerr << "WSAStartup() Error" << std::endl;
		return 1;
	}

	// IPv4 기반 TCP 수신 소켓 생성
	serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET)
	{
		std::cerr << "socket() Error: " << WSAGetLastError() << '\n';

		WSACleanup();
		return 1;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY; // 사용 가능한 모든 로컬 주소에 접속을 받음
	serverAddr.sin_port = htons(static_cast<unsigned short>(port));

	// 생성한 소켓을 지정된 로컬 포트에 연결
	if (bind(serverSocket,reinterpret_cast<SOCKADDR*>(&serverAddr),sizeof(serverAddr)) == SOCKET_ERROR)
	{
		std::cerr << "bind() Error: " << WSAGetLastError() << '\n';

		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	// 연결 요청 큐를 활성화 해 서버 소켓을 수신 대기 상태로 전환
	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		std::cerr << "listen() Error: "	<< WSAGetLastError() << '\n';
		closesocket(serverSocket);
		WSACleanup();

		return 1;
	}

	// 서버 준비가 끝난 뒤 종료 핸들러를 등록해 Ctrl+C를 정상 종료 절차로 연결
	if (!SetConsoleCtrlHandler(HandleConsoleControl,TRUE))
	{
		std::cerr << "SetConsoleCtrlHandler() Error: " << GetLastError() << '\n';
		closesocket(serverSocket);
		WSACleanup();

		return 1;
	}

	// 모든 Client Worker가 동일한 방 목록을 공유하도록 설정
	auto roomRegistry = std::make_shared<RoomRegistry>();
	ClientWorkerManager workerManager;

	std::cout << "Server Started!" << std::endl;
    while (!stopRequested.load(std::memory_order_relaxed))
    {
		// accept에서 무기한 대기하지 않도록 select로 연결 요청
        fd_set readSet;

        FD_ZERO(&readSet);
        FD_SET(serverSocket, &readSet);

        TIMEVAL timeout{};
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        const int selectResult = select(0,&readSet,nullptr,nullptr,&timeout);

        if (selectResult == SOCKET_ERROR)
        {
            std::cerr << "select() Error: " << WSAGetLastError() << '\n';
            break;
        }

        // 1초 동안 연결 요청이 없었던 경우
        if (selectResult == 0)
        {
            continue;
        }

		// 연결 요청과 종료 요청이 거의 동시에 발생했다면 새 클라이언트를 받지 않는다
        if (stopRequested.load(std::memory_order_relaxed))
        {
            break;
        }

        if (!FD_ISSET(serverSocket, &readSet))
        {
            continue;
        }

        SOCKET clientSocket = INVALID_SOCKET;

        SOCKADDR_IN clientAddr{};
        int clientAddrSize = sizeof(clientAddr);
		
		// 대기 중인 연결 하나를 수락하고, 클라이언트 전용 소켓을 얻음
        clientSocket = accept(serverSocket,reinterpret_cast<SOCKADDR*>(&clientAddr),&clientAddrSize);

        if (clientSocket == INVALID_SOCKET)
        {
            if (stopRequested.load(std::memory_order_relaxed))
            {
                break;
            }

            std::cerr << "accept() Error: " << WSAGetLastError() << '\n';
            continue;
        }

        std::shared_ptr<ClientConnection> connection;

		// 연결 객체 생성 중 예외 발생시 아직 소유권이 이전되지 않은 소켓을 직접 닫음
        try
        {
            connection = std::make_shared<ClientConnection>(clientSocket);
        }
        catch (const std::exception& exception)
        {
            std::cerr << "[System] Failed to create client connection. Socket: " << clientSocket << ", Error: " << exception.what() << '\n';
            closesocket(clientSocket);
            continue;
        }
        catch (...)
        {
            std::cerr << "[System] Unknown client connection error. Socket: " << clientSocket << '\n';
            closesocket(clientSocket);
            continue;
        }

		// 연결별 작업 스레드 시작.
        if (!workerManager.Start(connection,roomRegistry))
        {
            std::cerr << "[System] Failed to start client worker. Socket: " << clientSocket << '\n';
            connection->Stop();
            continue;
        }
    }
	std::cout << "[System] Server shutdown requested.\n";

	closesocket(serverSocket);

	// 활성 연결을 shutdown하고 worker를 모두 join
	workerManager.StopAll();

	SetConsoleCtrlHandler(HandleConsoleControl,FALSE);

	WSACleanup();

	return 0;
}