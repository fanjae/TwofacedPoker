#pragma once

#include <string>
#include <string_view>

enum class ClientCommandType
{
    GET_ROOMS,
    EXIT_ROOM,
    LOGIN,
    USER_UPDATE,
    CREATE_ROOM,
    JOIN_ROOM,
    CLOSE_SOCKET,
    ROOM_EVENT,
    GAME_EVENT,
    CHAT
};

struct ParsedClientCommand
{
    ClientCommandType type;
    std::string payload;
};

class ClientCommandParser
{
public:
    static ParsedClientCommand parse(
        std::string_view message
    );
};