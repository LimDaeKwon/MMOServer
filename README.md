# MMOServer

MMO 서버를 구성하는 네트워크 라이브러리와 게임, 로그인, 채팅, 모니터링 서버를 C++로 구현한 프로젝트입니다.  
다양한 네트워크 및 콘텐츠 처리 구조를 적용하고, 부하 테스트와 프로파일링을 통해 성능을 비교하고 개선했습니다.

## 프로젝트 구성

| 프로젝트 | 설명 |
|---|---|
| [DKServerCore](libs/DKServerCore/README.md) | 네트워크, 패킷, 메모리 관리 및 모니터링 공통 라이브러리 |
| [SelectMMOTCPFighter](Servers/SelectMMOTCPFighter/README.md) | Select 기반 단일 스레드 MMO Fighter 서버 |
| [IOCPSingleMMOTCPFighter](Servers/IOCPSingleMMOTCPFighter/README.md) | IOCP와 단일 콘텐츠 스레드로 구성한 MMO Fighter 서버 |
| [SingleChatServer](Servers/SingleChatServer/README.md) | 채팅 콘텐츠를 단일 스레드에서 처리하는 서버 |
| [MultiChatServer](Servers/MultiChatServer/README.md) | 여러 Worker 스레드에서 채팅 콘텐츠를 병렬로 처리하는 서버 |
| [LoginServer](Servers/LoginServer/README.md) | MySQL과 Redis를 연동한 로그인 서버 |
| [ContentsGameEchoServer](Servers/ContentsGameEchoServer/README.md) | 콘텐츠 그룹별 독립적인 처리 구조를 적용한 Echo 서버 |
| [MonitoringServer](Servers/MonitoringServer/README.md) | 서버 상태 정보를 수집하고 전달하는 모니터링 서버 |

## 구조

- `libs/DKServerCore`: 네트워크, 패킷, 메모리 관리 등 서버 공통 기능
- `Servers`: 공통 라이브러리를 기반으로 구현한 실행 서버 프로젝트

각 서버 프로젝트에서 네트워크 입출력 모델과 콘텐츠 처리 방식에 따른 구조적 차이와 성능을 검증했습니다.

## 개발 환경

| 구분 | 환경 |
|---|---|
| 언어 | C++ |
| 플랫폼 | Windows |
| 개발 도구 | Visual Studio |
| 네트워크 | Winsock2, IOCP |
| 데이터베이스 | MySQL, Redis |
