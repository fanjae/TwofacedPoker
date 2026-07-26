#include "GameManager.h"
#include "../Common/Foundation.h"
#include "../Common/Constants.h"
#include "../Protocol/BetCommandParser.h"
#include "../Network/ClientHandle.h"
#include "../Room/RoomManager.h"

#include <iostream>
#include <utility>



GameManager::GameManager(std::shared_ptr<RoomManager> roomManager) : roomManager(std::move(roomManager)), gameSession()
{
}

// 락이 걸리지 않은 상태에서 게임 세션 초기화
void GameManager::resetGameUnlocked()
{
	gameSession.reset();
}

// 뮤텍스 락을 걸고 게임 세션 초기화 
void GameManager::resetGame()
{
	std::lock_guard<std::mutex> lock(gameMutex);
	resetGameUnlocked();
}

// 플레이거 퇴장할 때의 처리
bool GameManager::handlePlayerExit(int userNumber,const std::string& userID,SOCKET socket)
{
	std::lock_guard<std::mutex> lock(gameMutex);

	const bool gameWasRunning = gameSession.isBetting();

	const bool removed = roomManager->removeUser(userNumber,userID,socket,gameWasRunning);

	if (!removed)
	{
		return false;
	}

	// 유저가 나갔으므로 게임 초기화
	resetGameUnlocked();
	return true;
}

// 턴을 상대방에게 넘김
bool GameManager::passTurnToOpponent(SOCKET currentSocket)
{
	const auto opponentSocket = roomManager->getOpponentSocket(currentSocket);

	if (!opponentSocket)
	{
		resetGameUnlocked();
		return false;
	}

	if (!gameSession.passTurnTo(*opponentSocket))
	{
		resetGameUnlocked();
		return false;
	}

	return true;
}

// 최종 게임 종료 처리 및 결과 브로드 캐스트
void GameManager::finishGame(SOCKET socket, MatchResult finalResult)
{
	std::string selfMessage;
	std::string opponentMessage;

	if (finalResult == MatchResult::FINAL_WIN)
	{
		selfMessage = GAME_CLIENT_EVENT + GAME_RESULT + FINALWIN;

		opponentMessage = GAME_CLIENT_EVENT + GAME_RESULT + FINALLOSE;
	}
	else if (finalResult == MatchResult::FINAL_LOSE)
	{
		selfMessage = GAME_CLIENT_EVENT + GAME_RESULT + FINALLOSE;

		opponentMessage = GAME_CLIENT_EVENT + GAME_RESULT + FINALWIN;
	}
	else
	{
		std::cerr << "[Game] Invalid final result: " << static_cast<int>(finalResult) << '\n';
		return;
	}

	// 양측 클라이언트에 결과 전송
	roomManager->broadcast_Message(selfMessage,socket,TargetType::SELF);
	roomManager->broadcast_Message(opponentMessage,socket,TargetType::OTHERS);

	resetGameUnlocked();
}

// 클라이언트로부터 온 게임 관련 메시지 이벤트 처리
bool GameManager::Handle_Game_Event(const SOCKET socket,const std::string& message)
{
	std::lock_guard<std::mutex> lock(gameMutex);

	// 게임 시작 요청인 경우
	if (message == GAME_START)
	{
		if (!gameSession.isWaiting())
		{
			std::cerr << "[Game] Game is already playing. Socket: " << socket << '\n';
			return false;
		}

		Handle_Game_Start(socket);
		return true;
	}

	// 베팅 메시지인지 확인
	const bool isBettingMessage = message.compare(0,BETTING.size(),BETTING) == 0;

	if (isBettingMessage)
	{
		if (!gameSession.isBetting())
		{
			std::cerr << "[Game] Betting requested before game start. Socket: "	<< socket << '\n';
			return false;
		}

		// 현재 턴이 아닌데 베팅을 시도한 경우 차단
		if (!gameSession.isCurrentTurn(socket))
		{
			std::cerr << "[Game] Betting requested out of turn. Socket: " << socket << '\n';

			const std::string sendMessage =	GAME_CLIENT_EVENT + BETTING	+ IMPOSSIBLE;
			roomManager->broadcast_Message(sendMessage,socket,TargetType::SELF);

			return false;
		}

		// 베팅 로직 수행
		betUser(socket,message.substr(BETTING.length()));
		return true;
	}

	std::cerr << "[Game] Unknown game message: "<< message << '\n';
	return false;
}

