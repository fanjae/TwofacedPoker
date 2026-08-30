#include "RoomManager.h"
#include "../Protocol/Packet.h"
#include "../Common/Foundation.h"
#include "../Common/Constants.h"
#include "../Game/RoundResolver.h"
#include "../Game/BettingRules.h"
#include "../Game/SettlementRules.h"
#include "../Network/ClientConnection.h"

#include <vector>
#include <algorithm>
#include <iostream>

// RoomManger는 한 방에 속한 사용자, 소켓 연결, 베팅 및 카드 상태를 함께 관리.
// 공유 컨테이너는 게임 상태는 roomMutex로 보호, 실제 네트워크 송신은 락 밖에서 수행.
RoomManager::RoomManager(const std::string& roomName) : roomName(roomName), dealerChips(0)
{
}

// 방 단위 프로토콜 메시지를 해석해 해당 처리 함수로 전달 한다.
bool RoomManager::Handle_Room_Event(const SOCKET& socket, const std::string& message)
{
	std::cout << message << std::endl;

	// 준비 상태 명령은 접두사를 제거한 페이로드만 준비 처리 함수
	if (message.substr(0, USER_READY_STATE.length()) == USER_READY_STATE)
	{
		Handle_User_Ready(socket, message.substr(USER_READY_STATE.length()));
	}
	return true;
}

std::shared_ptr<RoomManager> RoomManager::createRoom(const std::string& roomName)
{
	return std::make_shared<RoomManager>(roomName);
}

