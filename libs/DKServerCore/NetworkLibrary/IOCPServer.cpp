#include "IOCPServer.h"

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#include <iostream>
#include <process.h>
#include <stdlib.h>
#include <unordered_map>

#include "Profiler.h"
#include "CPacket.h"
#include "CPacketQueue.h"
#include "ContentsCPacket.h"

#pragma comment(lib, "ws2_32.lib")

unsigned int __stdcall IOCPServer::AcceptThread(void* thisPointer)
{
	IOCPServer* thisForAccept = static_cast<IOCPServer*>(thisPointer);

	while (true)
	{
		SOCKET clientSock = accept(thisForAccept->listenSock_, nullptr, nullptr);

		if (clientSock == INVALID_SOCKET)
		{
			int error = WSAGetLastError();

			if (error == 10004)
			{
				break;
			}

			wprintf(L"accept Error %d ", error);
			DebugBreak();
		}

		Session* newSession = thisForAccept->SessionAlloc(thisForAccept->FindEmptySession(), clientSock);

		InterlockedIncrement(&thisForAccept->acceptCount_);
		thisForAccept->RegisterIOCP(reinterpret_cast<HANDLE>(clientSock), reinterpret_cast<ULONG_PTR>(newSession));

		sockaddr_in clientAddr;
		WCHAR addr[INET_ADDRSTRLEN];

		thisForAccept->GetClientAddress(clientSock, clientAddr, addr);
		thisForAccept->OnAccept(addr, ntohs(clientAddr.sin_port), newSession->sessionId_);

		thisForAccept->ReceiveFirst(newSession);
	}

	return 0;
}

unsigned int __stdcall IOCPServer::WorkerThread(void* thisPointer)
{
	IOCPServer* thisForWorker = static_cast<IOCPServer*>(thisPointer);

	while (true)
	{
		DWORD cbTransferred = 0;
		MyOverlapped* overlapPointer = nullptr;
		Session* target = nullptr;

		int retval = GetQueuedCompletionStatus(thisForWorker->handleIocp_, &cbTransferred, reinterpret_cast<PULONG_PTR>(&target), reinterpret_cast<LPOVERLAPPED*>(&overlapPointer), INFINITE);

		if (overlapPointer == nullptr && cbTransferred == 0 && target == nullptr)
		{
			PostQueuedCompletionStatus(thisForWorker->handleIocp_, 0, 0, nullptr);
			break;
		}

		if (retval == 0)
		{
			int error = WSAGetLastError();

			if (!(error == 64 || error == 995 || error == 1236))
			{
				__debugbreak();
			}
		}
		else
		{
			if (overlapPointer == nullptr && cbTransferred == 0 && target != nullptr)
			{
				thisForWorker->SendPost(target);
				continue;
			}
			else if (overlapPointer->type_ == DKServerCore::RecvIoType)
			{
				thisForWorker->RecvCompletion(target, cbTransferred);
			}
			else if (overlapPointer->type_ == DKServerCore::SendIoType)
			{
				thisForWorker->SendCompletion(target);
			}
		}

		thisForWorker->ReturnReference(target);
	}

	return 0;
}

unsigned int __stdcall IOCPServer::MonitorThread(void* thisPointer)
{
	IOCPServer* thisForMonitor = static_cast<IOCPServer*>(thisPointer);

	while (true)
	{
		InterlockedExchange(&thisForMonitor->acceptCount_, 0);
		InterlockedExchange(&thisForMonitor->recvMessageCount_, 0);
		InterlockedExchange(&thisForMonitor->sendMessageCount_, 0);

		Sleep(1000);
	}

	return 0;
}

