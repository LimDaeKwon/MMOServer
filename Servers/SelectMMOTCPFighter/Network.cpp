#include "Network.h"

#include "Character.h"
#include "Session.h"
#include "Sector.h"

#include <iostream>
#include "RingBuffer.h"
#include "winsock2.h"
#include "ws2tcpip.h"
#include "PacketDefine.h"
#include "CPacket.h"
#include "NetworkProxy.h"
#include "NetworkStub.h"
#include "conio.h"
#include "Contents.h"
#include "unordered_map"
#include "ObjectFreeList.h"
#pragma comment(lib,"Ws2_32.lib")




unsigned int SessionID = 0;
SOCKET ListenSocket = INVALID_SOCKET;
std::unordered_map<unsigned int, Session*> Sessions;
ObjectFreeList<Session> SessionFreeList(10000);


extern std::list<unsigned int> DeleteList;
extern std::unordered_map<unsigned int, Character*> CharacterMap;
extern std::list<Character*> Sector[SECTORMAXY][SECTORMAXX];
//extern std::unordered_map<unsigned int, unsigned int> whydelete;
//extern bool Shutdown = false;


void Network()
{
	


	fd_set ReadSet;
	fd_set WriteSet;


	timeval t;
	t.tv_sec = 0;
	t.tv_usec = 0;

	int Count = 0;


	unsigned int StartID;

	if (!Sessions.size())
	{
		AcceptClient();
	}




	std::unordered_map<unsigned int, Session*>::iterator Iter;
	Iter = Sessions.begin();

	FD_ZERO(&ReadSet);
	FD_ZERO(&WriteSet);
	FD_SET(ListenSocket, &ReadSet);
	Session* target;
	for (; Iter != Sessions.end(); ++Iter)
	{
		target = Iter->second;


		if (Count == 0)
		{
			StartID = target->sessionId_;
		}

		FD_SET(target->socket_, &ReadSet);
		if (target->sendQueue_.GetUseSize() > 0)
		{
			FD_SET(target->socket_, &WriteSet);
		}

		Count++;


		if (Count == 63)
		{

			timeval t;
			t.tv_sec = 0;
			t.tv_usec = 0;

			int SelectReturn = select(0, &ReadSet, &WriteSet, NULL, &t);
			if (SelectReturn == SOCKET_ERROR)
			{
				SelectReturn = WSAGetLastError();
				wprintf(L"Select Error : %d", SelectReturn);
				DebugBreak();
			}

			if (SelectReturn > 0)
			{
				if (FD_ISSET(ListenSocket, &ReadSet))
				{
					AcceptClient();
				}


				std::unordered_map<unsigned int, Session*>::iterator IterSession = Sessions.find(StartID);
				for (; IterSession != Sessions.end(); ++IterSession)
				{
					if ((SelectReturn == 0))
					{
						break;
					}

					target = IterSession->second;
					if (target->isDelete_ == 1)
					{
						continue;
					}

					if (FD_ISSET(target->socket_, &ReadSet))
					{
						Receive(target);
						SelectReturn--;
					}

					if (FD_ISSET(target->socket_, &WriteSet))
					{
						SendAll(target);
						SelectReturn--;
					}
				}
			}

			//Update();

			FD_ZERO(&ReadSet);
			FD_ZERO(&WriteSet);
			FD_SET(ListenSocket, &ReadSet);
			Count = 0;
		}

	}



	if (Count > 0)
	{
		timeval t;
		t.tv_sec = 0;
		t.tv_usec = 0;

		int SelectReturn = select(0, &ReadSet, &WriteSet, NULL, &t);
		if (SelectReturn == SOCKET_ERROR)
		{
			SelectReturn = WSAGetLastError();
			wprintf(L"Select Error : %d", SelectReturn);
			DebugBreak();
		}

		if (SelectReturn > 0)
		{

			if (FD_ISSET(ListenSocket, &ReadSet))
			{
				AcceptClient();
			}




			std::unordered_map<unsigned int, Session*>::iterator IterSession = Sessions.find(StartID);
			for (; IterSession != Sessions.end(); ++IterSession)
			{
				if ((SelectReturn == 0))
				{
					break;
				}

				target = IterSession->second;
				if (target->isDelete_ == 1)
				{
					continue;
				}

				if (FD_ISSET(target->socket_, &ReadSet))
				{
					Receive(target);
					SelectReturn--;
				}

				if (FD_ISSET(target->socket_, &WriteSet))
				{
					SendAll(target);
					SelectReturn--;
				}
			}
		}
	}



}

