#pragma once
constexpr unsigned short PacketSsMonitorLogin = 20001;
constexpr unsigned short PacketSsMonitorDataUpdate = 20002;

constexpr int MonitorServerTypeChat = 3;

constexpr unsigned char MonitorDataTypeChatServerRun = 30;
constexpr unsigned char MonitorDataTypeChatServerCpu = 31;
constexpr unsigned char MonitorDataTypeChatServerMemory = 32;
constexpr unsigned char MonitorDataTypeChatSession = 33;
constexpr unsigned char MonitorDataTypeChatPlayer = 34;
constexpr unsigned char MonitorDataTypeChatUpdateTps = 35;
constexpr unsigned char MonitorDataTypeChatPacketPool = 36;
constexpr unsigned char MonitorDataTypeChatUpdateMessagePool = 37;