# MultiChatServer

여러 I/O Worker 스레드가 채팅 Content를 직접 병렬 처리하는 C++ 서버

**개요**

접속, 수신, 송신과 연결 종료뿐만 아니라 로그인, 섹터 이동과 채팅 Content도 여러 I/O Worker 스레드에서 처리한다.

Redis에 저장된 인증 토큰을 검증한 뒤 접속을 허용하며, 채팅 메시지를 보낸 유저가 위치한 섹터와 주변 8개 섹터에 메시지를 전파한다.

**구조**

`MultiChatServer`

- `NetLibrary`를 상속한 채팅 서버
- I/O Worker 스레드에서 Content 로직을 병렬 처리
- Redis를 통한 인증 토큰 검증
- 유저 목록과 계정 목록을 SRW Lock으로 보호
- 섹터 이동은 Exclusive Lock으로 처리
- 채팅 전파는 주변 섹터의 Shared Lock을 획득하여 처리

`ChatMonitoringClient`

- `LanClient`를 상속한 모니터링 클라이언트
- 서버 CPU, 메모리, 세션 수와 처리량 수집
- 수집한 상태 정보를 Monitoring Server로 전송

`Player`

- 세션 ID와 계정 정보 관리
- 로그인 상태와 현재 섹터 위치 저장

`Sector`

- 섹터 좌표와 주변 섹터 정보 관리

`GameDefine` / `PacketDefine`

- 채팅 크기와 섹터 범위 설정
- 로그인, 섹터 이동, 채팅 및 Heartbeat 패킷 정의

`MonitoringDefine`

- Monitoring Server와 주고받는 패킷 및 데이터 유형 정의

`MultiChatMain`

- 설정 파일 로드와 서버 시작
- 서버 처리량과 연결 종료 사유 출력
- 프로파일링 데이터 출력 및 초기화

`cpp_redis` / `tacopie`

- Redis 연결에 사용하는 외부 라이브러리
