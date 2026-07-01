#include "IOCPServerRB.h"

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

unsigned int __stdcall IOCPServerRB::AcceptThread(void* thisPointer)
{
	IOCPServerRB* thisForAccept = static_cast<IOCPServerRB*>(thisPointer);

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

unsigned int __stdcall IOCPServerRB::WorkerThread(void* thisPointer)
{
	IOCPServerRB* thisForWorker = static_cast<IOCPServerRB*>(thisPointer);

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


		if (cbTransferred == 0) //정상 종료
		{

		}
		else if (retval == 0)
		{
			int error = WSAGetLastError();

			if (!(error == 64 || error == 995 || error == 1236))
			{
				__debugbreak();
			}
		}
		else
		{
			if (overlapPointer == nullptr && cbTransferred == 2 && target != nullptr)
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
				thisForWorker->SendCompletion(target, cbTransferred);
			}
		}

		thisForWorker->ReturnReference(target);
	}

	return 0;
}

unsigned int __stdcall IOCPServerRB::MonitorThread(void* thisPointer)
{
	IOCPServerRB* thisForMonitor = static_cast<IOCPServerRB*>(thisPointer);

	while (true)
	{
		InterlockedExchange(&thisForMonitor->acceptCount_, 0);
		InterlockedExchange(&thisForMonitor->recvMessageCount_, 0);
		InterlockedExchange(&thisForMonitor->sendMessageCount_, 0);

		Sleep(1000);
	}

	return 0;
}

unsigned int __stdcall IOCPServerRB::HeartbeatThread(void* thisPointer)
{
	IOCPServerRB* thisForHeartbeat = static_cast<IOCPServerRB*>(thisPointer);

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

IOCPServerRB::IOCPServerRB()
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

IOCPServerRB::~IOCPServerRB()
{
}

bool IOCPServerRB::Start(const char* serverIp, unsigned int serverPort, unsigned int workerNum, unsigned int concurrentThreads, unsigned int nagle, unsigned int sessions, unsigned int header)
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

bool IOCPServerRB::Start(const DKServerCore::IocpServerStartConfig& config)
{
	maxSession_ = config.maxSessionCount;
	sessionNum_ = 0;
	sessionArray_ = new Session[maxSession_];

	headerSize_ = config.headerSize;
	packetCode_ = config.packetCode;

	if (headerSize_ != sizeof(PacketHeader))
	{
		return false;
	}

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
		return false;
	}

	handleIocp_ = CreateIOCP(config.concurrentThreadCount);

	if (handleIocp_ == nullptr)
	{
		return false;
	}

	listenSock_ = socket(AF_INET, SOCK_STREAM, 0);

	if (listenSock_ == INVALID_SOCKET)
	{
		return false;
	}

	SOCKADDR_IN serverAddress;
	ZeroMemory(&serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	InetPtonA(AF_INET, config.ip.c_str(), &serverAddress.sin_addr);
	serverAddress.sin_port = htons(config.port);

	int bindReturn = bind(listenSock_, reinterpret_cast<const sockaddr*>(&serverAddress), sizeof(serverAddress));

	if (bindReturn == SOCKET_ERROR)
	{
		return false;
	}

	LINGER linger;
	linger.l_linger = 0;
	linger.l_onoff = 1;

	int socketOption = setsockopt(listenSock_, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char*>(&linger), sizeof(linger));

	if (socketOption == SOCKET_ERROR)
	{
		return false;
	}

	if (config.nagle)
	{
		DWORD noDelay = 1;

		int noDelayOption = setsockopt(listenSock_, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&noDelay), sizeof(noDelay));

		if (noDelayOption == SOCKET_ERROR)
		{
			return false;
		}
	}

	int listenReturn = listen(listenSock_, SOMAXCONN_HINT(7000));

	if (listenReturn == SOCKET_ERROR)
	{
		return false;
	}

	threadsNum_ = config.workerThreadCount;
	threads_ = new HANDLE[threadsNum_];

	for (unsigned int i = 0; i < threadsNum_; ++i)
	{
		threads_[i] = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, WorkerThread, this, 0, nullptr));

		if (threads_[i] == nullptr)
		{
			return false;
		}
	}

	acceptThread_ = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, AcceptThread, this, 0, nullptr));
	if (acceptThread_ == nullptr)
	{
		return false;
	}

	monitorThread_ = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, MonitorThread, this, 0, nullptr));
	if (monitorThread_ == nullptr)
	{
		return false;
	}

	heartbeatThread_ = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, HeartbeatThread, this, 0, nullptr));
	if (heartbeatThread_ == nullptr)
	{
		return false;
	}

	return true;
}

