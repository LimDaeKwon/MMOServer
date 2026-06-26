#include "SelectServerPQ.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include "GameDefine.h"
#include "CPacket.h"
#include "Profiler.h"

constexpr int SelectSessionBatchSize = FD_SETSIZE - 1;

SelectServerPQ::SelectServerPQ() :
	gameLoopThread_(nullptr),
	listenSocket_(INVALID_SOCKET),
	sessionId_(1),
	frameMs_(0),
	oldTick_(0),
	maxSessionCount_(DefaultMaxSessionCount),
	packetCode_(PacketCode)
{
	pendingAcceptSessions_.reserve(SelectSessionBatchSize);
}

SelectServerPQ::~SelectServerPQ()
{
}

bool SelectServerPQ::Start(const char* serverIp, unsigned int serverPort, unsigned int nagle, unsigned int maxSessionCount, unsigned char packetCode, unsigned int frameMs)
{
	WSADATA wsa;

	int startupResult = WSAStartup(MAKEWORD(2, 2), &wsa);

	if (startupResult != 0)
	{
		wprintf(L"WSAStartup %d \n", startupResult);
		DebugBreak();
	}
	listenSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (listenSocket_ == INVALID_SOCKET)
	{
		int error = WSAGetLastError();

		wprintf(L"ListenSocket Error %d \n", error);

		DebugBreak();
	}

	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));

	serverAddr.sin_family = AF_INET;
	InetPtonA(AF_INET, serverIp, &serverAddr.sin_addr);
	serverAddr.sin_port = htons(serverPort);

	int bindReturn = bind(listenSocket_, reinterpret_cast<const sockaddr*>(&serverAddr), sizeof(serverAddr));

	if (bindReturn == SOCKET_ERROR)
	{
		bindReturn = WSAGetLastError();

		wprintf(L"bind Error : %d \n", bindReturn);

		DebugBreak();
	}

	LINGER linger;
	linger.l_linger = 0;
	linger.l_onoff = 1;

	int socketOption = setsockopt(listenSocket_, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char*>(&linger), sizeof(linger));

	if (socketOption == SOCKET_ERROR)
	{
		int error = WSAGetLastError();

		printf("setsockopt Error %d ", error);

		DebugBreak();
	}

	DWORD noDelay = 0;

	if (nagle == 0)
	{
		noDelay = 1;
	}

	int noDelayOption = setsockopt(listenSocket_, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&noDelay), sizeof(noDelay));

	if (noDelayOption == SOCKET_ERROR)
	{
		int error = WSAGetLastError();

		printf("setsockopt Error %d ", error);

		DebugBreak();
	}

	int listenReturn = listen(listenSocket_, SOMAXCONN_HINT(7000));

	if (listenReturn == SOCKET_ERROR)
	{
		listenReturn = WSAGetLastError();

		wprintf(L"Listen Error : %d \n", listenReturn);

		DebugBreak();
	}

	u_long on = 1;
	int ioctlSocketError = ioctlsocket(listenSocket_, FIONBIO, &on);

	if (ioctlSocketError == INVALID_SOCKET)
	{
		ioctlSocketError = WSAGetLastError();

		wprintf(L"IoctlSocketError Error : %d \n", ioctlSocketError);

		DebugBreak();
	}

	maxSessionCount_ = maxSessionCount;
	packetCode_ = packetCode;
	frameMs_ = frameMs;
	gameLoopThread_ = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, GameLoopThread, this, 0, nullptr));





	return true;
}

bool SelectServerPQ::Start(const DKServerCore::SelectServerStartConfig& config)
{

	return Start(config.ip.c_str(), config.port, config.nagle, config.maxSessionCount, config.packetCode, config.frameMs);
}

