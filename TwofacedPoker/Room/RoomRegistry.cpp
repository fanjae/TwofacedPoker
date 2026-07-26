#include "../Network/ClientConnection.h"
#include "../Game/GameManager.h"
#include "RoomManager.h"
#include "RoomRegistry.h"

// 방 번호로 방의 전체 컨텍스트 검색
std::optional<RoomContext> RoomRegistry::find(int roomNumber) const
{
    // rooms 컨테이너가 여러 클라이언트 스레드에서 접근 가능하므로 Lock으로 보호
    std::lock_guard<std::mutex> lock(mutex);

    const auto it = rooms.find(roomNumber);

    // 방번호 미존재시 nullopt 반환
    if (it == rooms.end())
    {
        return std::nullopt;
    }

    // RoomContext를 값으로 복사 반환
    return it->second;
}

// 방 번호에 해당하는 RoomManager 반환
std::shared_ptr<RoomManager> RoomRegistry::findRoom(int roomNumber) const
{
    // find()를 통해 RoomContext를 먼저 조회
    const auto context = find(roomNumber);

    return context ? context->roomManager : nullptr;
}

// 방 번호에 해당하는 GameMameManager 반환
std::shared_ptr<GameManager> RoomRegistry::findGame(int roomNumber) const
{
    const auto context = find(roomNumber);

    return context ? context->gameManager : nullptr;
}

// 새로운 방을 생성하고, 방을 생성한 사용자 즉시 입장.
std::optional<CreatedRoom> RoomRegistry::createAndJoin(const std::string& roomName,int userNumber,const std::string& userId,const std::shared_ptr<ClientConnection>& connection)
{
    // 방 번호 발급, 방 생성, rooms 삽입 등을 하나의 임계 구역에서 처리
    std::lock_guard<std::mutex> lock(mutex);

    const int roomNumber = nextRoomNumber++;

    // 실제 방 참가자와 연결 정보를 관리할 roomManager 생성
    auto roomManager = RoomManager::createRoom(roomName);

    if (!roomManager || !roomManager->addUser(userNumber,userId,connection))
    {
        return std::nullopt;
    }

    // gameManager 생성
    auto gameManager = std::make_shared<GameManager>(roomManager);

    // 방 번호를 키로 하여 RoomManager, GameMAnager 등록
    rooms.emplace(roomNumber, RoomContext{ roomManager,gameManager });

    return CreatedRoom
    {
        roomNumber,
        roomManager
    };
}

// 기존 방에 사용자 입장
std::shared_ptr<RoomManager> RoomRegistry::tryJoin(int roomNumber,int userNumber,const std::string& userId,const std::shared_ptr<ClientConnection>& connection)
{
    // 방 검색과 사용자 추가 사이에 방이 삭제 또는 변경되지 않도록 lock 이후 처리
    std::lock_guard<std::mutex> lock(mutex);

    const auto it = rooms.find(roomNumber);

    // 방이 없거나 RoomManager가 비정상 상태인 경우 입장 처리 X
    if (it == rooms.end() || !it->second.roomManager)
    {
        return nullptr;
    }

    const auto& roomManager = it->second.roomManager;

    if (!roomManager->addUser(userNumber,userId,connection))
    {
        return nullptr;
    }

    return roomManager;
}

// 현재 입장 가능한 방 목록 반환
std::vector<RoomSummary> RoomRegistry::listAvailable() const
{
    // Registry mutex를 오래 유지하지 않기 위해 방 번호와 RoomManager 포인터만 복사
    std::vector<std::pair<int,std::shared_ptr<RoomManager>>> snapshot;

    {
        
        std::lock_guard<std::mutex> lock(mutex);

        // 불필요한 vector 재할당 줄임.
        snapshot.reserve(rooms.size());

        for (const auto& [roomNumber, context] : rooms)
        {
            snapshot.emplace_back(roomNumber,context.roomManager);
        }
    }

    // 각 RoomManager의 mutex를 개별적으로 사용해 방 상태 확인
    std::vector<RoomSummary> result;

    for (const auto& [roomNumber, room] : snapshot)
    {
        if (room && room->getroomCount() < 2)
        {
            result.push_back(RoomSummary{ roomNumber,room->getroomName()});
        }
    }

    return result;
}

// 특정 방이 비어있을 경우 Registry에서 제거.
bool RoomRegistry::eraseIfEmpty(int roomNumber,const std::shared_ptr<RoomManager>& expectedRoom)
{
    // 방 검색 또는 삭제 과정 중 변경되지 않도록 lock 처리
    std::lock_guard<std::mutex> lock(mutex);

    const auto it = rooms.find(roomNumber);

    // 방이 없거나, Registry에 등록된 roomManager가 다르면 삭제하지 않도록 처리
    if (it == rooms.end() || it->second.roomManager != expectedRoom)
    {
        return false;
    }

    // 빈방이 아닌 경우 삭제X.
    if (!expectedRoom->isroomEmpty())
    {
        return false;
    }

    // 방이 비었으므로, Registry에서 제거
    rooms.erase(it);
    return true;
}