bool IOCPServerRB::Stop()
{
	PostQueuedCompletionStatus(handleIocp_, 0, 0, nullptr);

	WaitForMultipleObjects(threadsNum_, threads_, TRUE, INFINITE);

	closesocket(listenSock_);

	WaitForSingleObject(acceptThread_, INFINITE);

	return true;
}

int IOCPServerRB::GetSessionCount()
{
	return sessionNum_;
}

void IOCPServerRB::Disconnect(__int64 sessionId)
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

IOCPServerRB::Session* IOCPServerRB::SessionAlloc(int* emptyIndex, unsigned long long clientSock)
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

void IOCPServerRB::SendCompletion(Session* target, DWORD cbTransferred)
{
	target->sendBuffer_.MoveFront(cbTransferred);
	InterlockedExchange8(reinterpret_cast<volatile char*>(&target->sendFlag_), 0);
	SendPost(target);
}

void IOCPServerRB::RecvCompletion(Session* target, DWORD cbTransferred)
{
	target->loginFlag_ = true;
	target->recvBuffer_.MoveRear(cbTransferred);
	target->lastRecvTime_ = GetTickCount64();
	RecvProc(target);
	Receive(target);
}

void IOCPServerRB::SendPacket(__int64 sessionId, CPacket* packet)
{
	Profile profile(L"SendPacket");

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

	AddHeader(packet);

	int sendSize = packet->GetDataSize() + 2;
	char* sendPtr = packet->GetBufferPtr() + DKServerCore::PacketLibHeaderSize - 2;

	if (target->sendBuffer_.GetFreeSize() < sendSize)
	{
		wprintf(L"EnqueueFail in SendPacketUnicast %lld \n ", target->sessionId_);
		Disconnect(target->sessionId_);
		ReturnReference(target);
		return;
	}

	int enqueueReturn = target->sendBuffer_.Enqueue(sendPtr, sendSize);

	if (enqueueReturn != sendSize)
	{
		wprintf(L"EnqueueFail in SendPacketUnicast %lld \n ", target->sessionId_);
		Disconnect(target->sessionId_);
		ReturnReference(target);
		return;
	}

	PostQueuedCompletionStatus(handleIocp_, 2, reinterpret_cast<ULONG_PTR>(target), nullptr);
	ReturnReference(target);

}

