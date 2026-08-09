#pragma once

constexpr unsigned short PacketCsGameReqLogin = 1001;
constexpr unsigned short PacketScGameResLogin = 1002;

constexpr unsigned short PacketCsGameReqEcho = 5000;
constexpr unsigned short PacketScGameResEcho = 5001;
constexpr unsigned short PacketCsGameReqHeartbeat = 5002;

constexpr int PacketGameReqLoginSize = 78;
constexpr int PacketGameReqEchoSize = 18;
constexpr int PacketGameReqHeartbeatSize = 2;

constexpr unsigned char GameLoginStatusFail = 0;
constexpr unsigned char GameLoginStatusOk = 1;