// 게임 시작 로직
void GameManager::Handle_Game_Start(const SOCKET socket)
{
	std::cout << "[System] Game start event.\n";

	// 두 명 모두 준비되지 않았다면 READY 메시지 전송
	if (!roomManager->All_User_Start_Ready_State())
	{
		roomManager->broadcast_Message(GAME_CLIENT_EVENT + START + READY,socket,TargetType::ALL);
		return;
	}

	if (!gameSession.start(socket))
	{
		std::cerr << "[Game] Failed to start game. Socket: " << socket << '\n';
		return;
	}
	roomManager->broadcast_Message(GAME_CLIENT_EVENT + START + DONE,socket,TargetType::ALL);

	// 유저 정보 초기화 및 턴 메시지 전송
	roomManager->resetAllUsers(InitType::INIT);
	roomManager->broadcast_Message(GAME_CLIENT_EVENT + GAME_INIT,socket,TargetType::ALL);
	roomManager->broadcast_Message(GAME_CLIENT_EVENT + TURN + MY,socket,TargetType::SELF);
	roomManager->broadcast_Message(GAME_CLIENT_EVENT + TURN + OTHER,socket,TargetType::OTHERS);

	giveBasicBetting(socket);
}

// 라운드 시작 전 기본 베팅 수거
void GameManager::giveBasicBetting(SOCKET socket)
{
	const std::string message = GAME_CLIENT_EVENT + BASIC_BETTING;

	roomManager->broadcast_Message(message,socket,TargetType::ALL);

	// 두 플레이어에게서 1칩씩 차감하고, dealerChips에 총 2칩을 추가한다.
	roomManager->collectBasicBet(socket, 1);
	giveCards(socket);
}

// 플레이어들에게 카드 분배
void GameManager::giveCards(SOCKET socket)
{
	roomManager->updateCards(socket,gameSession.dealCards());
}

// 유저의 베팅 명령 파싱 및 수행
void GameManager::betUser(const SOCKET socket,const std::string& message)
{
	const auto command = BetCommandParser::parse(message);

	if (!command)
	{
		std::cerr << "[Game] Invalid betting message: " << message << '\n';
		return;
	}

	std::cout << "[Game] Bet type: " << static_cast<int>(command->type) << ", count: " << command->count << '\n';
	betChip(socket,command->count,command->type);
}

// 파싱된 데이터를 바탕으로 실제 베팅 처리
void GameManager::betChip(SOCKET socket,int betCount,BetType betType)
{
	const BetResult betResult =	resolveBet(socket, betCount, betType);

	switch (betResult)
	{
		case BetResult::IMPOSSIBLE: // 불가능한 베팅 
			return;

		case BetResult::RAISE: // 판돈을 더 올린 경우 턴 변경
			if (passTurnToOpponent(socket))
			{
				notifyTurnChanged(socket);
			}
			return;

		case BetResult::CALL: // 판돈을 맞췄다면 라운드 종료
			processRoundEnd(socket, betType);
			return;

		default:
			std::cerr << "[Game] Unexpected bet result: " << static_cast<int>(betResult) << '\n';
		return;
	}
}

// DIE나 특수 케이스, 일반 칩 베팅을 분기하여 검증
BetResult GameManager::resolveBet(SOCKET socket,int betCount,BetType betType)
{
	if (betType == BetType::DIE)
	{
		return BetResult::CALL;
	}

	if (betType == BetType::SPECIAL)
	{
		roomManager->specialCase();

		return BetResult::CALL;
	}

	return roomManager->betChips(socket,betCount,betType);
}

// 콜 또는 다이 발생 시 라운드 결과 처리
void GameManager::processRoundEnd(SOCKET socket,BetType actionType)
{
	if (actionType == BetType::DIE)
	{
		// 한 명이 기권했을 경우
		const RoundResult roundResult = RoundResult::LOSE;

		broadcastRoundResult(socket, roundResult, actionType);
		roomManager->settleRound(socket, roundResult); // 정산

		MatchResult matchResult = roomManager->endCheck(socket); // 게임 종료

		if (finishIfNeeded(socket, matchResult))
		{
			return;
		}

		if (matchResult != MatchResult::PROGRESS)
		{
			std::cerr << "[Game] Unexpected match result: " << static_cast<int>(matchResult) << '\n';
			return;
		}

		prepareNextRound(socket, roundResult);
		return;
	}

	// 정상적인 배틀(CALL) 시 카드를 공개하고 결과 판정
	roomManager->broadcast_Message(GAME_CLIENT_EVENT + BATTLE, socket, TargetType::ALL);
	roomManager->printCard(socket);

	const RoundResult roundResult =	resolveRound(socket, actionType);

	if (roundResult == RoundResult::IMPOSSIBLE)
	{
		return;
	}

	roomManager->settleRound(socket,roundResult);

	MatchResult matchResult = roomManager->endCheck(socket);

	// 무승부에서는 공동 판돈이 이월되므로, 게임 종료로 처리하지 않음.
	if (roundResult == RoundResult::DRAW)
	{
		matchResult = MatchResult::PROGRESS;
	}

	if (finishIfNeeded(socket, matchResult))
	{
		return;
	}

	if (matchResult != MatchResult::PROGRESS)
	{
		std::cerr << "[Game] Unexpected match result: "	<< static_cast<int>(matchResult) << '\n';

		return;
	}

	prepareNextRound(socket,roundResult);
}

