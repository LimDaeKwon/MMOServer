# IOCPSingleMMOTCPFighter

IOCP 기반 네트워크 입출력과 단일 Content 스레드로 2D MMO Fighter Content를 처리하는 C++ 서버

**개요**

접속, 수신, 송신과 연결 종료는 여러 I/O Worker 스레드에서 처리하고, 발생한 네트워크 이벤트는 메시지 큐를 통해 단일 Content 스레드로 전달한다.

Content 스레드는 이벤트와 게임 로직을 순서대로 처리하며, 캐릭터의 8방향 이동과 공격, 섹터 갱신 및 위치 보정을 수행한다.

**구조**

`IOCPServer` / `IOCPServerRB`

- IOCP 기반 접속, 수신, 송신 및 연결 종료 처리
- Accept, I/O Worker, Timeout 및 Monitoring 스레드 관리
- 패킷큐와 링버퍼 기반 송신 버퍼 제공
- 세션 배열과 참조 카운트를 이용한 세션 관리

`MMOTCPServerSingle`

- 패킷큐 기반 송신 버퍼 사용
- 네트워크 이벤트를 메시지 큐에 등록
- 단일 Content 스레드에서 게임 로직 처리

`MMOTCPServerSingleRB`

- 링버퍼 기반 송신 버퍼 사용
- `MMOTCPServerSingle`과 동일한 Content 로직 처리

`Character` / `Sector`

- 캐릭터 상태와 섹터 정보 관리

`GameDefine` / `PacketDefine`

- 게임 설정값과 송수신 패킷 정의

`Dummy`

- 다중 접속 및 부하 테스트용 더미 클라이언트

`MMO_TCPFighter_Client_20231213`

- 서버 동작 확인을 위한 테스트 클라이언트
