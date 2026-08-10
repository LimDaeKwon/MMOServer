# LoginServer

MySQL과 Redis를 연동하여 로그인 Content를 병렬 처리하는 C++ 서버

**개요**

여러 I/O Worker 스레드에서 로그인 요청을 직접 처리하며, MySQL에서 계정 정보를 조회하고 Redis에 인증 토큰을 저장한다.

로그인에 성공하면 클라이언트에 계정 정보와 게임 서버 및 채팅 서버의 접속 정보를 전달한다.

**구조**

`LoginServer`

- `NetLibrary`를 상속한 로그인 서버
- I/O Worker 스레드에서 로그인 요청을 병렬 처리
- MySQL에서 계정 ID와 닉네임 조회
- Redis에 계정별 인증 토큰 저장
- 게임 서버와 채팅 서버 접속 정보 전달
- TLS를 이용하여 스레드별 MySQL 연결 관리

`LoginMonitoringClient`

- `LanClient`를 상속한 모니터링 클라이언트
- 서버 CPU, 메모리, 세션 수와 인증 처리량 수집
- 수집한 상태 정보를 Monitoring Server로 전송

`GameDefine` / `PacketDefine`

- 계정 정보와 서버 주소 크기 설정
- 로그인 요청과 응답 패킷 정의
- 로그인 결과 상태 정의

`MonitoringDefine`

- Monitoring Server와 주고받는 패킷 및 데이터 유형 정의

`LoginMain`

- 설정 파일 로드와 서버 시작
- 로그인 처리량과 연결 종료 사유 출력

`include` / `libmysql`

- MySQL 연결과 계정 조회에 사용하는 클라이언트 라이브러리

`cpp_redis` / `tacopie`

- Redis 연결에 사용하는 외부 라이브러리
