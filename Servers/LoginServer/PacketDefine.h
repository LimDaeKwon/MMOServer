#pragma once


constexpr unsigned short PacketCsLoginServer = 100;
constexpr unsigned short PacketCsLoginReqLogin = 101;


constexpr unsigned short PacketScLoginResLogin = 102;

constexpr int PacketLoginRequestDataSize = 72;

constexpr int LoginStatusNone = -1;
constexpr int LoginStatusFail = 0;
constexpr int LoginStatusOk = 1;
constexpr int LoginStatusGame = 2;
constexpr int LoginStatusAccountMiss = 3;
constexpr int LoginStatusSessionMiss = 4;
constexpr int LoginStatusStatusMiss = 5;
constexpr int LoginStatusNoServer = 6;