// 내부에서 다시 락을 잡지 않아 동일 mutex의 중복 잠금으로 인한 교착을 피한다.
std::optional<SOCKET> RoomManager::findOpponentSocketUnlocked(SOCKET socket) const
{
	// 요청한 소켓이 현재 방 구성원이 아니면 상대를 검색하지 않음
	if (connections.find(socket) == connections.end())
	{
		return std::nullopt;
	}

	// 최대 2인 방이므로 자신이 아닌 유효한 연결이 곧 상대방
	for (const auto& [targetSocket, connection] : connections)
	{
		if (targetSocket != socket && connection)
		{
			return targetSocket;
		}
	}
	return std::nullopt;
}
std::shared_ptr<User> RoomManager::findUserBySocketUnlocked(SOCKET socket) const
{
	// 소켓-사용자 번호 인덱스를 거쳐 실제 User 객체를 찾음
	const auto socketIt = socketUserNumber.find(socket);

	if (socketIt == socketUserNumber.end())
	{
		return nullptr;
	}
	const auto userIt =	users.find(socketIt->second);

	if (userIt == users.end() || !userIt->second)
	{
		return nullptr;
	}

	return userIt->second;
}
std::shared_ptr<User> RoomManager::findOpponentUserUnlocked(SOCKET socket) const
{
	// 두 사용자가 모두 입장한 경우에만 성립
	if (users.size() != 2)
	{
		return nullptr;
	}

	const auto currentUser = findUserBySocketUnlocked(socket);

	if (!currentUser)
	{
		return nullptr;
	}

	// 현재 사용자와 다른 User 객체를 상대방으로 선택
	for (const auto& [userNumber, user] : users)
	{
		if (user && user != currentUser)
		{
			return user;
		}
	}

	return nullptr;
}
void RoomManager::broadcast_Message(const std::string& message,SOCKET senderSocket,TargetType targetType)
{
	std::vector<PendingSend> pendingSends;

	{
		std::lock_guard<std::mutex> lock(roomMutex);

		// 락 안에서는 수신 대상과 연결 포인터만 수집.
		pendingSends.reserve(connections.size());

		collectBroadcastUnlocked(pendingSends,message,senderSocket,targetType);
	}

	// roomMutex 해제 후 송신
	sendPending(pendingSends);
}
bool RoomManager::addUser(int userNumber,const std::string& userID,std::shared_ptr<ClientConnection> connection)
{
	// 유효한 연결과 소켓만 방 구성원으로 받음
	if (!connection)
	{
		return false;
	}

	const SOCKET socket = connection->GetSocket();

	if (socket == INVALID_SOCKET)
	{
		return false;
	}

	std::lock_guard<std::mutex> lock(roomMutex);

	constexpr std::size_t MAX_USERS = 2;

	// 게임 규칙상 방 정원은 두 명으로 제한한다.
	if (users.size() >= MAX_USERS)
	{
		return false;
	}

	// 사용자 번호, 소켓, 소켓 인덱스 중 어느 하나라도 중복되면 상태 불일치를 막기 위해 거부한다.
	if (users.find(userNumber) != users.end())
	{
		return false;
	}

	if (connections.find(socket) != connections.end())
	{
		return false;
	}

	if (socketUserNumber.find(socket) != socketUserNumber.end())
	{
		return false;
	}

	// 세 컨테이너는 같은 참가자를 서로 다른 키로 조회하기 위한 하나의 논리적 상태다.
	users.emplace(userNumber,std::make_shared<User>(userNumber, userID));
	socketUserNumber.emplace(socket, userNumber);
	connections.emplace(socket, std::move(connection));

	return true;
}
bool RoomManager::removeUser(int userNumber,const std::string& userID,SOCKET socket,bool gameWasRunning)
{
	// 락 안에서 송신 대상을 수집한 뒤 락 밖에서 전송하기 위한 임시 큐다.
	std::vector<PendingSend> pendingSends;
	std::string exitMessage;

	{
		std::lock_guard<std::mutex> lock(roomMutex);

		const auto userIt = users.find(userNumber);
		const auto socketIt = socketUserNumber.find(socket);
		const auto connectionIt = connections.find(socket);

		// 전달받은 사용자 번호와 소켓이 세 컨테이너에서 모두 같은 참가자를 가리키는지 검증한다.
		if (userIt == users.end() || !userIt->second ||	socketIt == socketUserNumber.end() || socketIt->second != userNumber ||	connectionIt == connections.end() || !connectionIt->second)
		{
			return false;
		}

		exitMessage = userIt->second->getID() + " has exited.";

		// 정상적인 방 나가기 요청에 대한 응답
		collectBroadcastUnlocked(pendingSends,EXIT_ROOM_COMPLETE,socket,TargetType::SELF);

		// 상대방에게 기존 퇴장 메시지 전송
		collectBroadcastUnlocked(pendingSends,exitMessage,socket,TargetType::OTHERS);

		users.erase(userIt);
		socketUserNumber.erase(socketIt);
		connections.erase(connectionIt);

		// 방 구성원이 바뀌었으므로 기존 대전 상태 폐기
		dealerChips = 0;

		for (auto& [remainingUserNumber, remainingUser] : users)
		{
			if (!remainingUser)
			{
				continue;
			}

			remainingUser->setisReady(false);
			remainingUser->setChips(DEFAULT_CHIPS);

			remainingUser->setFrontBet(0);
			remainingUser->setBackBet(0);

			remainingUser->setFrontCard(0);
			remainingUser->setBackCard(0);

			remainingUser->setBetType(BetType::NONE);
		}

		// 퇴장 이후 남은 사용자에게 전달 
		if (gameWasRunning)
		{
			collectBroadcastUnlocked(pendingSends,GAME_CLIENT_EVENT + GAME_ABORTED,	socket,	TargetType::ALL);
		}
	}

	std::cout << "[Room] " << exitMessage << '\n';

	// roomMutex 해제 후 송신
	sendPending(pendingSends);

	return true;
}
void RoomManager::userUpdate(const SOCKET clientSocket)
{
	// 새 참가자와 기존 참가자가 서로의 ID 및 준비 상태를 동기화할 메시지 모음
	std::vector<PendingSend> pendingSends;

	{
		std::lock_guard<std::mutex> lock(roomMutex);

		const auto currentUser = findUserBySocketUnlocked(clientSocket);
		const auto opponentUser = findOpponentUserUnlocked(clientSocket);

		if (!currentUser || !opponentUser)
		{
			return;
		}

		// 상대방에게 현재 사용자의 ID 전송
		collectBroadcastUnlocked(pendingSends,ROOM_CLIENT_EVENT + UPDATE_ID + currentUser->getID(), clientSocket, TargetType::OTHERS);

		// 상대방에게 현재 사용자의 준비 상태 전송
		collectBroadcastUnlocked(pendingSends,ROOM_CLIENT_EVENT	+ UPDATE_READY_STATE + (currentUser->getisReady() ? DONE : READY), clientSocket, TargetType::OTHERS);

		// 현재 사용자에게 상대방의 ID 전송
		collectBroadcastUnlocked(pendingSends,ROOM_CLIENT_EVENT	+ UPDATE_ID	+ opponentUser->getID(), clientSocket, TargetType::SELF);

		// 현재 사용자에게 상대방의 준비 상태 전송
		collectBroadcastUnlocked(pendingSends, ROOM_CLIENT_EVENT + UPDATE_READY_STATE + (opponentUser->getisReady()	? DONE : READY), clientSocket, TargetType::SELF);
	}

	// roomMutex가 해제된 이후 송신
	sendPending(pendingSends);
}
void RoomManager::Handle_User_Ready(const SOCKET clientSocket,const std::string& message)
{
	bool readyState = false;
	std::string stateMessage;

	// 공유 상태를 사용하지 않는 입력 검증
	if (message == DONE)
	{
		readyState = true;
		stateMessage = DONE;
	}
	else if (message == READY)
	{
		readyState = false;
		stateMessage = READY;
	}
	else
	{
		return;
	}

	std::vector<PendingSend> pendingSends;

	{
		std::lock_guard<std::mutex> lock(roomMutex);

		const auto currentUser = findUserBySocketUnlocked(clientSocket);
		const auto opponentUser = findOpponentUserUnlocked(clientSocket);

		if (!currentUser || !opponentUser)
		{
			return;
		}

		// 서버 상태를 먼저 갱신한 뒤 상대방에게 변경 결과를 알림
		currentUser->setisReady(readyState);

		collectBroadcastUnlocked(pendingSends,ROOM_CLIENT_EVENT	+ UPDATE_READY_STATE + stateMessage, clientSocket, TargetType::OTHERS);
	}

	// roomMutex가 해제된 이후 송신
	sendPending(pendingSends);
}