unsigned int __stdcall IOCPServer::HeartbeatThread(void* thisPointer)
{
	IOCPServer* thisForHeartbeat = static_cast<IOCPServer*>(thisPointer);

	unsigned int localCount = 0;

	while (true)
	{
		if (localCount % thisForHeartbeat->unloginTimeout_ == 0)
		{
			printf("Heartbeat Check \n");

			for (unsigned int i = 0; i < thisForHeartbeat->maxSession_; ++i)
			{
				Session* target = &thisForHeartbeat->sessionArray_[i];

				if (target->useFlag_ == 0)
				{
					continue;
				}

				if (target->loginFlag_ == 1)
				{
					continue;
				}

				if (GetTickCount64() - target->lastRecvTime_ > thisForHeartbeat->unloginTimeout_ * 10000)
				{
					thisForHeartbeat->Disconnect(target->sessionId_);
				}
			}
		}

		if (localCount % thisForHeartbeat->timeout_ == 0)
		{
			printf("Heartbeat Check \n");

			for (unsigned int i = 0; i < thisForHeartbeat->maxSession_; ++i)
			{
				Session* target = &thisForHeartbeat->sessionArray_[i];

				if (target->useFlag_ == 0)
				{
					continue;
				}

				if (target->loginFlag_ == 0)
				{
					continue;
				}

				if (GetTickCount64() - target->lastRecvTime_ > thisForHeartbeat->timeout_ * 1000)
				{
					thisForHeartbeat->Disconnect(target->sessionId_);
				}
			}
		}

		localCount++;
		Sleep(1000);
	}

	return 0;
}

IOCPServer::IOCPServer()
	: acceptTps_(0)
	, recvMessageTps_(0)
	, sendMessageTps_(0)
	, acceptCount_(0)
	, recvMessageCount_(0)
	, sendMessageCount_(0)
	, maxSession_(0)
	, sessionNum_(0)
	, threadsNum_(0)
	, uniqueId_(0)
	, indexList_(0, false)
{
}

IOCPServer::~IOCPServer()
{
}

bool IOCPServer::Start(const char* serverIp, unsigned int serverPort, unsigned int workerNum, unsigned int concurrentThreads, unsigned int nagle, unsigned int sessions, unsigned int header)
{
	maxSession_ = sessions;
	sessionNum_ = 0;
	sessionArray_ = new Session[maxSession_];
	headerSize_ = sizeof(PacketHeader);
	UNREFERENCED_PARAMETER(header);

	int** temp = new int* [maxSession_];

	timeout_ = 30;
	unloginTimeout_ = 3;

	for (unsigned int i = 0; i < maxSession_; ++i)
	{
		temp[i] = indexList_.Alloc();
		*temp[i] = i;
	}

	for (unsigned int i = 0; i < maxSession_; ++i)
	{
		indexList_.Free(temp[i]);
	}

	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		DebugBreak();
	}

	handleIocp_ = CreateIOCP(concurrentThreads);

	if (handleIocp_ == nullptr)
	{
		DebugBreak();
	}

	listenSock_ = socket(AF_INET, SOCK_STREAM, 0);

	if (listenSock_ == INVALID_SOCKET)
	{
		int error = WSAGetLastError();
		wprintf(L"ListenSocket Error %d \n", error);
		DebugBreak();
	}

	SOCKADDR_IN serverAddress;
	ZeroMemory(&serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	InetPtonA(AF_INET, serverIp, &serverAddress.sin_addr);
	serverAddress.sin_port = htons(serverPort);

	int bindReturn = bind(listenSock_, reinterpret_cast<const sockaddr*>(&serverAddress), sizeof(serverAddress));

	if (bindReturn == SOCKET_ERROR)
	{
		bindReturn = WSAGetLastError();
		wprintf(L"BindReturn Error : %d \n", bindReturn);
		DebugBreak();
	}

	LINGER linger;
	linger.l_linger = 0;
	linger.l_onoff = 1;

	int socketOption = setsockopt(listenSock_, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char*>(&linger), sizeof(linger));

	if (socketOption == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		printf("setsockopt Error %d ", error);
		DebugBreak();
	}

	if (nagle)
	{
		DWORD noDelay = 1;

		int noDelayOption = setsockopt(listenSock_, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&noDelay), sizeof(noDelay));

		if (noDelayOption == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			printf("setsockopt Error %d ", error);
			DebugBreak();
		}
	}

	int listenReturn = listen(listenSock_, SOMAXCONN_HINT(7000));

	if (listenReturn == SOCKET_ERROR)
	{
		listenReturn = WSAGetLastError();
		wprintf(L"Listen Error : %d \n", listenReturn);
		DebugBreak();
	}

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);

	threadsNum_ = workerNum;
	threads_ = new HANDLE[threadsNum_];

	for (unsigned int i = 0; i < threadsNum_; ++i)
	{
		threads_[i] = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, WorkerThread, this, 0, nullptr));

		if (threads_[i] == nullptr)
		{
			wprintf(L"_beginthreadex Failed \n");
			DebugBreak();
		}
	}

	acceptThread_ = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, AcceptThread, this, 0, nullptr));

	if (acceptThread_ == nullptr)
	{
		wprintf(L"_beginthreadex Failed \n");
		DebugBreak();
	}

	monitorThread_ = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, MonitorThread, this, 0, nullptr));

	if (monitorThread_ == nullptr)
	{
		wprintf(L"_beginthreadex Failed \n");
		DebugBreak();
	}

	heartbeatThread_ = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, HeartbeatThread, this, 0, nullptr));

	if (heartbeatThread_ == nullptr)
	{
		wprintf(L"_beginthreadex Failed \n");
		DebugBreak();
	}

	return true;
}

