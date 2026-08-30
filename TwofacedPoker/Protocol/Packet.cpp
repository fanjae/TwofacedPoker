#include "Packet.h"

#include <iostream>
#include <mutex>

namespace
{
    // 지정된 크기만큼 데이터 수신
    // Length-Prefix 형태로 반복 수신을 하도록 처리
    bool RecvAll(SOCKET socket, char* buffer, int size)
    {
        int received = 0;

        while (received < size)
        {
            // 아직 수신되지 않은 남은 크기 만큼 수신
            const int result = recv(socket, buffer + received, size - received, 0);

            if (result == 0)
            {
                // 상대가 정상적으로 연결 종료
                return false;
            }

            if (result == SOCKET_ERROR)
            {
                std::cerr << "[Network] recv failed. Error: " << WSAGetLastError() << '\n';
                return false;
            }

            received += result;
        }

        return true;
    }

    // 지정된 크기만큼 데이터 모두 전송
    // Length-Prefix 형태로 반복 전송을 하도록 처리
    bool SendAll(SOCKET socket, const char* buffer, int size)
    {
        int sent = 0;

        while (sent < size)
        {
            // 아직 수신되지 않은 크기 만큼 전송
            const int result = send(socket, buffer + sent, size - sent, 0);

            if (result <= 0)
            {
                if (result == SOCKET_ERROR)
                {
                    std::cerr << "[Network] send failed. Error: " << WSAGetLastError() << '\n';
                }
                return false;
            }
            sent += result;
        }

        return true;
    }
}

// 수신 성공시 본문 문자열 반환
// 연결 오류의 경우 std::nullopt 반환
std::optional<std::string> ReceivePacket(SOCKET socket)
{
    std::uint32_t networkLength = 0;

    // 고정 크기 4바이트 길이 헤더 수신
    if (!RecvAll(socket, reinterpret_cast<char*>(&networkLength), static_cast<int>(sizeof(networkLength))))
    {
        return std::nullopt;
    }

    // 네트워크 바이트에서 호스트 바이트 순서로 변환
    const std::uint32_t bodyLength = ntohl(networkLength);

    // 빈 패킷과 비정상적 패킷 거부
    if (bodyLength == 0 || bodyLength > MAX_PACKET_SIZE)
    {
        std::cerr << "[Network] Invalid packet size: " << bodyLength << '\n';
        return std::nullopt;
    }

    // 수신할 본문 크기만큼 문자열 버퍼 확보
    std::string message(bodyLength, '\0');

    // 헤더에 기록된 길이만큼 본문 데이터 수신
    if (!RecvAll(socket, message.data(), static_cast<int>(bodyLength)))
    {
        return std::nullopt;
    }

    return message;
}

// 문자열을 길이 헤더와 본문으로 나누어 전송
bool SendPacket(SOCKET socket, std::string_view message)
{
    // 빈 패킷과 비정상적 패킷 거부
    if (message.empty() || message.size() > MAX_PACKET_SIZE)
    {
        std::cerr << "[Network] Invalid outgoing packet size: " << message.size() << '\n';
        return false;
    }

    const auto bodyLength = static_cast<std::uint32_t>(message.size());

    // 본문 길이를 네트워크 바이트 순서로 변환
    const std::uint32_t networkLength = htonl(bodyLength);

    // 4길이 헤더 전송
    if (!SendAll(socket, reinterpret_cast<const char*>(&networkLength), static_cast<int>(sizeof(networkLength))))
    {
        return false;
    }

    // 길이 헤더 전송에 성공시 실제 본문 데이터 전송
    return SendAll(socket, message.data(), static_cast<int>(message.size()));
}