void SelectServerPQ::Network()
{
	SessionPQ* sessionBatch[SelectSessionBatchSize];
	int sessionCount = 0;
	bool selectCalled = false;

	for (auto& sessionPair : sessions_)
	{
		SessionPQ* target = sessionPair.second;

		if (target->isDelete_ == 1)
		{
			continue;
		}

		sessionBatch[sessionCount] = target;
		++sessionCount;

		if (sessionCount == SelectSessionBatchSize)
		{
			ProcessNetworkBatch(sessionBatch, sessionCount);

			sessionCount = 0;
			selectCalled = true;
		}
	}

	if (sessionCount > 0 || selectCalled == false)
	{
		ProcessNetworkBatch(sessionBatch, sessionCount);
	}

	CommitAcceptedClients();
	DeleteDisconnect();

}

void SelectServerPQ::AcceptClient()
{
	int acceptError;
	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(clientAddr);

	SOCKET clientSocket = accept(listenSocket_, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);

	if (clientSocket == INVALID_SOCKET)
	{
		acceptError = WSAGetLastError();

		if (acceptError == WSAEWOULDBLOCK)
		{
			return;
		}

		wprintf(L"Accept Error : %d", acceptError);
		DebugBreak();
		return;
	}

	WCHAR clientIp[16] = { 0 };

	if (InetNtopW(AF_INET, &clientAddr.sin_addr, clientIp, 16) == nullptr)
	{
		wprintf(L"InetNtop Error \n");
		DebugBreak();
	}

	if (sessions_.size() + pendingAcceptSessions_.size() >= maxSessionCount_)
	{
		closesocket(clientSocket);
		return;
	}
	SessionPQ* newSession;
	{
		Profile profile(L"SessionAlloc");
		newSession = new SessionPQ;
	}

	newSession->sessionId_ = sessionId_++;
	newSession->socket_ = clientSocket;
	newSession->isDelete_ = 0;
	newSession->lastRecvTime_ = timeGetTime();

	pendingAcceptSessions_.push_back(newSession);
}

void SelectServerPQ::CommitAcceptedClients()
{
	for (SessionPQ* newSession : pendingAcceptSessions_)
	{
		sessions_.insert(std::unordered_map<SessionId, SessionPQ*>::value_type(newSession->sessionId_, newSession));

		OnAccept(newSession->sessionId_);
	}

	pendingAcceptSessions_.clear();
}

void SelectServerPQ::SendPacket(SessionId sessionId, CPacket* packet)
{
	Profile profile(L"SendPacket");
	SessionPQ* target = sessions_.at(sessionId);

	if (target->isDelete_)
	{
		return;
	}

	//PacketHeader header;
	//header.byCode_ = packetCode_;
	//header.bySize_ = static_cast<unsigned char>(packet->GetDataSize() - sizeof(header.byType_));
	//header.byType_ = *reinterpret_cast<unsigned char*>(packet->GetBufferPtr() + LibraryHeaderSize);

	char* sendStart = packet->GetBufferPtr() + LibraryHeaderSize - 2;

	sendStart[0] = packetCode_;
	sendStart[1] = static_cast<unsigned char>(packet->GetDataSize() - 1);

	packet->IncreaseRefCount();

	{
		Profile profile(L"SendEnqueue");

		if (target->sendQueue_.Enqueue(packet) == false)
		{
			CPacket::Free(packet);
			Disconnect(target->sessionId_);
			return;
		}
	}

}

void SelectServerPQ::Disconnect(SessionId sessionId)
{
	SessionPQ* target = sessions_.at(sessionId);
	if (target->isDelete_ == 1)
	{
		return;
	}
	target->isDelete_ = 1;
}

void SelectServerPQ::DeleteDisconnect()
{
	Profile profile(L"Release");

	std::unordered_map<SessionId, SessionPQ*>::iterator iter = sessions_.begin();

	while (iter != sessions_.end())
	{
		SessionPQ* session = iter->second;

		if (session->isDelete_ == 0)
		{
			++iter;
			continue;
		}

		SessionId sessionId = session->sessionId_;
		closesocket(session->socket_);
		session->receiveQueue_.ClearBuffer();
		
		CPacket* packet = nullptr;
		while (session->sendQueue_.Dequeue(&packet))
		{
			CPacket::Free(packet);
		}
		session->sendQueue_.ClearBuffer();
		OnRelease(sessionId);
		
		{
			Profile profile(L"SessionDelete");
			delete session;
		}
		
		iter = sessions_.erase(iter);
	}
}