bool IOCPServer::Stop()
{
	PostQueuedCompletionStatus(handleIocp_, 0, 0, nullptr);

	WaitForMultipleObjects(threadsNum_, threads_, TRUE, INFINITE);

	closesocket(listenSock_);

	WaitForSingleObject(acceptThread_, INFINITE);

	return true;
}

int IOCPServer::GetSessionCount()
{
	return sessionNum_;
}

void IOCPServer::Disconnect(__int64 sessionId)
{
	Session* target;
	unsigned int i = FindSession(sessionId);

	target = &sessionArray_[i];

	int localCount = InterlockedIncrement(&target->ioCount_);

	if ((localCount & DKServerCore::ReleaseFlag) == DKServerCore::ReleaseFlag)
	{
		ReturnReference(target);
		return;
	}

	if (target->sessionId_ != sessionId)
	{
		ReturnReference(target);
		return;
	}

	if (InterlockedExchange8(reinterpret_cast<volatile char*>(&target->disconnectFlag_), 1) == 1)
	{
		ReturnReference(target);
		return;
	}

	CancelIoEx(reinterpret_cast<HANDLE>(target->sock_), nullptr);

	ReturnReference(target);

}

IOCPServer::Session* IOCPServer::SessionAlloc(int* emptyIndex, unsigned long long clientSock)
{

	if (InterlockedIncrement(&sessionNum_) > maxSession_)
	{
		return nullptr;
	}

	Session* newSession = &sessionArray_[*emptyIndex];

	newSession->index_ = emptyIndex;


	__int64 i = *newSession->index_;

	newSession->sessionId_ = ++uniqueId_;
	newSession->sessionId_ |= (i << 48);
	newSession->bufferCount_.count_ = 0;
	newSession->sock_ = static_cast<SOCKET>(clientSock);
	newSession->sendFlag_ = false;
	newSession->disconnectFlag_ = false;
	newSession->useFlag_ = true;
	newSession->loginFlag_ = false;
	newSession->lastRecvTime_ = GetTickCount64();

	InterlockedIncrement(&newSession->ioCount_);
	InterlockedAnd(&newSession->ioCount_, 0x7fffffff);

	newSession->recvOverlapped_.type_ = DKServerCore::RecvIoType;
	newSession->sendOverlapped_.type_ = DKServerCore::SendIoType;

	return newSession;
}

void IOCPServer::SendCompletion(Session* target)
{
	for (int i = 0; i < target->bufferCount_.count_; ++i)
	{
		CPacket::Free(target->bufferCount_.buffers_[i]);
	}

	target->bufferCount_.count_ = 0;
	InterlockedExchange8(reinterpret_cast<volatile char*>(&target->sendFlag_), 0);
	SendPost(target);
}

