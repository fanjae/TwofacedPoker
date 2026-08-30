#include "ClientWorkerManager.h"
#include "ClientConnection.h"
#include "ClientHandle.h"
#include "../Room/RoomRegistry.h"

#include <atomic>
#include <thread>
#include <utility>

struct ClientWorkerManager::Worker
{
    // 하나의 클라이언트 연결과 해당 연결을 처리하는 수신 스레드 소유
    std::shared_ptr<ClientConnection> connection;
    std::shared_ptr<std::atomic<bool>> finished;
    std::thread thread;

    Worker(std::shared_ptr<ClientConnection> connection,std::shared_ptr<RoomRegistry> roomRegistry) : connection(std::move(connection)), finished(std::make_shared<std::atomic<bool>>(false)),
        thread([connection = this->connection,roomRegistry = std::move(roomRegistry),finished = this->finished]()
            {
                try
                {
                    // 클라이언트 패킷 수신과 명령 처리가 끝날때 까지 유지
                    ConnectClient(connection,roomRegistry);
                }
                catch (...)
                {
                    // 예외가 밖으로 전파되어 std::terminate가 호출되는 것을 방지
                    connection->Stop();
                }

                // Worker를 안전하게 수거할 수 있도록 완료 상태 공개
                finished->store(true,std::memory_order_release);
            }
        )
    {
    }

    ~Worker()
    {
        if (connection)
        {
            // recv()를 깨워 worker가 종료되도록 한다.
            connection->Stop();
        }

        if (thread.joinable())
        {
            thread.join();
        }
    }

    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;
};

ClientWorkerManager::ClientWorkerManager() = default;

ClientWorkerManager::~ClientWorkerManager()
{
    StopAll();
}

bool ClientWorkerManager::Start(std::shared_ptr<ClientConnection> connection,std::shared_ptr<RoomRegistry> roomRegistry)
{
    // Worker는 두 객체를 모두 사용하므로 유효하지 않은 요청은 스레드를 만들기 전에 거부
    if (!connection || !roomRegistry)
    {
        return false;
    }

    // 이미 끝난 worker를 먼저 정리
    ReapFinished();

    std::lock_guard<std::mutex> lock(mutex);

    if (stopping)
    {
        return false;
    }

    try
    {
        // Worker 생성과 동시에 해당 클라이언트를 담당하는 스레드가 시작.
        workers.push_back(std::make_unique<Worker>(std::move(connection),std::move(roomRegistry)));
    }
    catch (...)
    {
        return false;
    }

    return true;
}

void ClientWorkerManager::ReapFinished()
{
    // 완료된 Worker를 잠금 구간에서 분리한 뒤, 잠금 밖에서 소멸.
    std::vector<std::unique_ptr<Worker>> finishedWorkers;

    {
        std::lock_guard<std::mutex> lock(mutex);

        auto workerIt = workers.begin();

        while (workerIt != workers.end())
        {
            const bool finished = (*workerIt)->finished->load(std::memory_order_acquire);

            if (!finished)
            {
                ++workerIt;
                continue;
            }

            finishedWorkers.push_back(std::move(*workerIt));

            workerIt = workers.erase(workerIt);
        }
    }

    // mutex가 해제된 뒤 Worker 소멸
    // Worker 소멸자에서 thread를 join한다.
    finishedWorkers.clear();
}

void ClientWorkerManager::StopAll()
{
    // 서버 종료 시 전체 Worker 정리
    std::vector<std::unique_ptr<Worker>> stoppingWorkers;
    {
        std::lock_guard<std::mutex> lock(mutex);

        if (stopping)
        {
            return;
        }

        stopping = true;
        stoppingWorkers.swap(workers);
    }

    // 각 연결을 shutdown한 뒤 worker thread를 join한다.
    // mutex 밖에서 하도록 처리 
    stoppingWorkers.clear();
}