void SelectServerPQ::TimeOut()
{
	unsigned int currentTime = timeGetTime();
	std::unordered_map<SessionId, SessionPQ*>::iterator iter;
	for (iter = sessions_.begin(); iter != sessions_.end(); ++iter)
	{
		SessionPQ* target = iter->second;
		if (target->isDelete_ == 1)
		{
			continue;
		}
		if (currentTime - target->lastRecvTime_ > NetworkPacketRecvTimeout)
		{
			Disconnect(target->sessionId_);
		}
	}
}

void SelectServerPQ::ProcessNetworkBatch(SessionPQ** sessionBatch, int sessionCount)
{

	fd_set readSet;
	fd_set writeSet;

	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);
	FD_SET(listenSocket_, &readSet);

	for (int i = 0; i < sessionCount; ++i)
	{
		SessionPQ* target = sessionBatch[i];

		FD_SET(target->socket_, &readSet);

		if (target->sendQueue_.GetUseSize() > 0)
		{
			FD_SET(target->socket_, &writeSet);
		}
	}

	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	int selectResult = select(0, &readSet, &writeSet, nullptr, &timeout);

	if (selectResult == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		wprintf(L"Select Error : %d\n", error);
		DebugBreak();
		return;
	}

	if (selectResult == 0)
	{
		return;
	}

	if (FD_ISSET(listenSocket_, &readSet))
	{
		AcceptClient();
	}

	for (int i = 0; i < sessionCount; ++i)
	{
		SessionPQ* target = sessionBatch[i];

		if (target->isDelete_ == 1)
		{
			continue;
		}

		if (FD_ISSET(target->socket_, &readSet))
		{
			Receive(target);
		}

		if (target->isDelete_ == 1)
		{
			continue;
		}

		if (FD_ISSET(target->socket_, &writeSet))
		{

			Profile profile(L"SendAll");
			SendAll(target);

		}
	}

}

void SelectServerPQ::Receive(SessionPQ* target)
{

	int directEnqueueSize = target->receiveQueue_.DirectEnqueueSize();

	int recvError;
	int recvReturn = recv(target->socket_, target->receiveQueue_.GetRearBufferPtr(), directEnqueueSize, 0);

	if (recvReturn == SOCKET_ERROR)
	{
		recvError = WSAGetLastError();

		if (recvError != WSAEWOULDBLOCK)
		{
			Disconnect(target->sessionId_);
			return;
		}
	}

	if (recvReturn == 0)
	{
		Disconnect(target->sessionId_);
		return;
	}

	target->receiveQueue_.MoveRear(recvReturn);

	unsigned int receiveQueueSize;

	if (recvReturn > 0)
	{
		while (true)
		{
			receiveQueueSize = target->receiveQueue_.GetUseSize();

			if (receiveQueueSize == 0)
			{
				break;
			}

			PacketHeader header;

			if (receiveQueueSize < sizeof(PacketHeader))
			{
				break;
			}

			if (target->receiveQueue_.Peek(reinterpret_cast<char*>(&header), sizeof(header)) != sizeof(header))
			{
				break;
			}

			if (header.byCode_ != packetCode_)
			{
				wprintf(L"Header.byCode_ != PacketCode\n");

				Disconnect(target->sessionId_);
				break;
			}

			if (receiveQueueSize < sizeof(header) + header.bySize_)
			{
				break;
			}

			target->receiveQueue_.MoveFront(sizeof(header));

			CPacket* packetBuffer = CPacket::Alloc();

			unsigned int receiveQueueDequeuePacketSize = target->receiveQueue_.Dequeue(packetBuffer->GetBufferPtr() + LibraryHeaderSize, header.bySize_);

			if (receiveQueueDequeuePacketSize != header.bySize_)
			{
				wprintf(L"## ReceiveQDequeuePacketSize != header.bySize_ : %d \n", receiveQueueDequeuePacketSize);

				CPacket::Free(packetBuffer);
				Disconnect(target->sessionId_);
				break;
			}

			packetBuffer->MoveWritePosition(receiveQueueDequeuePacketSize);

			target->lastRecvTime_ = timeGetTime();

			OnMessage(target->sessionId_, header.byType_, packetBuffer);
			CPacket::Free(packetBuffer);
		}
	}


}