void RoomManager::resetAllUsers(InitType initType)
{
	std::lock_guard<std::mutex> lock(roomMutex);

	// 완전히 새로운 게임을 시작할 때만
	// 이전 게임의 이월 칩을 초기화한다.
	if (initType == InitType::INIT)
	{
		dealerChips = 0;
	}

	for (auto& [userNumber, user] : users)
	{
		if (!user)
		{
			continue;
		}

		// 새 게임에서는 플레이어 칩도 초기값으로 변경
		if (initType == InitType::INIT)
		{
			user->setChips(DEFAULT_CHIPS);
		}

		// 새 게임과 다음 라운드에서 모두 초기화할 상태
		user->setFrontBet(0);
		user->setBackBet(0);

		user->setFrontCard(0);
		user->setBackCard(0);

		user->setBetType(BetType::NONE);
	}
}

void RoomManager::updateCards(SOCKET socket, const std::array<std::pair<int, int>, 2>& cardData)
{
	// 카드 상태 갱신과 각 사용자에게 보낼 비공개/공개 카드 메시지를 한 번에 구성
	std::vector<PendingSend> pendingSends;

	{
		std::lock_guard<std::mutex> lock(roomMutex);

		const auto currentUser = findUserBySocketUnlocked(socket);
		const auto opponentUser = findOpponentUserUnlocked(socket);
		const auto opponentSocket = findOpponentSocketUnlocked(socket);

		if (!currentUser || !opponentUser || !opponentSocket)
		{
			return;
		}

		const auto& currentCards = cardData[0];
		const auto& opponentCards =	cardData[1];

		// 서버 카드 상태 갱신
		currentUser->setFrontCard(currentCards.first);
		currentUser->setBackCard(currentCards.second);

		opponentUser->setFrontCard(opponentCards.first);
		opponentUser->setBackCard(opponentCards.second);

		// 각 플레이어는 자신의 양면 카드를 받고, 상대 카드에서는 공개된 앞면만 받음
		collectMyCardUpdateUnlocked(pendingSends,socket,currentCards);

		collectMyCardUpdateUnlocked(pendingSends,*opponentSocket,opponentCards);

		collectVisibleOpponentCardUnlocked(pendingSends,socket,opponentCards.first);

		collectVisibleOpponentCardUnlocked(pendingSends,*opponentSocket,currentCards.first);
	}

	// roomMutex가 해제된 이후 송신
	sendPending(pendingSends);
}
BetResult RoomManager::betChips(const SOCKET socket,int count,BetType betType)
{
	// 베팅 결과와 이에 따른 칩/베팅 UI 갱신 메시지를 같은 상태 스냅샷에서 결정
	std::vector<PendingSend> pendingSends;

	BetResult result = BetResult::IMPOSSIBLE;

	{
		std::lock_guard<std::mutex> lock(roomMutex);

		const auto currentUser = findUserBySocketUnlocked(socket);
		const auto opponentUser = findOpponentUserUnlocked(socket);

		if (!currentUser || !opponentUser)
		{
			return BetResult::IMPOSSIBLE;
		}

		// 규칙 계층에는 RoomManager 내부 객체 대신 판정에 필요한 값만 전달
		const BettingState currentState
		{
			currentUser->getChips(),
			currentUser->getFrontBet(),
			currentUser->getBackBet(),
			currentUser->getBetType()
		};

		const BettingState opponentState
		{
			opponentUser->getChips(),
			opponentUser->getFrontBet(),
			opponentUser->getBackBet(),
			opponentUser->getBetType()
		};

		// BettingRules가 베팅 가능 여부와 실제 차감 비용을 계산
		const BetDecision decision = BettingRules::evaluate(currentState,opponentState,count,betType);

		if (decision.result == BetResult::IMPOSSIBLE)
		{
			collectBroadcastUnlocked(pendingSends,GAME_CLIENT_EVENT + BETTING + IMPOSSIBLE,	socket,TargetType::SELF);
		}
		else if (!deductBetUnlocked(socket,decision.cost,pendingSends)) // 규칙 판정 통과해도 실제 칩 차감 실패시 베팅 반영 안하도록 처리
		{
			collectBroadcastUnlocked(pendingSends,GAME_CLIENT_EVENT	+ BETTING + IMPOSSIBLE,	socket,	TargetType::SELF);
		}
		else
		{
			updateBetInfoUnlocked(socket, count, betType, pendingSends);
			result = decision.result;
		}
	}

	// roomMutex 해제 후 송신
	sendPending(pendingSends);

	return result;
}
void RoomManager::updateBetInfoUnlocked(SOCKET socket,int count,BetType betType,std::vector<PendingSend>& pendingSends)
{
	// 호출자는 roomMutex를 보유해야 하며, 선택한 면별 누적 베팅액을 갱신
	const auto user = findUserBySocketUnlocked(socket);

	if (!user)
	{
		return;
	}

	user->setBetType(betType);

	if (betType == BetType::FRONT || betType == BetType::BOTH)
	{
		user->setFrontBet(user->getFrontBet() + count);
		collectBetUpdateUnlocked(pendingSends,socket,FRONT,user->getFrontBet());
	}

	if (betType == BetType::BACK || betType == BetType::BOTH)
	{
		user->setBackBet(user->getBackBet() + count);
		collectBetUpdateUnlocked(pendingSends,socket,BACK,user->getBackBet());
	}

	if (betType == BetType::BOTH)
	{
		// 양면 베팅 여부는 금액 갱신과 별도로 양쪽 클라이언트에 표시
		collectBroadcastUnlocked(pendingSends,GAME_CLIENT_EVENT	+ BOTH + MY, socket,TargetType::SELF);
		collectBroadcastUnlocked(pendingSends,	GAME_CLIENT_EVENT + BOTH + OTHER, socket, TargetType::OTHERS);
	}
}

