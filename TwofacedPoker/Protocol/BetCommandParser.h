#pragma once

#include "../Common/Foundation.h"

#include <optional>
#include <string_view>

struct ParsedBetCommand
{
    BetType type;
    int count;
};

class BetCommandParser
{
public:
    static std::optional<ParsedBetCommand> parse(
        std::string_view message
    );
};