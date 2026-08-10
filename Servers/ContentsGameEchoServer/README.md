# ContentsGameEchoServer

세션을 Content 그룹별로 관리하고 각 그룹에서 메시지를 직렬 처리하는 C++ Echo 서버

**개요**

네트워크 라이브러리가 세션이 속한 Content 그룹을 관리하며, 접속, 메시지와 연결 종료 이벤트를 해당 그룹으로 전달한다.

테스트 서버는 Auth 그룹과 Echo 그룹으로 구성한다. 처음 접속한 세션은 Auth 그룹에서 로그인 요청을 처리한 뒤 Echo 그룹으로 이동하며, 각 그룹은 독립적인 메시지 큐와 업데이트 루프를 가진다.

**구조**

`GameEchoServer`

- `ContentsNetLibrary`를 상속한 Echo 서버
- Auth 그룹과 Echo 그룹 생성 및 등록
- 세션과 Player 객체 관리
- 그룹별 인원과 업데이트 처리량 수집

`AuthGroup`

- 새로 접속한 세션의 로그인 요청 처리
- 로그인 패킷 확인 후 세션을 Echo 그룹으로 이동
- 그룹에 속한 세션과 프레임 업데이트 관리

`EchoGroup`

- Echo 및 Heartbeat 패킷 처리
- 요청에 포함된 계정 정보 확인
- Echo 응답 패킷 전송
- 그룹에 속한 세션과 프레임 업데이트 관리

`GameMonitoringClient`

- `LanClient`를 상속한 모니터링 클라이언트
- 서버 CPU, 메모리, 세션 수와 그룹별 상태 수집
- 수집한 상태 정보를 Monitoring Server로 전송

`GameDefine` / `PacketDefine`

- Content 그룹 ID와 업데이트 주기 설정
- 로그인, Echo 및 Heartbeat 패킷 정의

`MonitoringDefine`

- Monitoring Server와 주고받는 패킷 및 데이터 유형 정의

`GameEchoMain`

- 설정 파일 로드와 서버 시작
- 그룹별 인원, FPS와 네트워크 처리량 출력
- 프로파일링 데이터 출력 및 초기화