RoundResult RoomManager::compareCard(const SOCKET socket,BetType actionType)
{
	// 두 사용자의 카드와 베팅 선택을 불변 값으로 구성해 라운드 판정 규칙에 전달
	std::lock_guard<std::mutex> lock(roomMutex);

	const auto currentUser = findUserBySocketUnlocked(socket);

	const auto opponentUser = findOpponentUserUnlocked(socket);

	if (!currentUser || !opponentUser)
	{
		return RoundResult::IMPOSSIBLE;
	}

	const PlayerRoundState current
	{
		currentUser->getFrontCard(),
		currentUser->getBackCard(),
		currentUser->getBetType()
	};

	const PlayerRoundState opponent
	{
		opponentUser->getFrontCard(),
		opponentUser->getBackCard(),
		opponentUser->getBetType()
	};

	// DIE 선택 여부까지 포함해 승패, 무승부 또는 판정 불가 상태를 계산
	const RoundResult result = RoundResolver::resolve(current,opponent,actionType == BetType::DIE);

	if (result != RoundResult::IMPOSSIBLE)
	{
		// 완료된 라운드의 선택이 다음 라운드 판정에 남지 않도록 초기화
		currentUser->setBetType(BetType::NONE);
		opponentUser->setBetType(BetType::NONE);
	}

	return result;
}

