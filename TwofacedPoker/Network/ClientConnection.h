// ClientConnection.h
#pragma once

#include <WinSock2.h>

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

class ClientConnection
{
public:
    explicit ClientConnection(SOCKET socket);
    ~ClientConnection();

    ClientConnection(const ClientConnection&) = delete;
    ClientConnection& operator=(const ClientConnection&) = delete;

    SOCKET GetSocket() const noexcept;

    // 네트워크 송신이 아니라 큐 삽입만 수행
    bool Enqueue(std::string message);

    void Stop();

private:
    void SenderLoop();

    static constexpr std::size_t MAX_QUEUE_SIZE = 256;

    SOCKET socket;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::deque<std::string> sendQueue;

    bool stopping = false;
    bool stopped = false;

    std::thread senderThread;
};