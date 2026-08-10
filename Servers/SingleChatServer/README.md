# SingleChatServer

IOCP 네트워크 이벤트와 채팅 Content를 단일 Content 스레드에서 처리하는 C++ 서버

**개요**

접속, 수신, 송신과 연결 종료는 여러 I/O Worker 스레드에서 처리하고, 발생한 네트워크 이벤트는 메시지 큐를 통해 단일 Content 스레드로 전달한다.

유저를 섹터 단위로 관리하며, 채팅 메시지를 보낸 유저가 위치한 섹터와 주변 8개 섹터에 메시지를 전파한다.

**구조**

`ChattingServerSingle`

- `NetLibrary`를 상속한 채팅 서버
- 네트워크 이벤트를 메시지 큐에 등록
- 단일 Content 스레드에서 이벤트를 순서대로 처리
- 로그인, 섹터 이동, 채팅 및 Heartbeat 처리

`Player`

- 세션 ID와 계정 정보 관리
- 로그인 상태와 현재 섹터 위치 저장

`Sector`

- 섹터 좌표와 주변 섹터 정보 관리

`GameDefine` / `PacketDefine`

- 채팅 크기와 섹터 범위 설정
- 로그인, 섹터 이동, 채팅 및 Heartbeat 패킷 정의

`SingleChatMain`

- 설정 파일 로드와 서버 시작
- 서버 처리량과 시스템 상태 출력
- 프로파일링 데이터 출력 및 초기화