MatchResult RoomManager::endCheck(const SOCKET socket)
{
	// 정산 후 어느 한쪽의 칩이 소진되었는지 검사해 전체 매치 종료 여부를 결정
	std::lock_guard<std::mutex> lock(roomMutex);

	const auto currentUser = findUserBySocketUnlocked(socket);
	const auto opponentUser = findOpponentUserUnlocked(socket);

	if (!currentUser || !opponentUser)
	{
		return MatchResult::IMPOSSIBLE;
	}

	if (currentUser->getChips() <= 0)
	{
		// 최종 승패가 결정되면 다음 게임은 두 사용자 모두 다시 준비
		currentUser->setisReady(false);
		opponentUser->setisReady(false);

		return MatchResult::FINAL_LOSE;
	}

	if (opponentUser->getChips() <= 0)
	{
		currentUser->setisReady(false);
		opponentUser->setisReady(false);

		return MatchResult::FINAL_WIN;
	}

	return MatchResult::PROGRESS;
}

void RoomManager::specialCase()
{
	// 별도의 베팅 선택이 없는 특수 처리에서 더 큰 카드가 있는 면을 자동 선택
	std::lock_guard<std::mutex> lock(roomMutex);

	for (const auto& [userNumber, user] : users)
	{
		if (!user)
		{
			continue;
		}

		if (user->getFrontCard() < user->getBackCard())
		{
			user->setBetType(BetType::BACK);
		}
		else
		{
			user->setBetType(BetType::FRONT);
		}
	}
}

bool RoomManager::All_User_Start_Ready_State()
{
	// 정확히 두 명이 입장했고 두 사용자 모두 준비한 경우에만 게임을 시작
    std::lock_guard<std::mutex> lock(roomMutex);

	if (users.size() != 2)
	{
		return false;
	}

    for (const auto& [userNumber, user] : users)
    {
        if (!user || !user->getisReady())
        {
            return false;
        }
    }

    return true;
}
bool RoomManager::isroomEmpty() const
{
	// 연결 컨테이너를 기준으로 실제 접속자가 남아 있는지 판단.
	std::lock_guard<std::mutex> lock(roomMutex);
	return connections.empty();
}