//void SelectServer::SendAll(Session* target)
//{
//    int directDequeueSize = target->sendQueue_.DirectDequeueSize();
//
//    int sendReturn = send(target->socket_, target->sendQueue_.GetFrontBufferPtr(), directDequeueSize, 0);
//
//    if (sendReturn == SOCKET_ERROR)
//    {
//        int error = WSAGetLastError();
//
//        if (error != WSAEWOULDBLOCK)
//        {
//            Disconnect(target->sessionId_);
//            return;
//        }
//    }
//
//    if (sendReturn != directDequeueSize)
//    {
//        wprintf(L"SendSize : %d , SendReturn : %d \n", directDequeueSize, sendReturn);
//
//        Disconnect(target->sessionId_);
//        return;
//    }
//
//    int moveFrontSize = target->sendQueue_.MoveFront(directDequeueSize);
//
//    if (moveFrontSize != directDequeueSize)
//    {
//        wprintf(L"MoveFrontSize : %d , DirectDequeueSize : %d \n", moveFrontSize, directDequeueSize);
//        DebugBreak();
//    }
//}

void SelectServerPQ::SendAll(SessionPQ* target)
{
	CPacket* sendPackets[DKServerCore::MaxBatchSize];
	WSABUF buffers[DKServerCore::MaxBatchSize];

	DWORD bufferCount = 0;
	DWORD sendTotal = 0;

	while (bufferCount < DKServerCore::MaxBatchSize)
	{
		CPacket* packet = nullptr;

		if (target->sendQueue_.Dequeue(&packet) == false)
		{
			break;
		}

		char* sendStart = packet->GetBufferPtr() + LibraryHeaderSize - 2;
		DWORD sendSize = static_cast<DWORD>(packet->GetDataSize() + 2);

		sendPackets[bufferCount] = packet;
		buffers[bufferCount].buf = sendStart;
		buffers[bufferCount].len = sendSize;

		sendTotal += sendSize;
		++bufferCount;
	}

	if (bufferCount == 0)
	{
		return;
	}

	DWORD sentBytes = 0;
	int sendResult;

	{
		Profile profile(L"WSASend");
		sendResult = WSASend(target->socket_, buffers, bufferCount, &sentBytes, 0, nullptr, nullptr);
	}

	for (DWORD index = 0; index < bufferCount; ++index)
	{
		CPacket::Free(sendPackets[index]);
	}

	if (sendResult == SOCKET_ERROR)
	{
		Disconnect(target->sessionId_);
		return;
	}

	if (sentBytes != sendTotal)
	{
		Disconnect(target->sessionId_);
		return;
	}
}


bool SelectServerPQ::TryUpdate()
{

	DWORD tick = timeGetTime();

	unsigned int frame = tick - oldTick_;
	if (frame > frameMs_)
	{
		TimeOut();
		unsigned int fixUpdate = (frame / 40);

		for (unsigned int i = 0; i < fixUpdate; ++i)
		{
			OnUpdate();
		}

		oldTick_ += (frameMs_ * (frame / frameMs_));
	}

	return false;
}

void SelectServerPQ::InitOldTick()
{
	oldTick_ = timeGetTime();
}

void SelectServerPQ::SetProfileEnabled()
{
	if (sessions_.size() >= 12000)
	{
		SetEnabled(true);
	}
	else
	{
		SetEnabled(false);
	}
}

unsigned int __stdcall SelectServerPQ::GameLoopThread(void* thisPointer)
{
	SelectServerPQ* thisForGameLoop = static_cast<SelectServerPQ*>(thisPointer);
	thisForGameLoop->InitOldTick();

	while (true)
	{
		thisForGameLoop->SetProfileEnabled();
		Profile profile(L"Frame");
		{
			Profile a(L"Network");
			thisForGameLoop->Network();
		}

		thisForGameLoop->TryUpdate();

		//ServerControl();

	}


}