void IOCPServer::RecvCompletion(Session* target, DWORD cbTransferred)
{
	target->loginFlag_ = true;
	target->recvBuffer_.MoveRear(cbTransferred);
	target->lastRecvTime_ = GetTickCount64();
	RecvProc(target);
	Receive(target);
}

void IOCPServer::SendPacket(__int64 sessionId, CPacket* sendPacket)
{
	Session* target;
	unsigned int i = FindSession(sessionId);

	target = &sessionArray_[i];

	int localCount = InterlockedIncrement(&target->ioCount_);

	if ((localCount & DKServerCore::ReleaseFlag) == DKServerCore::ReleaseFlag)
	{
		ReturnReference(target);
		return;
	}

	if (target->sessionId_ != sessionId)
	{
		ReturnReference(target);
		return;
	}

	AddHeader(sendPacket);

	sendPacket->IncreaseRefCount();

	int enqueueReturn = target->sendBuffer_.Enqueue(sendPacket);

	InterlockedIncrement(&target->sendCount_);

	if (enqueueReturn == false)
	{
		CPacket::Free(sendPacket);
		ReturnReference(target);
		wprintf(L"EnqueueFail in SendPacketUnicast Session Id : %lld\n", target->sessionId_);

		return;
	}

	PostQueuedCompletionStatus(handleIocp_, 0, reinterpret_cast<ULONG_PTR>(target), nullptr);
	ReturnReference(target);
}

void IOCPServer::SendPost(Session* target)
{
	if (InterlockedOr8(reinterpret_cast<volatile char*>(&target->disconnectFlag_), 0) == 1)
	{
		return;
	}

	long localCount = InterlockedIncrement(&target->ioCount_);

	if ((localCount & DKServerCore::ReleaseFlag) == DKServerCore::ReleaseFlag)
	{
		if (InterlockedDecrement(&target->ioCount_) == 0)
		{
			Release(target);
		}

		return;
	}


	if (InterlockedExchange8(reinterpret_cast<volatile char*>(&target->sendFlag_), 1) == 0)
	{
		WSABUF localWsaBuf[DKServerCore::LanNetworkMaxBatchSize * 2];

		int bufCount = SetWSABUF(target, localWsaBuf);

		if (!bufCount)
		{
			RecursiveCheck(target);
			return;
		}

		DWORD sendBytes = 0;

		InterlockedIncrement(&target->ioCount_);
		ZeroMemory(&target->sendOverlapped_.overlapped_, sizeof(target->sendOverlapped_.overlapped_));

		int wsaSendReturn = WSASend(target->sock_, localWsaBuf, bufCount, &sendBytes, 0, &target->sendOverlapped_.overlapped_, nullptr);

		CheckSendReturn(target, wsaSendReturn);
	}
}

void IOCPServer::ReceiveFirst(Session* newSession)
{
	WSABUF wsaBuf;
	wsaBuf.buf = newSession->recvBuffer_.GetRearBufferPtr();
	wsaBuf.len = newSession->recvBuffer_.GetFreeSize();

	DWORD recvBytes;
	DWORD flags = 0;

	ZeroMemory(&newSession->recvOverlapped_.overlapped_, sizeof(newSession->recvOverlapped_.overlapped_));

	int retval = WSARecv(newSession->sock_, &wsaBuf, 1, &recvBytes, &flags, &newSession->recvOverlapped_.overlapped_, 0);

	if (retval == SOCKET_ERROR)
	{
		int wsaRecvError = WSAGetLastError();

		if (wsaRecvError != WSA_IO_PENDING)
		{
			wprintf(L"In First WSARecvError : %d  , Session ID : %lld\n", wsaRecvError, newSession->sessionId_);

			ReturnReference(newSession);
		}
	}
}

