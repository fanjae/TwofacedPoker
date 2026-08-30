#pragma once

#include <memory>
#include <mutex>
#include <vector>

class ClientConnection;
class RoomRegistry;

class ClientWorkerManager
{
public:
    ClientWorkerManager();
    ~ClientWorkerManager();

    ClientWorkerManager(const ClientWorkerManager&) = delete;

    ClientWorkerManager& operator=(const ClientWorkerManager&) = delete;

    bool Start(std::shared_ptr<ClientConnection> connection,std::shared_ptr<RoomRegistry> roomRegistry);

    void ReapFinished();

    void StopAll();

private:
    struct Worker;

    std::mutex mutex;
    std::vector<std::unique_ptr<Worker>> workers;

    bool stopping = false;
};