#include <charconv>
#include <system_error>

#include "BetCommandParser.h"
#include "../Common/Constants.h"
#include "../Common/Foundation.h"

namespace
{
    // 메시지의 앞부분이 특정 베팅 명령어와 일치한지 확인
    // 명령어 뒤에 칩 개수가 붙는 FRONT/BOTH/BACK 판별에 사용
    bool startsWith(std::string_view message, const std::string& prefix)
    {
        return message.size() >= prefix.size() && message.compare(0,prefix.size(),prefix) == 0;
    }

    // 명령어 뒤에 붙은 문자열 전체를 정수로 변환
    // 비어 있거나 숫자가 아닌 문자가 섞여 있으면 파싱 실패로 처리
    std::optional<int> parseCount(std::string_view text)
    {
        if (text.empty())
        {
            return std::nullopt;
        }

        int count = 0;

        const char* begin = text.data();
        const char* end = begin + text.size();

        // 프로토콜 입력 검증을 위해 from_chars를 사용.
        const auto [parsedEnd, error] = std::from_chars(begin,end,count);

        // 일부만 숫자인 경우도 허용하지 않음
        if (error != std::errc{} || parsedEnd != end)
        {
            return std::nullopt;
        }

        return count;
    }

    // 칩 개수가 필요한 베팅 명령의 공통 파싱 로직
    // 성공하면 명령 종류와 칩 개수를 묶어 반환, 형식 틀리면 std::nullopt 반환.
    std::optional<ParsedBetCommand> parseChipBet(std::string_view message,const std::string& prefix,BetType type)
    {
        if (!startsWith(message, prefix))
        {
            return std::nullopt;
        }

        // 실제 베팅 칩 개수 처리.
        const std::string_view countText = message.substr(prefix.size());
        const auto count = parseCount(countText);

        if (!count)
        {
            return std::nullopt;
        }

        return ParsedBetCommand
        {
            type,
            *count
        };
    }
}

// 베팅 명령어 파싱
std::optional<ParsedBetCommand> BetCommandParser::parse(std::string_view message)
{
    // DIE와 SPECIAL 처리
    if (message == DIE)
    {
        return ParsedBetCommand
        {
            BetType::DIE,
            0
        };
    }

    if (message == SPECIAL)
    {
        return ParsedBetCommand
        {
            BetType::SPECIAL,
            0
        };
    }

    // 칩 베팅 명령은 접두사 뒤의 숫자까지 해석해야 하므로 공통 함수 사용
    if (startsWith(message, FRONT))
    {
        return parseChipBet(message,FRONT,BetType::FRONT);
    }

    if (startsWith(message, BOTH))
    {
        return parseChipBet(message,BOTH,BetType::BOTH);
    }

    if (startsWith(message, BACK))
    {
        return parseChipBet(message,BACK, BetType::BACK);
    }

    return std::nullopt;
}