int RoomManager::getroomCount() const
{
	// 방 목록에서 정원 여부를 판단할 때 사용하는 현재 참가자 수
	std::lock_guard<std::mutex> lock(roomMutex);
	return static_cast<int>(users.size());
}
std::string RoomManager::getroomName() const
{
	return roomName;
}
std::optional<SOCKET> RoomManager::getOpponentSocket(SOCKET socket) const
{
	// 외부 호출용 함수이므로 직접 락을 잡고 호출
	std::lock_guard<std::mutex> lock(roomMutex);

	return findOpponentSocketUnlocked(socket);
}

void RoomManager::collectBasicBet(const SOCKET socket,int amount)
{
	// 라운드 시작 시 두 플레이어에게 동일한 기본 베팅액을 차감
	if (amount <= 0)
	{
		return;
	}

	std::vector<PendingSend> pendingSends;

	{
		std::lock_guard<std::mutex> lock(roomMutex);

		const auto currentUser = findUserBySocketUnlocked(socket);
		const auto opponentUser = findOpponentUserUnlocked(socket);
		const auto opponentSocket = findOpponentSocketUnlocked(socket);

		if (!currentUser || !opponentUser || !opponentSocket)
		{
			return;
		}

		// 한 명이라도 기본 베팅액이 부족하면 어느 쪽에서도 칩을 차감하지 않음
		if (currentUser->getChips() < amount || opponentUser->getChips() < amount)
		{
			return;
		}

		currentUser->setChips(currentUser->getChips() - amount);
		opponentUser->setChips(opponentUser->getChips() - amount);

		// 두 플레이어의 기본 베팅을 공동 판돈에 추가
		dealerChips += amount * 2;

		currentUser->setBetType(BetType::NONE);
		opponentUser->setBetType(BetType::NONE);

		// 현재 사용자의 변경된 칩
		collectChipUpdateUnlocked(pendingSends,socket,currentUser->getChips());

		collectChipUpdateUnlocked(pendingSends,*opponentSocket,opponentUser->getChips());

		collectBroadcastUnlocked(pendingSends,GAME_CLIENT_EVENT	+ DEALER + CHIP_UPDATE + std::to_string(dealerChips),socket,TargetType::ALL);
	}

	// roomMutex가 해제된 이후 송신
	sendPending(pendingSends);
}
void RoomManager::settleRound(const SOCKET socket,RoundResult roundResult)
{
	// 라운드 결과를 바탕으로 플레이어 칩과 공동 판돈을 정산하고 변경된 값만 알림
	std::vector<PendingSend> pendingSends;

	{
		std::lock_guard<std::mutex> lock(roomMutex);

		const auto currentUser = findUserBySocketUnlocked(socket);
		const auto opponentUser = findOpponentUserUnlocked(socket);
		const auto opponentSocket = findOpponentSocketUnlocked(socket);

		if (!currentUser || !opponentUser || !opponentSocket)
		{
			return;
		}

		// 정산 규칙에는 현재 칩, 면별 베팅액, 공동 판돈을 값으로 전달
		const SettlementState state
		{
			currentUser->getChips(),
			opponentUser->getChips(),

			currentUser->getFrontBet(),
			currentUser->getBackBet(),

			opponentUser->getFrontBet(),
			opponentUser->getBackBet(),

			dealerChips
		};

		const auto settlement = SettlementRules::settle(state,roundResult);

		// 판정 결과와 정산 규칙이 맞지 않으면 기존 방 상태를 변경.
		if (!settlement)
		{
			std::cerr << "[Game] Invalid round result for settlement.\n";
			return;
		}

		const bool currentChipsChanged = currentUser->getChips() != settlement->currentChips;
		const bool opponentChipsChanged = opponentUser->getChips() != settlement->opponentChips;
		const bool dealerChipsChanged = dealerChips != settlement->dealerChips;

		currentUser->setChips(settlement->currentChips);
		opponentUser->setChips(settlement->opponentChips);

		dealerChips = settlement->dealerChips;

		currentUser->setFrontBet(0);
		currentUser->setBackBet(0);

		opponentUser->setFrontBet(0);
		opponentUser->setBackBet(0);

		if (dealerChipsChanged)
		{
			collectBroadcastUnlocked(pendingSends,GAME_CLIENT_EVENT	+ DEALER + CHIP_UPDATE + std::to_string(dealerChips), socket, TargetType::ALL);
		}

		if (currentChipsChanged)
		{
			collectChipUpdateUnlocked(pendingSends,socket,currentUser->getChips());
		}

		if (opponentChipsChanged)
		{
			collectChipUpdateUnlocked(pendingSends,*opponentSocket,opponentUser->getChips());
		}
	}

	// roomMutex가 해제된 이후 실제 송신 큐에 추가
	sendPending(pendingSends);
}

