#pragma once

constexpr unsigned short PacketSsMonitorLogin = 20001;
constexpr unsigned short PacketSsMonitorDataUpdate = 20002;

constexpr int MonitorServerTypeLogin = 1;

constexpr unsigned char MonitorDataTypeLoginServerRun = 1;
constexpr unsigned char MonitorDataTypeLoginServerCpu = 2;
constexpr unsigned char MonitorDataTypeLoginServerMemory = 3;
constexpr unsigned char MonitorDataTypeLoginSession = 4;
constexpr unsigned char MonitorDataTypeLoginAuthTps = 5;
constexpr unsigned char MonitorDataTypeLoginPacketPool = 6;