#include "SelectServer.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include "GameDefine.h"
#include "CPacket.h"
#include "Profiler.h"

constexpr int SelectSessionBatchSize = FD_SETSIZE - 1;

SelectServer::SelectServer() :
	gameLoopThread_(nullptr),
	listenSocket_(INVALID_SOCKET),
	sessionFreeList_(DefaultMaxSessionCount),
	sessionId_(1),
	frameMs_(0),
	oldTick_(0),
	maxSessionCount_(DefaultMaxSessionCount),
	packetCode_(PacketCode)
{
	pendingAcceptSessions_.reserve(SelectSessionBatchSize);
}

SelectServer::~SelectServer()
{
}

bool SelectServer::Start(const char* serverIp, unsigned int serverPort, unsigned int nagle, unsigned int maxSessionCount, unsigned char packetCode, unsigned int frameMs)
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

bool SelectServer::Start(const DKServerCore::SelectServerStartConfig& config)
{
	return Start(config.ip.c_str(), config.port, config.nagle, config.maxSessionCount, config.packetCode, config.frameMs);
}

void SelectServer::Network()
{
	Session* sessionBatch[SelectSessionBatchSize];
	int sessionCount = 0;
	bool selectCalled = false;

	for (auto& sessionPair : sessions_)
	{
		Session* target = sessionPair.second;

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

void SelectServer::AcceptClient()
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
	Session* newSession;
	{
		Profile profile(L"SessionAlloc");
		newSession = sessionFreeList_.Alloc();
	}
	

	newSession->sessionId_ = sessionId_++;
	newSession->socket_ = clientSocket;
	newSession->isDelete_ = 0;
	newSession->lastRecvTime_ = timeGetTime();

	pendingAcceptSessions_.push_back(newSession);
}

void SelectServer::CommitAcceptedClients()
{
	for (Session* newSession : pendingAcceptSessions_)
	{
		sessions_.insert(std::unordered_map<SessionId, Session*>::value_type(newSession->sessionId_, newSession));

		OnAccept(newSession->sessionId_);
		//++acceptCount_;
	}

	pendingAcceptSessions_.clear();
}

void SelectServer::SendPacket(SessionId sessionId, CPacket* packet)
{
	Profile profile(L"SendPacket");
	Session* target = sessions_.at(sessionId);

	if (target->isDelete_)
	{
		return;
	}

	PacketHeader header;
	header.byCode_ = packetCode_;
	header.bySize_ = static_cast<unsigned char>(packet->GetDataSize() - sizeof(header.byType_));
	header.byType_ = *reinterpret_cast<unsigned char*>(packet->GetBufferPtr() + LibraryHeaderSize);

	int dataSize = packet->GetDataSize() - sizeof(header.byType_);

	if (target->sendQueue_.GetFreeSize() < sizeof(header) + dataSize)
	{
		wprintf(L"EnqueueFail in SendPacketUnicast %lld \n ", target->sessionId_);

		Disconnect(target->sessionId_);
		return;
	}

	{
		Profile profile(L"SendEnqueue");
		int enqueueHeaderReturn = target->sendQueue_.Enqueue(reinterpret_cast<char*>(&header), sizeof(header));

		if (enqueueHeaderReturn != sizeof(header))
		{
			wprintf(L"EnqueueFail in SendPacketUnicast %lld \n ", target->sessionId_);

			Disconnect(target->sessionId_);
			return;
		}

		int enqueuePacketReturn = target->sendQueue_.Enqueue(packet->GetBufferPtr() + LibraryHeaderSize + sizeof(header.byType_), dataSize);

		if (enqueuePacketReturn != dataSize)
		{
			wprintf(L"EnqueueFail in SendPacketUnicast %lld \n ", target->sessionId_);

			Disconnect(target->sessionId_);
		}

		//++sendPacketCount_;

	}
	
}

void SelectServer::Disconnect(SessionId sessionId)
{
	Session* target = sessions_.at(sessionId);
	if (target->isDelete_ == 1)
	{
		return;
	}
	freeSessionStack_.push(target);
	target->isDelete_ = 1;
	//++disconnectCount_;
}

unsigned int SelectServer::GetSessionCount() const
{
	return static_cast<unsigned int>(sessions_.size());
}

unsigned int SelectServer::GetAcceptTPS() const
{
	return acceptTPS_;
}

unsigned int SelectServer::GetRecvPacketTPS() const
{
	return recvPacketTPS_;
}

unsigned int SelectServer::GetSendPacketTPS() const
{
	return sendPacketTPS_;
}

unsigned int SelectServer::GetSendCompleteTPS() const
{
	return sendCompleteTPS_;
}

unsigned int SelectServer::GetDisconnectTPS() const
{
	return disconnectTPS_;
}

unsigned int SelectServer::GetReleaseTPS() const
{
	return releaseTPS_;
}

unsigned int SelectServer::GetFrameTPS() const
{
	return frameTPS_;
}

void SelectServer::DeleteDisconnect()
{
	size_t deleteCount = freeSessionStack_.size();
	
	if (deleteCount)
	{
		Profile profile(L"Release");

		for (int i = 0; i < deleteCount; ++i)
		{
			Session* session = freeSessionStack_.top();
			freeSessionStack_.pop();
			closesocket(session->socket_);
			session->receiveQueue_.ClearBuffer();
			session->sendQueue_.ClearBuffer();
			OnRelease(session->sessionId_);
			{
				Profile profile(L"SessionDelete");
				sessionFreeList_.Free(session);
			}
			sessions_.erase(session->sessionId_);
			//++releaseCount_;
		}
	}
}

void SelectServer::TimeOut()
{
	unsigned int currentTime = timeGetTime();
	std::unordered_map<SessionId, Session*>::iterator iter;
	for (iter = sessions_.begin(); iter != sessions_.end(); ++iter)
	{
		Session* target = iter->second;
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

void SelectServer::ProcessNetworkBatch(Session** sessionBatch, int sessionCount)
{

	fd_set readSet;
	fd_set writeSet;

	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);
	FD_SET(listenSocket_, &readSet);

	for (int i = 0; i < sessionCount; ++i)
	{
		Session* target = sessionBatch[i];

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
		Session* target = sessionBatch[i];

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

void SelectServer::Receive(Session* target)
{

	int directEnqueueSize = target->receiveQueue_.DirectEnqueueSize();

	int recvError;
	int recvReturn = recv(target->socket_, target->receiveQueue_.GetRearBufferPtr(), directEnqueueSize, 0);

	if (recvReturn == SOCKET_ERROR)
	{
		recvError = WSAGetLastError();

		if (recvError == WSAEWOULDBLOCK)
		{
			
			return;
		}
		Disconnect(target->sessionId_);
		return;
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
			//++recvPacketCount_;
		}
	}


}

void SelectServer::SendAll(Session* target)
{

	int useSize = target->sendQueue_.GetUseSize();
	int firstSize = target->sendQueue_.DirectDequeueSize();
	int secondSize = useSize - firstSize;

	WSABUF buffers[2];

	buffers[0].buf = target->sendQueue_.GetFrontBufferPtr();
	buffers[0].len = firstSize;

	buffers[1].buf = target->sendQueue_.GetStartBufferPtr();
	buffers[1].len = secondSize;

	DWORD sentBytes = 0;
	DWORD bufferCount = 1;
	if (secondSize > 0)
	{
		bufferCount = 2;
	}
	
	{
		Profile profile(L"WSASend");
		int result = WSASend(target->socket_, buffers, bufferCount, &sentBytes, 0, nullptr, nullptr);

		if (result == SOCKET_ERROR)
		{
			int error = WSAGetLastError();

			if (error == WSAEWOULDBLOCK)
			{
				return;
			}

			Disconnect(target->sessionId_);
			return;
		}
	}
	

	target->sendQueue_.MoveFront(sentBytes);
	//++sendCompleteCount_;
}


bool SelectServer::TryUpdate()
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

void SelectServer::InitOldTick()
{
	oldTick_ = timeGetTime();
}

void SelectServer::SetProfileEnabled()
{
	if (sessions_.size() >= 9000)
	{
		SetEnabled(true);
	}
	else
	{
		SetEnabled(false);
	}
}

unsigned int __stdcall SelectServer::GameLoopThread(void* thisPointer)
{
	SelectServer* thisForGameLoop = static_cast<SelectServer*>(thisPointer);
	thisForGameLoop->InitOldTick();
	SetEnabled(true);
	{ Profile profile(L"SendPacket"); }
	{ Profile profile(L"SendEnqueue"); }
	{ Profile profile(L"SendAll"); }
	{ Profile profile(L"WSASend"); }

	{ Profile profile(L"GameRun"); }
	{ Profile profile(L"SendPacketToSectors"); }
	{ Profile profile(L"SendPacketAround"); }
	{ Profile profile(L"SectorUpdate"); }
	{ Profile profile(L"GetUpdateSectorAround"); }

	{ Profile profile(L"OnMessage"); }
	{ Profile profile(L"NetPacketProcMoveStop"); }
	{ Profile profile(L"NetPacketProcMoveStart"); }
	{ Profile profile(L"NetPacketProcAttack"); }
	{ Profile profile(L"HitCheck"); }
	{ Profile profile(L"NetPacketEcho"); }

	{ Profile profile(L"SessionAlloc"); }
	{ Profile profile(L"OnAccept"); }
	{ Profile profile(L"SessionDelete"); }
	{ Profile profile(L"OnRelease"); }
	{ Profile profile(L"Release"); }

	{ Profile profile(L"OnUpdate"); }
	{ Profile profile(L"Network"); }
	{ Profile profile(L"Frame"); }
	SetEnabled(false);



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

	/*	++thisForGameLoop->frameCount_;

		DWORD now = timeGetTime();
		if (now - thisForGameLoop->lastMonitorTick_ >= 1000)
		{
			thisForGameLoop->acceptTPS_ = thisForGameLoop->acceptCount_;
			thisForGameLoop->recvPacketTPS_ = thisForGameLoop->recvPacketCount_;
			thisForGameLoop->sendPacketTPS_ = thisForGameLoop->sendPacketCount_;
			thisForGameLoop->sendCompleteTPS_ = thisForGameLoop->sendCompleteCount_;
			thisForGameLoop->disconnectTPS_ = thisForGameLoop->disconnectCount_;
			thisForGameLoop->releaseTPS_ = thisForGameLoop->releaseCount_;
			thisForGameLoop->frameTPS_ = thisForGameLoop->frameCount_;

			thisForGameLoop->acceptCount_ = 0;
			thisForGameLoop->recvPacketCount_ = 0;
			thisForGameLoop->sendPacketCount_ = 0;
			thisForGameLoop->sendCompleteCount_ = 0;
			thisForGameLoop->disconnectCount_ = 0;
			thisForGameLoop->releaseCount_ = 0;
			thisForGameLoop->frameCount_ = 0;

			thisForGameLoop->lastMonitorTick_ = now;
		}*/

	}


}
