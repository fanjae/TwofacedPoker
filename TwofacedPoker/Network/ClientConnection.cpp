// ClientConnection.cpp
#include "ClientConnection.h"
#include "../Protocol/Packet.h"

#include <utility>
#include <iostream>

// 클라이언트 소켓 보관, senderThread가 senderLoop() 실행, eqneue()로 들어온 메시지 하나씩 꺼냄.
ClientConnection::ClientConnection(SOCKET socket) : socket(socket)
{
    senderThread = std::thread(&ClientConnection::SenderLoop, this);
}

ClientConnection::~ClientConnection()
{
    Stop();
}

SOCKET ClientConnection::GetSocket() const noexcept
{
    return socket;
}

// 송신 메시지 큐에 추가.
bool ClientConnection::Enqueue(std::string message)
{
    // 서버 내부에서 잘못 생성된 메시지가 senderLoop()까지 전달되면
    // SendPacket() 실패가 정상 연결 종료로 이어질 수 있으므로 큐에 넣기 전에 거부.
    if (message.empty() || message.size() > MAX_PACKET_SIZE)
    {
        std::cerr << "[Network] Invalid outgoing packet size: " << message.size()
            << ". Socket: " << socket << '\n';
        return false;
    }

    bool queueOverflow = false;

    {
        // sendQueue에 여러 스레드가 접근할 수 있으므로, queueMutex 보호.
        std::lock_guard<std::mutex> lock(queueMutex);

        if (stopping)
        {
            return false;
        }

        // 큐가 일정이상 넘어가면 해당 연결을 종료 대상으로 처리
        if (sendQueue.size() >= MAX_QUEUE_SIZE)
        {
            stopping = true;
            queueOverflow = true;
        }
        else
        {
            // 문자열 복사하지 않고, 큐 내부로 이동.
            sendQueue.push_back(std::move(message));
        }
    } // queueMutex Lock 해제

    if (queueOverflow)
    {
        std::cerr << "[Network] Send queue overflow. Socket: " << socket << '\n';

        // 송신 큐 가득찬 클라이언트는 송수신 모두 중단.
        shutdown(socket, SD_BOTH);
        queueCondition.notify_all();
        return false;
    }

    // 새로운 메시지가 큐에 들어온 경우 대기 중인 송신 스레드 깨움
    queueCondition.notify_one();
    return true;
}

// 송신 스레드에서 실행 되는 함수.
// 큐에 메시지 대기 후, SendPacket()으로 전송
void ClientConnection::SenderLoop()
{
    while (true)
    {
        std::string message;

        {
            // condition_variable::wait()에 대한 Unique_lock 처리
            std::unique_lock<std::mutex> lock(queueMutex);

            queueCondition.wait(lock, [this]
                {
                    return stopping || !sendQueue.empty();
                });

            if (stopping)
            {
                break;
            }

            // 가장 먼저 들어온 메시지 꺼냄.
            // 문자열은 복사 대신 이동 처리.
            message = std::move(sendQueue.front());
            sendQueue.pop_front();

        } // queueMutex Lock 해제

        // queueMutex를 보유하지 않은 상태에서 블로킹 송신
        if (!SendPacket(socket, message))
        {
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                stopping = true;
            }

            // 수신 중인 스레드도 깨워 연결 정리를 유도
            shutdown(socket, SD_BOTH);
            break;
        }
    }
}

// 연결 종료 및 자원 정리
void ClientConnection::Stop()
{
    {
        std::lock_guard<std::mutex> lock(queueMutex);

        if (stopped)
        {
            return;
        }

        stopping = true; // senderLoop()와 enqueue()에 종료 상태 전달
        stopped = true;
    }

    // 블로킹 중인 recv/send를 종료
    shutdown(socket, SD_BOTH);
    queueCondition.notify_all();

    if (senderThread.joinable())
    {
        senderThread.join();
    }

    closesocket(socket);
    socket = INVALID_SOCKET;
}