void AcceptClient()
{
	int AcceptError;
	SOCKADDR_IN ClientAddr;
	int AddrLen = sizeof(ClientAddr);

	SOCKET ClientSocket = accept(ListenSocket, (sockaddr*)&ClientAddr, &AddrLen);
	if (ClientSocket == INVALID_SOCKET)
	{

		AcceptError = WSAGetLastError();
		if (AcceptError == WSAEWOULDBLOCK)
		{
			return;
		}

		wprintf(L"Accept Error : %d", AcceptError);
		DebugBreak();
	}

	WCHAR ClientIP[16] = { 0 };

	if (InetNtop(AF_INET, &ClientAddr.sin_addr, ClientIP, 16) == NULL)
	{

		wprintf(L"InetNtop Error \n");
		DebugBreak();

	}

	//wprintf(L"## Connect # IP : %s    /  SessionID : %d \n", ClientIP, SessionID);

	Session* NewSession = SessionFreeList.Alloc();

	NewSession->sessionId_ = SessionID++;
	NewSession->socket_ = ClientSocket;
	NewSession->isDelete_ = 0;
	NewSession->lastRecvTime_ = timeGetTime();

	Sessions.insert(std::unordered_map<unsigned int, Session*>::value_type(NewSession->sessionId_, NewSession));
	CreateCharater(NewSession);


}



void SendPacketUnicast(Session* Target, CPacket* Packet)
{
	int DataSize = Packet->GetDataSize();
	int EnqueueHeaderReturn = Target->sendQueue_.Enqueue(Packet->GetBufferPtr(), DataSize);
	if (EnqueueHeaderReturn != DataSize)
	{
		//인큐 불가 상태.
		wprintf(L"EnqueueFail in SendPacketUnicast %d \n ",Target->sessionId_);

		Disconnect(Target);

		//DebugBreak();
	}
}

void SendPacketSectorOne(int SectorX, int SectorY, Session* Except, CPacket* Packet)
{
	int DataSize = Packet->GetDataSize();
	int EnqueueHeaderReturn;
	Character* Target;
	std::list<Character*>::iterator Iter;
	for (Iter = Sector[SectorY][SectorX].begin(); Iter != Sector[SectorY][SectorX].end(); ++Iter)
	{
		Target = *Iter;
		if ((Target->characterSession_ == Except) || (Target->characterSession_->isDelete_ == 1))
		{
			continue;
		}

		EnqueueHeaderReturn = Target->characterSession_->sendQueue_.Enqueue(Packet->GetBufferPtr(), DataSize);
		if (EnqueueHeaderReturn != DataSize)
		{
			//인큐 불가 상태.
			wprintf(L"EnqueueFail in SendPacketUnicast%d \n ", Target->sessionId_);
			Disconnect(Target->characterSession_);


			//DebugBreak();
		}
	}
}

void SendPacketAroundRemoveSector(Session* Target, CPacket* Packet, SectorAround* Around)
{

	for (unsigned int Index = 0; Index < Around->count_; Index++)
	{
		SendPacketSectorOne(Around->around_[Index].x_, Around->around_[Index].y_, NULL, Packet);
	}


}

void SendPacketAroundAddSector(Session* Target, CPacket* Packet, SectorAround* Around)
{
	for (unsigned int Index = 0; Index < Around->count_; Index++)
	{
		SendPacketSectorOne(Around->around_[Index].x_, Around->around_[Index].y_, NULL, Packet);
	}
}


void SendPacketAround(Session* Session, CPacket* Packet, bool SendMe)
{
	Character* Target = CharacterMap.at(Session->sessionId_);
	SectorAround Around;

	GetSectorAround(Target->characterSectorPos_.x_, Target->characterSectorPos_.y_, &Around);


	if (SendMe)
	{
		for (unsigned int Index = 0; Index < Around.count_; Index++)
		{
			SendPacketSectorOne(Around.around_[Index].x_, Around.around_[Index].y_, NULL, Packet);
		}
	}
	else
	{
		for (unsigned int Index = 0; Index < Around.count_; Index++)
		{
			SendPacketSectorOne(Around.around_[Index].x_, Around.around_[Index].y_, Session, Packet);
		}
	}


}
CPacket* cPacketBuffer = CPacket::Alloc();