bool RoomManager::deductBetUnlocked(const SOCKET socket,int amount,std::vector<PendingSend>& pendingSends)
{
	// 호출자가 roomMutex를 보유한 상태에서 칩을 차감하고 본인/상대 UI 갱신을 예약
	if (amount <= 0)
	{
		return false;
	}

	const auto user = findUserBySocketUnlocked(socket);

	if (!user || user->getChips() < amount)
	{
		return false;
	}

	user->setChips(user->getChips() - amount);

	collectChipUpdateUnlocked(pendingSends,socket,user->getChips());

	return true;
}
void RoomManager::sendPending(const std::vector<PendingSend>& pendingSends)
{
	// 이 함수는 roomMutex 밖에서 호출한다. Enqueue가 지연되어도 방 상태 접근을 막지 않기 위함
	for (const PendingSend& pending : pendingSends)
	{
		std::cout << "[ID] " << pending.userID << " [Socket] " << pending.socket << " [Message] " << pending.message << '\n';

		if (!pending.connection->Enqueue(pending.message))
		{
			std::cerr << "[Network] Failed to queue packet. Socket: " << pending.socket	<< '\n';
		}
	}
}

void RoomManager::collectOpponentBackCardUnlocked(std::vector<PendingSend>& pendingSends,SOCKET receiverSocket,int backCard)
{
	// 공개 조건을 만족한 상대의 뒷면 카드를 지정된 수신자에게만 보냄
	collectBroadcastUnlocked(pendingSends,GAME_CLIENT_EVENT + OTHER	+ PRINT	+ std::to_string(backCard),	receiverSocket,	TargetType::SELF);
}

void RoomManager::collectWaitUnlocked(std::vector<PendingSend>& pendingSends, SOCKET receiverSocket)
{
	// 상대 카드 공개를 기다려야 하는 사용자에게 대기 상태를 알림
	collectBroadcastUnlocked(pendingSends, GAME_CLIENT_EVENT + WAIT, receiverSocket, TargetType::SELF);
}

