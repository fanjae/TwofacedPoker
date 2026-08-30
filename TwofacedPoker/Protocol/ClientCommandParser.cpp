#include "ClientCommandParser.h"
#include "../Common/Constants.h"

namespace
{
    // 명령어 뒤에 추가 데이터가 붙는 메시지를 접두사 기준으로 판별
    bool startsWith(std::string_view message,const std::string& prefix)
    {
        return message.size() >= prefix.size() && message.compare(0,prefix.size(),prefix) == 0;
    }

    // 명령어 접두사를 제거하고 실제 처리 대상 데이터만 복사해 반환.
    std::string extractPayload(std::string_view message,const std::string& prefix)
    {
        return std::string(message.substr(prefix.size()));
    }
}

ParsedClientCommand ClientCommandParser::parse(std::string_view message)
{
    // 추가 데이터가 없는 고정 명령은 전체가 정확히 일치할 때만 인정한다.
    if (message == GET_CHATTING_ROOM)
    {
        return
        {
            ClientCommandType::GET_ROOMS,
            {}
        };
    }

    if (message == EXIT_ROOM)
    {
        return
        {
            ClientCommandType::EXIT_ROOM,
            {}
        };
    }

    if (message == LOGIN)
    {
        return
        {
            ClientCommandType::LOGIN,
            {}
        };
    }

    if (message == USER_UPDATE)
    {
        return
        {
            ClientCommandType::USER_UPDATE,
            {}
        };
    }

    if (message == CLOSE_SOCKET)
    {
        return
        {
            ClientCommandType::CLOSE_SOCKET,
            {}
        };
    }

    if (startsWith(message, CREATE_CHATTING_ROOM))
    {
        return
        {
            ClientCommandType::CREATE_ROOM,
            extractPayload(message,CREATE_CHATTING_ROOM)
        };
    }

    if (startsWith(message, JOIN_CHATTING_ROOM))
    {
        return
        {
            ClientCommandType::JOIN_ROOM,
            extractPayload(message,JOIN_CHATTING_ROOM)
        };
    }

    if (startsWith(message, ROOM_EVENT))
    {
        return
        {
            ClientCommandType::ROOM_EVENT,
            extractPayload(message,ROOM_EVENT)
        };
    }

    if (startsWith(message, GAME_CLIENT_EVENT))
    {
        return
        {
            ClientCommandType::GAME_EVENT,
            extractPayload(message,GAME_CLIENT_EVENT)
        };
    }

    // 알려진 명령이 아니면 기존 동작처럼 채팅으로 처리
    return
    {
        ClientCommandType::CHAT,
        std::string(message)
    };
}