void IOCPServer::RecvProc(Session* target)
{
	while (true)
	{
		int targetRecvBufferSize = target->recvBuffer_.GetUseSize();
		PacketHeader header;

		if (targetRecvBufferSize < sizeof(header))
		{
			break;
		}

		if (target->recvBuffer_.Peek(reinterpret_cast<char*>(&header), sizeof(header)) != sizeof(header))
		{
			break;
		}

		if (CheckLibraryPacketCode(header.code_) == false)
		{
			Disconnect(target->sessionId_);
			break;
		}

		if (targetRecvBufferSize < sizeof(header) + header.size_)
		{
			break;
		}

		target->recvBuffer_.MoveFront(sizeof(header));

		CPacket* packetBuffer = CPacket::Alloc();

		unsigned int receiveDequeuePacketSize = target->recvBuffer_.Dequeue(packetBuffer->GetBufferPtr() + DKServerCore::PacketLibHeaderSize, header.size_);

		if (receiveDequeuePacketSize != header.size_)
		{
			wprintf(L"## ReceiveQDequeuePacketSize != Header.BySize : %d \n", receiveDequeuePacketSize);

			CPacket::Free(packetBuffer);
			break;
		}

		packetBuffer->MoveWritePosition(receiveDequeuePacketSize);

		OnMessage(target->sessionId_, header.type_, packetBuffer);

		CPacket::Free(packetBuffer);
	}

}

void IOCPServer::Receive(Session* target)
{
	if (target->disconnectFlag_ == 1)
	{
		return;
	}

	WSABUF recvWsaBuf[2];

	recvWsaBuf[0].buf = target->recvBuffer_.GetRearBufferPtr();
	recvWsaBuf[0].len = target->recvBuffer_.DirectEnqueueSize();
	recvWsaBuf[1].buf = target->recvBuffer_.GetStartBufferPtr();
	recvWsaBuf[1].len = target->recvBuffer_.GetFreeSize() - target->recvBuffer_.DirectEnqueueSize();

	DWORD recvBytes;
	DWORD flags = 0;

	ZeroMemory(&target->recvOverlapped_.overlapped_, sizeof(target->recvOverlapped_.overlapped_));
	InterlockedIncrement(&target->ioCount_);

	int retval = WSARecv(target->sock_, recvWsaBuf, 2, &recvBytes, &flags, &target->recvOverlapped_.overlapped_, 0);

	CheckRecvReturn(target, retval);
}

void IOCPServer::AddHeader(CPacket* packetBuffer)
{
	char* temp = packetBuffer->GetBufferPtr();
	temp += DKServerCore::PacketLibHeaderSize - headerSize_;

	PacketHeader packetHeader;
	packetHeader.code_ = DKServerCore::LibraryPacketCode;
	packetHeader.size_ = static_cast<BYTE>(packetBuffer->GetDataSize() - sizeof(packetHeader.type_));
	packetHeader.type_ = *reinterpret_cast<BYTE*>(packetBuffer->GetBufferPtr() + DKServerCore::PacketLibHeaderSize);

	memcpy_s(temp, headerSize_, &packetHeader, sizeof(packetHeader));
}

void IOCPServer::Release(Session* target)
{
	if (InterlockedCompareExchange(&target->ioCount_, DKServerCore::ReleaseFlag, 0) != 0)
	{
		return;
	}

	for (int i = 0; i < target->bufferCount_.count_; ++i)
	{
		CPacket::Free(target->bufferCount_.buffers_[i]);
	}

	target->bufferCount_.count_ = 0;

	target->recvBuffer_.ClearBuffer();
	ClearSendBuffer(target);

	InterlockedExchange8(reinterpret_cast<volatile char*>(&target->useFlag_), 0);
	InterlockedExchange8(reinterpret_cast<volatile char*>(&target->loginFlag_), 0);

	closesocket(target->sock_);
	target->sock_ = INVALID_SOCKET;

	OnRelease(target->sessionId_);

	indexList_.Free(target->index_);
	InterlockedDecrement(&sessionNum_);

	return;
}

void IOCPServer::RegisterIOCP(HANDLE newSocket, ULONG_PTR key)
{
	CreateIoCompletionPort(newSocket, handleIocp_, key, 0);
}

