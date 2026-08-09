#pragma once

constexpr unsigned short PacketSsMonitorLogin = 20001;
constexpr unsigned short PacketSsMonitorDataUpdate = 20002;

constexpr int MonitorServerTypeGame = 2;

constexpr unsigned char MonitorDataTypeGameServerRun = 10;
constexpr unsigned char MonitorDataTypeGameServerCpu = 11;
constexpr unsigned char MonitorDataTypeGameServerMemory = 12;
constexpr unsigned char MonitorDataTypeGameSession = 13;
constexpr unsigned char MonitorDataTypeGameAuthPlayer = 14;
constexpr unsigned char MonitorDataTypeGameGamePlayer = 15;
constexpr unsigned char MonitorDataTypeGameAcceptTps = 16;
constexpr unsigned char MonitorDataTypeGamePacketRecvTps = 17;
constexpr unsigned char MonitorDataTypeGamePacketSendTps = 18;
constexpr unsigned char MonitorDataTypeGameDbWriteTps = 19;
constexpr unsigned char MonitorDataTypeGameDbWriteMessage = 20;
constexpr unsigned char MonitorDataTypeGameAuthThreadFps = 21;
constexpr unsigned char MonitorDataTypeGameGameThreadFps = 22;
constexpr unsigned char MonitorDataTypeGamePacketPool = 23;