void RoomManager::collectVisibleOpponentCardUnlocked(std::vector<PendingSend>& pendingSends,SOCKET userSocket,int frontCard)
{
	// 항상 공개되는 상대방의 앞면 카드만 지정된 사용자에게 전달
	collectBroadcastUnlocked(pendingSends,GAME_CLIENT_EVENT + OTHER + CARD_UPDATE + std::to_string(frontCard),userSocket,TargetType::SELF);
}
void RoomManager::collectMyCardUpdateUnlocked(std::vector<PendingSend>& pendingSends,SOCKET userSocket,const std::pair<int, int>& cards)
{
	// 본인에게는 앞면과 뒷면을 모두 개별 메시지로 전달
	collectBroadcastUnlocked(pendingSends,GAME_CLIENT_EVENT + MY + CARD_UPDATE + FRONT + std::to_string(cards.first),userSocket,TargetType::SELF);
	collectBroadcastUnlocked(pendingSends, GAME_CLIENT_EVENT + MY + CARD_UPDATE + BACK + std::to_string(cards.second), userSocket, TargetType::SELF);
}
void RoomManager::collectBetUpdateUnlocked(std::vector<PendingSend>& pendingSends,SOCKET userSocket,const std::string& cardSide,int betAmount)
{
	// 같은 베팅액을 본인 관점(MY)과 상대 관점(OTHER)의 메시지로 각각 구성
	const std::string amountText = std::to_string(betAmount);

	collectBroadcastUnlocked(pendingSends,GAME_CLIENT_EVENT + MY + BET_UPDATE + cardSide + amountText, userSocket, TargetType::SELF);
	collectBroadcastUnlocked(pendingSends,GAME_CLIENT_EVENT + OTHER + BET_UPDATE + cardSide + amountText, userSocket, TargetType::OTHERS);
}
void RoomManager::collectChipUpdateUnlocked(std::vector<PendingSend>& pendingSends,SOCKET userSocket,int chips)
{
	// 칩 보유량 역시 본인과 상대방의 UI 표현이 다르므로 두 관점으로 전송
	collectBroadcastUnlocked(pendingSends,GAME_CLIENT_EVENT	+ MY+ CHIP_UPDATE + std::to_string(chips),userSocket,TargetType::SELF);
	collectBroadcastUnlocked(pendingSends,GAME_CLIENT_EVENT	+ OTHER	+ CHIP_UPDATE + std::to_string(chips),userSocket,TargetType::OTHERS);
}
void RoomManager::collectBroadcastUnlocked(std::vector<PendingSend>& pendingSends,const std::string& message,SOCKET senderSocket,TargetType targetType)
{
	// roomMutex를 보유한 상태에서 대상만 선별하고 연결 shared_ptr를 PendingSend에 보관
	for (const auto& [targetSocket, connection] : connections)
	{
		bool shouldSend = false;

		switch (targetType)
		{
		case TargetType::SELF:
			shouldSend = targetSocket == senderSocket;
			break;

		case TargetType::OTHERS:
			shouldSend = targetSocket != senderSocket;
			break;

		case TargetType::ALL:
			shouldSend = true;
			break;
		}

		if (!shouldSend || !connection)
		{
			continue;
		}

		const auto targetUser =	findUserBySocketUnlocked(targetSocket);

		// 실제 전송은 락 밖에서 수행할 수 있도록 필요한 정보를 값으로 저장
		pendingSends.push_back(
			PendingSend
			{
				targetSocket,
				connection,
				targetUser ? targetUser->getID() : "Unknown",
				message
			}
		);
	}
}
void RoomManager::printCard(const SOCKET socket)
{
	// 양쪽의 베팅 면을 기준으로 상대 뒷면 공개 또는 대기 메시지를 결정
	std::vector<PendingSend> pendingSends;

	{
		std::lock_guard<std::mutex> lock(roomMutex);

		const auto currentUser = findUserBySocketUnlocked(socket);
		const auto opponentUser = findOpponentUserUnlocked(socket);
		const auto opponentSocket =	findOpponentSocketUnlocked(socket);

		if (!currentUser || !opponentUser || !opponentSocket)
		{
			return;
		}

		const BetType currentBetType = currentUser->getBetType();
		const BetType opponentBetType = opponentUser->getBetType();

		// BACK 또는 BOTH를 선택한 플레이어의 뒷면 카드만 상대에게 공개
		const bool currentRevealsBack = currentBetType == BetType::BACK || currentBetType == BetType::BOTH;
		const bool opponentRevealsBack = opponentBetType == BetType::BACK || opponentBetType == BetType::BOTH;

		// 현재 사용자가 상대방의 뒤 카드를 볼 수 있는지 처리
		if (opponentRevealsBack)
		{
			collectOpponentBackCardUnlocked(pendingSends,socket,opponentUser->getBackCard());
		}
		else if (currentRevealsBack)
		{
			collectWaitUnlocked(pendingSends,socket);
		}

		// 상대 사용자가 현재 사용자의 뒤 카드를 볼 수 있는지 처리
		if (currentRevealsBack)
		{
			collectOpponentBackCardUnlocked(pendingSends,*opponentSocket,currentUser->getBackCard());
		}
		else if (opponentRevealsBack)
		{
			collectWaitUnlocked(pendingSends,*opponentSocket);
		}
	}

	// roomMutex가 해제된 이후 송신
	sendPending(pendingSends);
}