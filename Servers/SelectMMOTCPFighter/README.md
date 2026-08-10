# SelectMMOTCPFighter

Select 기반 단일 스레드 구조로 2D MMO Fighter Content를 처리하는 C++ 서버

**개요**

네트워크 입출력과 Content 로직을 하나의 게임 루프 스레드에서 처리한다.

캐릭터는 2D 필드에서 8방향 이동과 공격을 수행하며, 서버 위치와 클라이언트 위치의 차이가 허용 범위를 벗어나면 Sync 패킷으로 보정한다.

**구조**

`SelectServer`

- Select 기반 접속, 수신, 송신 및 연결 종료 처리
- 세션 Timeout과 프레임 업데이트 관리
- 세션을 묶어 Select 호출 범위 단위로 처리

`SelectMMOTCPFighter`

- 캐릭터 생성과 삭제
- 이동, 공격 및 피격 처리
- 섹터 이동과 주변 캐릭터에 대한 패킷 전파
- 서버 기준 위치 계산과 Sync 처리

`BasicSelectMMOTCPFighter`

- 패킷큐 기반 송신 버퍼를 사용하는 비교용 구현

`Character` / `Sector`

- 캐릭터 상태와 섹터 정보 관리

`GameDefine` / `PacketDefine`

- 게임 설정값과 송수신 패킷 정의
