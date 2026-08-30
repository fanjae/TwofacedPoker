#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

class RoomManager;
class GameManager;
class ClientConnection;

struct RoomContext
{
    std::shared_ptr<RoomManager> roomManager;
    std::shared_ptr<GameManager> gameManager;
};

struct CreatedRoom
{
    int roomNumber;
    std::shared_ptr<RoomManager> roomManager;
};

struct RoomSummary
{
    int roomNumber;
    std::string roomName;
};

class RoomRegistry
{
public:
    std::optional<RoomContext> find(int roomNumber) const;

    std::shared_ptr<RoomManager> findRoom(int roomNumber) const;

    std::shared_ptr<GameManager> findGame(int roomNumber) const;

    std::optional<CreatedRoom> createAndJoin(const std::string& roomName, int userNumber, const std::string& userId, const std::shared_ptr<ClientConnection>& connection);

    std::shared_ptr<RoomManager> tryJoin(int roomNumber,int userNumber,const std::string& userId,const std::shared_ptr<ClientConnection>& connection);

    std::vector<RoomSummary> listAvailable() const;

    bool eraseIfEmpty(int roomNumber,const std::shared_ptr<RoomManager>& expectedRoom);

private:
    mutable std::mutex mutex;
    std::map<int, RoomContext> rooms;
    int nextRoomNumber = 1;
};