void IOCPServerRB::SendPost(Session* target)
{
	if (InterlockedOr8(reinterpret_cast<volatile char*>(&target->disconnectFlag_), 0) == 1)
	{
		return;
	}

	long localCount = InterlockedIncrement(&target->ioCount_);

	if ((localCount & DKServerCore::ReleaseFlag) == DKServerCore::ReleaseFlag)
	{
		ReturnReference(target);
		return;
	}


	if (InterlockedExchange8(reinterpret_cast<volatile char*>(&target->sendFlag_), 1) == 0)
	{
		WSABUF localWsaBuf[2];

		int bufCount = SetWSABUF(target, localWsaBuf);

		if (!bufCount)
		{
			RecursiveCheck(target);
			ReturnReference(target);
			return;
		}

		InterlockedIncrement(&target->ioCount_);
		DWORD sendBytes = 0;
		ZeroMemory(&target->sendOverlapped_.overlapped_, sizeof(target->sendOverlapped_.overlapped_));

		int wsaSendReturn = WSASend(target->sock_, localWsaBuf, bufCount, &sendBytes, 0, &target->sendOverlapped_.overlapped_, nullptr);

		if (wsaSendReturn == SOCKET_ERROR)
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

	ReturnReference(target);



}

void IOCPServerRB::ReceiveFirst(Session* newSession)
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

void IOCPServerRB::RecvProc(Session* target)
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

void IOCPServerRB::Receive(Session* target)
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

void IOCPServerRB::AddHeader(CPacket* packetBuffer)
{
	char* temp = packetBuffer->GetBufferPtr();
	temp += DKServerCore::PacketLibHeaderSize - 2;

#pragma pack(push, 1)
	struct SendHeader
	{
		BYTE code_;
		BYTE size_;
	};
#pragma pack(pop)

	SendHeader packetHeader;
	packetHeader.code_ = packetCode_;
	packetHeader.size_ = static_cast<BYTE>(packetBuffer->GetDataSize() - 1);

	memcpy_s(temp, sizeof(SendHeader), &packetHeader, sizeof(SendHeader));
}

void IOCPServerRB::Release(Session* target)
{
	if (InterlockedCompareExchange(&target->ioCount_, DKServerCore::ReleaseFlag, 0) != 0)
	{
		return;
	}

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

void IOCPServerRB::RegisterIOCP(HANDLE newSocket, ULONG_PTR key)
{
	CreateIoCompletionPort(newSocket, handleIocp_, key, 0);
}

HANDLE IOCPServerRB::CreateIOCP(DWORD concurrent)
{
	return CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, concurrent);
}

int* IOCPServerRB::FindEmptySession()
{
	int* tempIndex = indexList_.Alloc();
	return tempIndex;
}

void IOCPServerRB::ClearSendBuffer(Session* target)
{
	target->sendBuffer_.ClearBuffer();
}

void IOCPServerRB::ReturnReference(Session* target)
{
	if (InterlockedDecrement(&target->ioCount_) == 0)
	{
		Release(target);
	}
}

int IOCPServerRB::SetWSABUF(Session* target, WSABUF* wsaBuf)
{
	int useSize = target->sendBuffer_.GetUseSize();

	if (useSize == 0)
	{
		return 0;
	}

	int firstSize = target->sendBuffer_.DirectDequeueSize();
	int secondSize = useSize - firstSize;

	wsaBuf[0].buf = target->sendBuffer_.GetFrontBufferPtr();
	wsaBuf[0].len = firstSize;

	if (secondSize > 0)
	{
		wsaBuf[1].buf = target->sendBuffer_.GetStartBufferPtr();
		wsaBuf[1].len = secondSize;
		return 2;
	}

	return 1;
}

void IOCPServerRB::GetClientAddress(SOCKET clientSocket, sockaddr_in& clientAddr, WCHAR* addr)
{
	int addrLen = sizeof(clientAddr);

	getpeername(clientSocket, reinterpret_cast<SOCKADDR*>(&clientAddr), &addrLen);

	if (InetNtopW(AF_INET, &clientAddr.sin_addr, addr, INET_ADDRSTRLEN) == nullptr)
	{
		wprintf(L"InetNtop Error \n");
		DebugBreak();
	}
}

void IOCPServerRB::RecursiveCheck(Session* target)
{
	InterlockedExchange8(reinterpret_cast<volatile char*>(&target->sendFlag_), 0);

	if (target->sendBuffer_.GetUseSize() != 0)
	{
		SendPost(target);
	}
}

void IOCPServerRB::CheckSendReturn(Session* target, int sendReturn)
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

void IOCPServerRB::CheckRecvReturn(Session* target, int recvReturn)
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

bool IOCPServerRB::CheckLibraryPacketCode(BYTE code)
{
	if (code != DKServerCore::LibraryPacketCode)
	{
		return false;
	}

	return true;
}

int IOCPServerRB::FindSession(__int64 sessionId)
{
	return static_cast<int>(sessionId >> 48);
}

int IOCPServerRB::GetAcceptTPS()
{
	return acceptTps_;
}

int IOCPServerRB::GetRecvMessageTPS()
{
	return recvMessageTps_;
}

int IOCPServerRB::GetSendMessageTPS()
{
	return sendMessageTps_;
}
