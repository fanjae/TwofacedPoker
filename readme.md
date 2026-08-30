# TwoFacedPoker Server

> TwoFacedPoker는 tvN 예능 프로그램 「더 지니어스」에 등장한 양면 포커를 온라인 2인용 게임으로 구현한 프로젝트입니다. 플레이어는 앞면, 뒷면 또는 양면에 칩을 베팅하고, 서버가 관리하는 턴과 카드 공개 결과에 따라 라운드 및 최종 승패를 결정합니다.

- 이 저장소는 TwoFacedPoker의 **Windows C++ 게임 서버**입니다.
- Windows 클라이언트는 [TwoFacedPoker Client](https://github.com/fanjae/TwofacedPoker_Client) 저장소에서 확인할 수 있습니다.

## 프로젝트 개요

| 항목 | 내용 |
|---|---|
| 프로젝트명 | TwoFacedPoker Server |
| 개발 환경 | C++17, WinSock2, Windows API |
| 실행 환경 | Windows |
| IDE | Visual Studio 2022 |
| 프로젝트 형식 | Visual Studio C++ Console Application |
| 기본 게임 인원 | 방당 2명 |

## 서버 아키텍처

서버는 TCP 기반 클라이언트-서버 구조로 동작합니다.

```text
┌──────────────┐      TCP      ┌─────────────────────────────┐
│ Windows      │ ◀───────────▶ │ TwoFacedPoker Server        │
│ Client × N   │                │                             │
└──────────────┘                │ Main accept loop            │
                                │ ├─ Client worker × N        │
                                │ │  └─ Client sender thread  │
                                │ └─ Shared RoomRegistry      │
                                │    ├─ RoomManager × N       │
                                │    └─ GameManager × N        │
                                └─────────────────────────────┘
```

- 메인 스레드는 IPv4 TCP 소켓을 열고 클라이언트 연결을 수락합니다.
- 클라이언트마다 수신 및 이벤트 처리를 담당하는 worker를 생성합니다.
- 연결별 sender thread와 송신 큐를 사용해 여러 게임 이벤트의 전송을 직렬화합니다.
- `RoomRegistry`가 서버 전체 방 목록을 공유하고, 각 `RoomManager`가 사용자·카드·베팅·칩 상태를 관리합니다.
- 게임 흐름은 `GameManager`, 라운드 상태는 `GameSession`, 베팅·정산·승패 판정은 별도 규칙 모듈로 분리되어 있습니다.
- 공유 상태는 mutex로 보호하며, 방 상태 잠금을 해제한 뒤 실제 네트워크 송신을 수행합니다.

## 실행 방법

### 사전 요구 사항

- Windows
- Visual Studio 2022
- `Desktop development with C++` 워크로드
- MSVC v143 빌드 도구
- Windows 10 SDK 이상

### Visual Studio에서 실행

1. `TwofacedPoker.sln`을 Visual Studio 2022에서 엽니다.
2. 플랫폼을 `x64`, 구성을 `Debug` 또는 `Release`로 선택합니다.
3. 프로젝트 속성의 `Debugging > Command Arguments`에 서버 포트를 입력합니다. 예: `9190`
4. 빌드 후 실행합니다.

서버는 실행 시 포트 번호를 반드시 하나의 인자로 받아야 합니다.

```text
TwofacedPoker.exe <port>
```

예를 들어 Release 빌드 결과를 직접 실행할 때는 다음과 같습니다.

```powershell
.\x64\Release\TwofacedPoker.exe 9190
```

- 포트는 `1`부터 `65535` 사이의 숫자여야 합니다.
- 서버는 사용 가능한 모든 로컬 IPv4 주소(`0.0.0.0`)에서 연결을 수신합니다.
- 서버를 정상 종료하려면 콘솔에서 `Ctrl+C` 또는 `Ctrl+Break`를 입력합니다.
- 외부 클라이언트가 접속할 경우 Windows 방화벽에서 해당 TCP 포트의 인바운드 허용이 필요할 수 있습니다.

### 클라이언트 연결

클라이언트 프로젝트 루트의 `server.ini`에 서버 주소와 포트를 입력합니다.

```ini
[server]
server=127.0.0.1
port=9190
```

같은 PC에서 테스트할 때는 `127.0.0.1`을 사용하고, 다른 PC에서 접속할 때는 서버 PC의 IPv4 주소를 사용합니다.

## 구현 기능

| 기능 | 설명 |
|---|---|
| 서버 수신 | 지정 포트에서 IPv4 TCP 연결 수락 및 다중 클라이언트 처리 |
| 로그인 | 접속한 클라이언트에 서버가 사용자 번호를 할당 |
| 로비 | 현재 생성된 방 목록 조회 |
| 방 관리 | 방 생성, 방 번호를 통한 입장, 방당 최대 2명 관리 |
| 준비 및 시작 | 두 사용자의 준비 상태를 동기화하고 모두 준비된 경우 게임 시작 |
| 실시간 채팅 | 같은 방의 사용자에게 채팅 메시지 전달 |
| 게임 진행 | 턴 관리, 카드 분배, 카드 공개, 라운드 초기화 |
| 베팅 | 앞면(`Front`), 뒷면(`Back`), 양면(`Both`) 베팅과 베팅 취소·포기 처리 |
| 라운드 정산 | 승리·패배·무승부 및 양면 베팅 결과에 따른 칩 정산 |
| 최종 결과 | 칩 소진 등 게임 종료 조건을 확인하고 최종 승패 전달 |
| 연결 종료 | 방 퇴장, 비정상 연결 종료, 빈 방 제거 및 소켓 정리 |

## 네트워크 및 프로토콜

### 패킷 형식

모든 패킷은 4바이트 길이 헤더와 문자열 본문으로 구성됩니다.

| 구간 | 크기 | 설명 |
|---|---:|---|
| Length | 4 bytes | 본문 길이, unsigned 32-bit network byte order(Big-Endian) |
| Body | 1~1024 bytes | UTF-8 기반 문자열 명령 또는 이벤트 데이터 |

- 서버는 TCP의 부분 수신·부분 전송을 고려해 지정된 길이를 모두 읽고 모두 전송합니다.
- 빈 패킷과 `1024`바이트를 초과하는 패킷은 거부합니다.
- 송신 큐는 연결별 최대 `256`개 메시지로 제한하며, 큐가 초과하면 해당 연결을 종료 대상으로 처리합니다.

### 주요 명령 범주

| 범주 | 예시 |
|---|---|
| 로그인 및 연결 | `/Login`, `/Close_Socket` |
| 로비 및 방 | `/Get_Chatting_Room`, `/Create_Chatting_Room <name>`, `/Join_Chatting_Room <number>`, `/Exit_Room` |
| 사용자 상태 | `/User_Update` |
| 방 이벤트 | `/Room_Event ...` |
| 게임 이벤트 | `/Game_Client_Event ...` |
| 채팅 | 정의된 명령이 아닌 문자열은 방 채팅 메시지로 처리 |

게임 이벤트에는 준비 상태, 게임 시작, 턴 변경, 기본 베팅, 카드·칩·베팅 상태 갱신, 카드 공개, 라운드 결과 및 최종 결과가 포함됩니다.

## 프로젝트 구조

```text
TwofacedPoker/
├── Common/
│   ├── Constants.h       # 프로토콜 문자열과 공통 상수
│   └── Foundation.h      # 카드, 베팅, 사용자, 게임 결과 타입
├── Game/
│   ├── BettingRules.*     # 베팅 가능 여부 및 베팅 결과 판정
│   ├── Deck.*             # 카드 덱 및 카드 분배
│   ├── GameManager.*      # 게임 진행 오케스트레이션
│   ├── GameSession.*      # 게임 상태와 턴 관리
│   ├── RoundResolver.*    # 카드 비교 및 라운드 결과 판정
│   └── SettlementRules.*  # 라운드 칩 정산
├── Network/
│   ├── ClientConnection.* # 연결별 송신 큐와 sender thread
│   ├── ClientHandle.*     # 클라이언트 이벤트 및 명령 처리
│   └── ClientWorkerManager.* # 연결 worker 생성·수거·종료
├── Protocol/
│   ├── Packet.*            # 길이 헤더 기반 TCP 송수신
│   ├── ClientCommandParser.* # 클라이언트 명령 분류
│   └── BetCommandParser.* # 베팅 명령 파싱
├── Room/
│   ├── RoomRegistry.*      # 전체 방 목록 및 방 생성·입장
│   └── RoomManager.*       # 방 사용자·상태·브로드캐스트 관리
├── main.cpp                # Winsock 초기화, accept loop, 서버 종료
└── TwofacedPoker.sln
```

## 리팩토링 변경 사항

- 게임 진행 책임을 `GameManager` 오케스트레이션 계층으로 분리했습니다.
- 게임 상태와 턴 전환을 `GameSession`으로 캡슐화했습니다.
- 베팅 판정, 라운드 결과, 칩 정산 규칙을 각각의 모듈로 분리했습니다.
- 공용 도메인 타입과 프로토콜 상수를 `Common` 영역으로 정리했습니다.
- TCP 부분 송수신에 대응하는 `RecvAll`·`SendAll`과 패킷 크기 검증을 적용했습니다.
- 연결별 송신 큐와 sender thread를 도입해 동시 송신을 직렬화했습니다.
- 서버 종료 시 수신·송신 소켓을 종료하고 worker thread를 join하도록 정리했습니다.

## 관련 저장소

- [TwoFacedPoker Client](https://github.com/fanjae/TwofacedPoker_Client)

## 개발 일지

- [TwoFacedPoker 개발일지 Blog](https://fanjae.tistory.com/category/Projects/Two%20Faced%20Poker)

## 플레이 영상

- 준비 중