HANDLE IOCPServer::CreateIOCP(DWORD concurrent)
{
	return CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, concurrent);
}

int* IOCPServer::FindEmptySession()
{
	int* tempIndex = indexList_.Alloc();
	return tempIndex;
}

void IOCPServer::ClearSendBuffer(Session* target)
{
	while (true)
	{
		CPacket* temp;

		if (target->sendBuffer_.Dequeue(&temp) == false)
		{
			break;
		}

		CPacket::Free(temp);
	}
}

void IOCPServer::ReturnReference(Session* target)
{
	if (InterlockedDecrement(&target->ioCount_) == 0)
	{
		Release(target);
	}
}

int IOCPServer::SetWSABUF(Session* target, WSABUF* wsaBuf)
{
	int bufCount = 0;

	int packetCount = 0;

	while (packetCount < DKServerCore::LanNetworkMaxBatchSize)
	{
		CPacket* temp = nullptr;

		if (target->sendBuffer_.Dequeue(&temp) == false)
		{
			break;
		}

		target->bufferCount_.buffers_[packetCount] = temp;
		wsaBuf[bufCount].buf = temp->GetBufferPtr() + DKServerCore::PacketLibHeaderSize - headerSize_;
		wsaBuf[bufCount].len = headerSize_;
		bufCount++;
		wsaBuf[bufCount].buf = temp->GetBufferPtr() + DKServerCore::PacketLibHeaderSize + sizeof(BYTE);
		wsaBuf[bufCount].len = temp->GetDataSize() - sizeof(BYTE);
		bufCount++;
		packetCount++;
	}

	target->bufferCount_.count_ = packetCount;

	return bufCount;
}

void IOCPServer::GetClientAddress(SOCKET clientSocket, sockaddr_in& clientAddr, WCHAR* addr)
{
	int addrLen = sizeof(clientAddr);

	getpeername(clientSocket, reinterpret_cast<SOCKADDR*>(&clientAddr), &addrLen);

	if (InetNtopW(AF_INET, &clientAddr.sin_addr, addr, INET_ADDRSTRLEN) == nullptr)
	{
		wprintf(L"InetNtop Error \n");
		DebugBreak();
	}
}

void IOCPServer::RecursiveCheck(Session* target)
{
	InterlockedExchange8(reinterpret_cast<volatile char*>(&target->sendFlag_), 0);

	if (target->sendBuffer_.GetSize() != 0)
	{
		SendPost(target);
	}
}

void IOCPServer::CheckSendReturn(Session* target, int sendReturn)
{
	if (sendReturn == SOCKET_ERROR)
	{
		int wsaSendError = WSAGetLastError();

		if (wsaSendError == WSA_IO_PENDING)
		{
			if (target->disconnectFlag_ == 1)
			{
				CancelIoEx(reinterpret_cast<HANDLE>(target->sock_), nullptr);
			}
		}
		else
		{
			if (wsaSendError == 10038)
			{
				DebugBreak();
			}

			ReturnReference(target);
		}
	}
}

void IOCPServer::CheckRecvReturn(Session* target, int recvReturn)
{
	if (recvReturn == SOCKET_ERROR)
	{
		int wsaRecvError = WSAGetLastError();

		if (wsaRecvError == WSA_IO_PENDING)
		{
			if (target->disconnectFlag_ == 1)
			{
				CancelIoEx(reinterpret_cast<HANDLE>(target->sock_), nullptr);
			}
		}
		else
		{
			ReturnReference(target);
		}
	}
}

bool IOCPServer::CheckLibraryPacketCode(BYTE code)
{
	if (code != DKServerCore::LibraryPacketCode)
	{
		return false;
	}

	return true;
}

int IOCPServer::FindSession(__int64 sessionId)
{
	return static_cast<int>(sessionId >> 48);
}

int IOCPServer::GetAcceptTPS()
{
	return acceptTps_;
}

int IOCPServer::GetRecvMessageTPS()
{
	return recvMessageTps_;
}

int IOCPServer::GetSendMessageTPS()
{
	return sendMessageTps_;
}
