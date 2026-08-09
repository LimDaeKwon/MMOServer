#pragma once


// Client -> Server
constexpr unsigned short PacketCsChatServer = 0;
constexpr unsigned short PacketCsChatReqLogin = 1;
constexpr unsigned short PacketCsChatReqSectorMove = 3;
constexpr unsigned short PacketCsChatReqMessage = 5;
constexpr unsigned short PacketCsChatReqHeartbeat = 7;

// Server -> Client
constexpr unsigned short PacketScChatResLogin = 2;
constexpr unsigned short PacketScChatResSectorMove = 4;
constexpr unsigned short PacketScChatResMessage = 6;

constexpr int PacketChatLoginRequestDataSize = 152;
constexpr int PacketChatSectorMoveRequestDataSize = 12;
constexpr int PacketChatMessageRequestMaxDataSize = 116;
constexpr int PacketChatMessageMaxByteSize = 106;