void Receive(Session* Target)
{

	int DirectEnqueueSize = Target->receiveQueue_.DirectEnqueueSize();

	int RecvError;
	int RecvReturn = recv(Target->socket_, Target->receiveQueue_.GetRearBufferPtr(), DirectEnqueueSize, 0);


	if (RecvReturn == SOCKET_ERROR)
	{
		RecvError = WSAGetLastError();
		if (RecvError != WSAEWOULDBLOCK)
		{
			//printf("RecvError  :  %d rst. \n", RecvError);
			//printf(" 리시브 실패로 종료 당하는 녀석 : %d \n\n", Target->SessionID);
			//whydelete.insert(std::unordered_map<unsigned int, unsigned int>::value_type(Target->SessionID, RecvError));

			Disconnect(Target);
			//DebugBreak();
			return;
		}
	}

	if (RecvReturn == 0)
	{
		//정상 종료.

		//printf(" 정상 종료 당하는 녀석: %d \n\n", Target->SessionID);
		//whydelete.insert(std::unordered_map<unsigned int, unsigned int>::value_type(Target->SessionID, RECVZERO));

		Disconnect(Target);
		return;
	}
	Target->receiveQueue_.MoveRear(RecvReturn);
	//wprintf(L"## ID : %d  ReceiveQEnqueue Size : %d  \n", Target->SessionID, RecvReturn);
	unsigned int ReceiveQSize;
	if (RecvReturn > 0)
	{

		while (1)
		{
			ReceiveQSize = Target->receiveQueue_.GetUseSize();
			if (ReceiveQSize == 0)
			{
				break;
			}

			PacketHeader Header;
			if (ReceiveQSize < sizeof(PacketHeader))
			{
				break;
			}

			if (Target->receiveQueue_.Peek((char*)&Header, sizeof(Header)) != sizeof(Header))
			{
				break;
			}

			if (Header.ByCode != 0x89)
			{
				wprintf(L"Header.ByCode != 0x89\n");
				//whydelete.insert(std::unordered_map<unsigned int, unsigned int>::value_type(Target->SessionID, CODE));

				Disconnect(Target);
				break;
			}

			if (ReceiveQSize < sizeof(Header) + Header.BySize)
			{
				break;
			}

			unsigned int ReceiveQDequeueHeaderSize = Target->receiveQueue_.MoveFront(sizeof(Header));

			//wprintf(L"## ID : %d  DequeueHeaderSize : %d  ", Target->SessionID, ReceiveQDequeueHeaderSize);

			cPacketBuffer->Clear();

			unsigned int ReceiveQDequeuePacketSize = Target->receiveQueue_.Dequeue(cPacketBuffer->GetBufferPtr(), Header.BySize);

			if (ReceiveQDequeuePacketSize != Header.BySize)
			{
				wprintf(L"## ReceiveQDequeuePacketSize != Header.BySize : %d \n", ReceiveQDequeuePacketSize);
				//DebugBreak();
				Disconnect(Target);

				break;
			}
			cPacketBuffer->MoveWritePosition(ReceiveQDequeuePacketSize);

			//wprintf(L"## DequeuePacketSize : %d \n", ReceiveQDequeuePacketSize);

			Target->lastRecvTime_ = timeGetTime();

			PacketProc(Target, Header.ByType, cPacketBuffer);

		}
	}
}



void ServerControl()
{
	static bool ControlMode = false;


	if (_kbhit())
	{
		WCHAR ControlKey = _getwch();

		if (L'u' == ControlKey || L'U' == ControlKey)
		{
			ControlMode = true;

		}

		if (ControlMode && L'q' == ControlKey || L'q' == ControlKey)
		{

			//원하는 기능 처리.

		}

	}

}