// 양측 카드를 비교하여 승무패 결정
RoundResult GameManager::resolveRound(SOCKET socket,BetType actionType)
{
	const RoundResult roundResult =	roomManager->compareCard(socket,actionType);

	if (roundResult == RoundResult::IMPOSSIBLE)
	{
		std::cerr << "[Game] Card comparison failed.\n";
		return RoundResult::IMPOSSIBLE;
	}
	broadcastRoundResult(socket,roundResult,actionType);

	return roundResult;
}

// 클라이언트에 라운드 결과를 브로드 캐스팅
void GameManager::broadcastRoundResult(SOCKET socket,RoundResult roundResult,BetType actionType)
{
	std::string selfResult;
	std::string opponentResult;

	switch (roundResult)
	{
	case RoundResult::WIN:
		selfResult = WIN;
		opponentResult = LOSE;
		break;

	case RoundResult::LOSE:
		selfResult = LOSE;

		opponentResult = actionType == BetType::DIE	? DIE : WIN;

		break;

	case RoundResult::DRAW:
		roomManager->broadcast_Message(GAME_CLIENT_EVENT + GAME_RESULT + DRAW,socket,TargetType::ALL);
		return;

	case RoundResult::BOTH_WIN:
		selfResult = BOTHWIN;
		opponentResult = BOTHLOSE;
		break;

	case RoundResult::BOTH_LOSE:
		selfResult = BOTHLOSE;
		opponentResult = BOTHWIN;
		break;

	case RoundResult::IMPOSSIBLE:
		std::cerr << "[Game] Cannot broadcast impossible round result.\n";

		return;

	default:
		std::cerr << "[Game] Unknown round result: " << static_cast<int>(roundResult) << '\n';
		return;
	}

	roomManager->broadcast_Message(GAME_CLIENT_EVENT + GAME_RESULT + selfResult, socket,TargetType::SELF);
	roomManager->broadcast_Message(GAME_CLIENT_EVENT + GAME_RESULT + opponentResult, socket, TargetType::OTHERS);
}

// 다음 라운드를 위한 초기화 및 준비 과정
void GameManager::prepareNextRound(SOCKET socket,RoundResult roundResult)
{
	if (roundResult == RoundResult::DRAW)
	{
		// 무승부는 dealerChips를 유지하고, 카드만 다시 지급한다.
		giveCards(socket);
	}
	else
	{
		// 승패가 결정되면 새 기본 베팅을 수집한다.
		giveBasicBetting(socket);
	}

	// 파산 등으로 게임이 종료되어야 하는지 체크
	if (finishIfNeeded(socket,roomManager->endCheck(socket)))
	{
		return;
	}

	// 다음 라운드 턴 설정
	if (!passTurnToOpponent(socket))
	{
		return;
	}

	roomManager->broadcast_Message(GAME_CLIENT_EVENT + GAME_INIT,socket,TargetType::ALL);
	notifyTurnChanged(socket);
}

// 칩 소진 등 게임 종료 조건 달성 시 게임을 종료
bool GameManager::finishIfNeeded(SOCKET socket,MatchResult matchResult)
{
	if (matchResult != MatchResult::FINAL_WIN && matchResult != MatchResult::FINAL_LOSE)
	{
		return false;
	}

	finishGame(socket, matchResult);
	return true;
}

// 턴 변경 메시지 브로드캐스트
void GameManager::notifyTurnChanged(SOCKET previousTurnSocket)
{
	roomManager->broadcast_Message(GAME_CLIENT_EVENT + TURN + OTHER, previousTurnSocket,TargetType::SELF);
	roomManager->broadcast_Message(GAME_CLIENT_EVENT + TURN + MY, previousTurnSocket,TargetType::OTHERS);
}