void DeleteDisconnect()
{
	if (DeleteList.size() > 0)
	{

		Session* Session;
		Character* DeleteTarget;
		unsigned int Session_ID;
		std::list<unsigned int>::iterator Iter;
		for (Iter = DeleteList.begin(); Iter != DeleteList.end();++Iter)
		{
			Session_ID = *Iter;
			Session = Sessions.at(Session_ID);
			DeleteTarget = CharacterMap.at(Session_ID);
			
			FreeCharacter(DeleteTarget);

			CharacterMap.erase(Session_ID);

			closesocket(Session->socket_);
			Session->receiveQueue_.ClearBuffer();
			Session->sendQueue_.ClearBuffer();

			SessionFreeList.Free(Session);
			Sessions.erase(Session_ID);

		}

		DeleteList.clear();
	}
}



void SendAll(Session* Target)
{

	int DirectDequeueSize = Target->sendQueue_.DirectDequeueSize();
	//한 프레임에 센드를 두번 해주는 느낌..
	int SendReturn = send(Target->socket_, Target->sendQueue_.GetFrontBufferPtr(), DirectDequeueSize, 0);
	//wprintf(L"## ID : %d  SendSize : %d \n", Target->ID, SecondMoveFrontSize);
	if (SendReturn == SOCKET_ERROR)
	{
		int Error = WSAGetLastError();

		if (Error != WSAEWOULDBLOCK)
		{
			//wprintf(L"SendError : %d \n", Error);
			//연결이 끊긴거임. 삭제해야해. 
			//printf("센드 실패로 종료 당하는 녀석 : %d \n\n", Target->SessionID);
			//whydelete.insert(std::unordered_map<unsigned int, unsigned int>::value_type(Target->SessionID, SENDFAILED));

			Disconnect(Target);
			return;
		}
	}
	if (SendReturn != DirectDequeueSize)
	{
		//뭔가 문제가 있는거.
		wprintf(L"SendSize : %d , SendReturn : %d \n", DirectDequeueSize, SendReturn);
		//DebugBreak();
		Disconnect(Target);
		return;
	}
	int MoveFrontSize = Target->sendQueue_.MoveFront(DirectDequeueSize);
	if (MoveFrontSize != DirectDequeueSize)
	{
		wprintf(L"MoveFrontSize : %d , DirectDequeueSize : %d \n", MoveFrontSize, DirectDequeueSize);
		DebugBreak();
	}
}


void Initialize()
{
	WSADATA Wsa;
	if (WSAStartup(MAKEWORD(2, 2), &Wsa) != 0)
	{
		wprintf(L"WSAStartup %d \n", WSAStartup(MAKEWORD(2, 2), &Wsa));
		DebugBreak();
	}

	ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (ListenSocket == INVALID_SOCKET)
	{
		int Error = WSAGetLastError();
		wprintf(L"ListenSocket Error %d \n", Error);

		DebugBreak();
	}


	SOCKADDR_IN ServerAddr;
	ZeroMemory(&ServerAddr, sizeof(ServerAddr));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	ServerAddr.sin_port = htons(SERVERPORT);

	int BindReturn = bind(ListenSocket, (const sockaddr*)&ServerAddr, sizeof(ServerAddr));
	if (BindReturn == SOCKET_ERROR)
	{
		BindReturn = WSAGetLastError();

		wprintf(L"Connect Error : %d \n", BindReturn);

		DebugBreak();
	}

	LINGER linger;
	linger.l_linger = 0;
	linger.l_onoff = 1;

	int SocketOption = setsockopt(ListenSocket, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(linger));
	if (SocketOption == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		printf("setsockopt Error %d ", error);

		DebugBreak();
	}
	DWORD NoDelay = 1;


	int NoDelayOption = setsockopt(ListenSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&NoDelay, sizeof(NoDelay));
	if (NoDelayOption == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		printf("setsockopt Error %d ", error);

		DebugBreak();
	}



	int ListenReturn = listen(ListenSocket, SOMAXCONN_HINT(7000));
	if (ListenReturn == SOCKET_ERROR)
	{
		ListenReturn = WSAGetLastError();

		wprintf(L"Listen Error : %d \n", ListenReturn);
		DebugBreak();
	}

	u_long On = 1;
	int IoctlSocketError = ioctlsocket(ListenSocket, FIONBIO, &On);

	if (IoctlSocketError == INVALID_SOCKET)
	{
		IoctlSocketError = WSAGetLastError();

		wprintf(L"IoctlSocketError Error : %d \n", IoctlSocketError);

		DebugBreak();
	}
}

