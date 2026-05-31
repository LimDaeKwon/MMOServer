#include "LanNetworkLibraryServer.h"
#include <iostream>
#include <winsock2.h>
#include <stdlib.h>
#include <process.h>
#include <windows.h>
#include "Ws2ipdef.h"
#include "ws2tcpip.h"
#include "Profiler.h"

#include <unordered_map>
#include "CPacket.h"
#include "CPacketQueue.h"
#include "ContentsCPacket.h"



#pragma comment(lib,"ws2_32.lib")

#define RELEASEFLAG 0x80000000


unsigned int __stdcall LanNetworkLibraryServer::AcceptThread(LPVOID this_ptr)
{
	LanNetworkLibraryServer* this_for_Accept = (LanNetworkLibraryServer*)this_ptr;
	while (1)
	{

		SOCKET client_sock;
		client_sock = accept(this_for_Accept->listen_sock, NULL, NULL);
		if (client_sock == INVALID_SOCKET)
		{
			int error = WSAGetLastError();
			if (error == 10004)
			{
				//리슨소켓 종료로 인한 어셉트 종료.
				break;
			}
			wprintf(L"accept Error %d ", error);
			DebugBreak();

		}

		Session* new_session = this_for_Accept->SessionAlloc(this_for_Accept->FindEmptySession(), client_sock);

		InterlockedIncrement(&this_for_Accept->accept_count);
		this_for_Accept->RegisterIOCP((HANDLE)client_sock, (ULONG_PTR)new_session);

		sockaddr_in clientaddr;
		WCHAR addrl[INET_ADDRSTRLEN];
		this_for_Accept->GetClientAddress(client_sock, clientaddr, addrl);
		this_for_Accept->OnAccept(addrl, ntohs(clientaddr.sin_port), new_session->session_id);

		//wprintf(L"Connect Client : IP  = %s , PORT = %d  , Session ID : %lld\n", addrl, ntohs(clientaddr.sin_port), this_for_Accept->unique_id);

		this_for_Accept->ReceiveFirst(new_session);

	}

	return 0;
}



unsigned int __stdcall LanNetworkLibraryServer::WorkerThread(LPVOID this_ptr)
{
	LanNetworkLibraryServer* this_for_worker = (LanNetworkLibraryServer*)this_ptr;
	while (1)
	{
		DWORD cbTransferred = 0;
		MyOverlapped* overlap_ptr;
		Session* target = nullptr;
		int retval;
		retval = GetQueuedCompletionStatus(this_for_worker->handle_iocp, &cbTransferred, (PULONG_PTR)&target, (LPOVERLAPPED*)&overlap_ptr, INFINITE);

		//IOCP 종료 메시지
		if (overlap_ptr == NULL && cbTransferred == NULL && target == NULL)
		{
			PostQueuedCompletionStatus(this_for_worker->handle_iocp, NULL, NULL, NULL);
			break;
		}

		//IO 문제 발생. 
		if (retval == 0)
		{
			int error = WSAGetLastError();
			if (!(error == 64 || error == 995 || error == 1236)) // 다른 특이한 에러가 있는지 보기 위해서 하나씩 추가. 
			{
				//64 : 연결을 끊음.
				//995 : CancelIoEx에 의한 취소.
				//1236 : 로컬rst로 추정
				__debugbreak();
			}

		}
		else
		{
			if ((overlap_ptr == NULL && cbTransferred == NULL && target != NULL))
			{
				this_for_worker->SendPost(target);
			}
			else if (overlap_ptr->Type == RECV)
			{
				this_for_worker->RecvCompletion(target, cbTransferred);
			}
			else if (overlap_ptr->Type == SEND)
			{
				this_for_worker->SendCompletion(target);
			}
		}

		this_for_worker->ReturnReference(target);


	}
	return 0;
}

unsigned int __stdcall LanNetworkLibraryServer::MonitorThread(LPVOID this_ptr)
{
	LanNetworkLibraryServer* this_for_monitor = (LanNetworkLibraryServer*)this_ptr;

	while (1)
	{

		//this_for_monitor->accept_TPS = this_for_monitor->accept_count;
		//this_for_monitor->recv_message_TPS = this_for_monitor->recv_message_count;
		//this_for_monitor->send_message_TPS = this_for_monitor->send_message_count;

		//std::cout << CPacket::GetCapacity() << std::endl;
		//std::cout << CPacket::GetUseSize() << std::endl;
		//std::cout << CPacket::GetPoolSize() << std::endl;

		InterlockedExchange(&this_for_monitor->accept_count, 0);
		InterlockedExchange(&this_for_monitor->recv_message_count, 0);
		InterlockedExchange(&this_for_monitor->send_message_count, 0);

		Sleep(1000);

	}




	return 0;
}

unsigned int __stdcall LanNetworkLibraryServer::HeartbeatThread(LPVOID this_ptr)
{

	LanNetworkLibraryServer* this_for_heartbeat = (LanNetworkLibraryServer*)this_ptr;
	unsigned int local_count = 0;
	while (1)
	{
		if (local_count % this_for_heartbeat->unlogin_timeout == 0)
		{
			printf("Heartbeat Check \n");
			for (unsigned int i = 0; i < this_for_heartbeat->max_session; ++i)
			{
				Session* target = &this_for_heartbeat->session_array[i];
				if (target->use_flag == 0)
				{
					continue;
				}
				if (target->login_flag == 1)
				{
					continue;
				}
				if (GetTickCount64() - target->last_recv_time > this_for_heartbeat->unlogin_timeout * 10000) // 3초
				{
					this_for_heartbeat->Disconnect(target->session_id);
				}

			}
		}

		if (local_count % this_for_heartbeat->timeout == 0)
		{
			printf("Heartbeat Check \n");
			for (unsigned int i = 0; i < this_for_heartbeat->max_session; ++i)
			{
				Session* target = &this_for_heartbeat->session_array[i];
				if (target->use_flag == 0)
				{
					continue;
				}
				if (target->login_flag == 0)
				{
					continue;
				}
				if (GetTickCount64() - target->last_recv_time > this_for_heartbeat->timeout * 1000) // 3초
				{
					this_for_heartbeat->Disconnect(target->session_id);
				}
			}

		}


		local_count++;
		Sleep(1000);
	}




	return 0;
}






LanNetworkLibraryServer::LanNetworkLibraryServer()
	: accept_TPS(0), recv_message_TPS(0), send_message_TPS(0), accept_count(0),
	recv_message_count(0), send_message_count(0), max_session(0), session_num(0), threads_num(0), unique_id(0),
	index_list(0, false)

{

}

LanNetworkLibraryServer::~LanNetworkLibraryServer()
{
}


bool LanNetworkLibraryServer::Start(const char* server_IP, unsigned int  server_port, unsigned int worker_num, unsigned int concurrent_threads, unsigned int nagle, unsigned int sessions, unsigned int header)
{

	max_session = sessions;
	session_num = 0;
	session_array = new Session[max_session];
	header_size = header;
	int** temp = new int* [max_session];
	timeout = 30;
	unlogin_timeout = 3;


	//초기 인덱스 세팅. 
	for (unsigned int i = 0; i < max_session; ++i)
	{
		temp[i] = index_list.Alloc();
		*temp[i] = i;
	}
	for (unsigned int i = 0; i < max_session; ++i)
	{
		index_list.Free(temp[i]);

	}

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		DebugBreak();
	}
	//테스트를 통하여 적절한 수 세팅해야 함.
	handle_iocp = CreateIOCP(concurrent_threads);
	if (handle_iocp == NULL)
	{
		DebugBreak();
	}


	listen_sock = socket(AF_INET, SOCK_STREAM, 0);

	if (listen_sock == INVALID_SOCKET)
	{
		int Error = WSAGetLastError();
		wprintf(L"ListenSocket Error %d \n", Error);

		DebugBreak();
	}


	SOCKADDR_IN server_address;
	ZeroMemory(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	InetPtonA(AF_INET, server_IP, &server_address.sin_addr);
	//server_address.sin_addr.s_addr = htonl(INADDR_ANY); // 문자열 파싱 해야 함 .
	server_address.sin_port = htons(server_port);

	int bind_return = bind(listen_sock, (const sockaddr*)&server_address, sizeof(server_address));
	if (bind_return == SOCKET_ERROR)
	{
		bind_return = WSAGetLastError();

		wprintf(L"BindReturn Error : %d \n", bind_return);

		DebugBreak();
	}


	//DWORD OptionVal = 0;

	//int socket_option_return = setsockopt(listen_sock, SOL_SOCKET, SO_SNDBUF, (const char*)&OptionVal, sizeof(OptionVal));
	//if (socket_option_return == SOCKET_ERROR)
	//{
	//	socket_option_return = WSAGetLastError();

	//	wprintf(L"SocketOptionReturn Error : %d \n", socket_option_return);
	//	DebugBreak();
	//}



	LINGER linger;
	linger.l_linger = 0;
	linger.l_onoff = 1;

	int SocketOption = setsockopt(listen_sock, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(linger));
	if (SocketOption == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		printf("setsockopt Error %d ", error);

		DebugBreak();
	}

	if (nagle)
	{
		DWORD NoDelay = 1;

		int NoDelayOption = setsockopt(listen_sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&NoDelay, sizeof(NoDelay));
		if (NoDelayOption == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			printf("setsockopt Error %d ", error);

			DebugBreak();
		}
	}



	int listen_return = listen(listen_sock, SOMAXCONN_HINT(7000));
	//int listen_return = listen(listen_sock, SOMAXCONN);
	if (listen_return == SOCKET_ERROR)
	{
		listen_return = WSAGetLastError();

		wprintf(L"Listen Error : %d \n", listen_return);
		DebugBreak();
	}

	SYSTEM_INFO si;
	GetSystemInfo(&si);

	threads_num = worker_num;

	threads = new HANDLE[threads_num];

	for (unsigned int i = 0; i < threads_num; ++i)
	{
		threads[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, this, 0, NULL);

		if (threads[i] == NULL)
		{
			wprintf(L"_beginthreadex Failed \n");
			DebugBreak();
		}

	}

	accept_thread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, NULL);

	if (accept_thread == NULL)
	{
		wprintf(L"_beginthreadex Failed \n");
		DebugBreak();
	}

	monitor_thread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, NULL);

	if (monitor_thread == NULL)
	{
		wprintf(L"_beginthreadex Failed \n");
		DebugBreak();
	}

	heartbeat_thread = (HANDLE)_beginthreadex(NULL, 0, HeartbeatThread, this, 0, NULL);

	if (heartbeat_thread == NULL)
	{
		wprintf(L"_beginthreadex Failed \n");
		DebugBreak();
	}


	return true;
}

bool LanNetworkLibraryServer::Stop()
{

	PostQueuedCompletionStatus(handle_iocp, NULL, (ULONG_PTR)NULL, NULL);

	WaitForMultipleObjects(threads_num, threads, TRUE, INFINITE);

	closesocket(listen_sock);

	WaitForSingleObject(accept_thread, INFINITE);


	return true;
}

int LanNetworkLibraryServer::GetSessionCount()
{
	return session_num;
}

void LanNetworkLibraryServer::Disconnect(__int64 session_ID)
{


	Session* target;
	unsigned int i = FindSession(session_ID);
	target = &session_array[i];


	int local_count = InterlockedIncrement(&target->io_count);
	//릴리즈 플래그 확인.

	if ((local_count & RELEASEFLAG) == RELEASEFLAG)
	{
		ReturnReference(target);
	}

	//이러면 해제 이후 다시 재사용 된 상태. 다시 카운트 감소시키고 리턴시키기. 
	if (target->session_id != session_ID)
	{
		//근데 생각해보니 재사용되었고 또 삭제되어야 했을 수도 있음. 
		//목적과 다른 녀석이지만 얘도 내가 증가시켰으므로 감소 후 지워주기. 
		ReturnReference(target);
	}

	// Disconnect 1회 제한 // 성능을 위해서. 
	if (InterlockedExchange8((char*)&target->disconnect_flag, 1) == 1)
	{
		ReturnReference(target);
	}

	CancelIoEx((HANDLE)target->sock, nullptr);

	ReturnReference(target);
}

LanNetworkLibraryServer::Session* LanNetworkLibraryServer::SessionAlloc(int* empty_index, unsigned long long client_sock)
{
	Session* new_session = &session_array[*empty_index];

	new_session->index = empty_index;


	if (*new_session->index == max_session)
	{
		DebugBreak();
	}

	if (InterlockedIncrement(&session_num) > max_session)
	{
		DebugBreak();
	}

	__int64 i = *new_session->index;



	new_session->session_id = ++unique_id;
	new_session->session_id |= (i << 48);
	new_session->buffer_count.count = 0;
	new_session->sock = (SOCKET)client_sock;
	new_session->send_flag = FALSE;
	new_session->disconnect_flag = FALSE;
	new_session->use_flag = TRUE;
	new_session->login_flag = FALSE;
	new_session->last_recv_time = GetTickCount64();

	InterlockedIncrement(&new_session->io_count);
	InterlockedAnd(&new_session->io_count, 0x7fffffff);

	new_session->recv_overlapped.Type = RECV;
	new_session->send_overlapped.Type = SEND;


	return new_session;
}

void LanNetworkLibraryServer::SendCompletion(Session* target)
{

	for (int i = 0; i < target->buffer_count.count; ++i)
	{
		CPacket::Free(target->buffer_count.buffers[i]);
	}

	target->buffer_count.count = 0;
	InterlockedExchange8((char*)&target->send_flag, 0);
	SendPost(target);

}

void LanNetworkLibraryServer::RecvCompletion(Session* target, DWORD cbTransferred)
{
	target->login_flag = true;
	target->recv_buffer.MoveRear(cbTransferred);
	target->last_recv_time = GetTickCount64();
	RecvProc(target);
	Receive(target);
}

#define SESSIONID __int64

void LanNetworkLibraryServer::SendPacket(SESSIONID session_ID, CPacket* send_packet)
{

	Session* target;
	unsigned int i = FindSession(session_ID);

	target = &session_array[i];
	int local_count = InterlockedIncrement(&target->io_count);

	//릴리즈플래그가 올라간 세션이라면 
	if ((local_count & RELEASEFLAG) == RELEASEFLAG)
	{
		ReturnReference(target);
		return;
	}

	//이러면 해제 이후 다시 재사용 된 상태. 다시 카운트 감소시키고 리턴시키기. 
	if (target->session_id != session_ID)
	{
		//근데 생각해보니 재사용되었고 또 삭제되어야 했을 수도 있음. 
		//목적과 다른 녀석이지만 얘도 내가 증가시켰으므로 감소 후 지워주기. 
		ReturnReference(target);
		return;
	}


	//인큐 , Send 

	if (packet_type == NET)
	{
		NetAddHeader(send_packet);

		send_packet->IncreaseRefCount();

		int enqueue_return = target->send_buffer.Enqueue(send_packet);

		InterlockedIncrement(&target->send_count);
		if (enqueue_return == false)
		{
			wprintf(L"EnqueueFail in SendPacketUnicast Session Id : %lld\n", target->session_id);
			DebugBreak();
			//Disconnect(target->session_id); //리턴이 2든 아니든 일단 호출
			return;
		}
		//SendPost(target);

		InterlockedIncrement(&target->io_count);
		PostQueuedCompletionStatus(handle_iocp, NULL, (ULONG_PTR)target, NULL);
	}
	else
	{
		LanAddHeader(send_packet);

		send_packet->IncreaseRefCount();

		int enqueue_return = target->send_buffer.Enqueue(send_packet);
		//디버그 브레이크 빼야 함 
		InterlockedIncrement(&target->send_count);
		if (enqueue_return == false)
		{
			wprintf(L"EnqueueFail in SendPacketUnicast Session Id : %lld\n", target->session_id);
			DebugBreak();
			//Disconnect(target->session_id); //리턴이 2든 아니든 일단 호출
			return;
		}
		PostQueuedCompletionStatus(handle_iocp, NULL, (ULONG_PTR)target, NULL);

	}
	ReturnReference(target);

}

void LanNetworkLibraryServer::SendPost(Session* target)
{

	if (target->disconnect_flag == 1)
	{
		return;
	}


	if (InterlockedExchange8((char*)&target->send_flag, 1) == 0)
	{

		WSABUF local_wsabuf[MAXBATCHSIZE];
		int buf_count = SetWSABUF(target, local_wsabuf);

		if (!buf_count)
		{
			RecursiveCheck(target);
			return;
		}

		DWORD sendbytes = 0;
		int WSASend_return;
		InterlockedIncrement(&target->io_count);
		ZeroMemory(&target->send_overlapped.overlapped, sizeof(target->send_overlapped.overlapped));

		WSASend_return = WSASend(target->sock, local_wsabuf, buf_count, &sendbytes, 0, &target->send_overlapped.overlapped, NULL);
		CheckSendReturn(target, WSASend_return);

	}

}



void LanNetworkLibraryServer::ReceiveFirst(Session* new_session)
{
	WSABUF wsabuf;
	wsabuf.buf = new_session->recv_buffer.GetRearBufferPtr();
	wsabuf.len = new_session->recv_buffer.GetFreeSize();

	DWORD recvbytes;
	DWORD flags = 0;
	int retval;

	ZeroMemory(&new_session->recv_overlapped.overlapped, sizeof(new_session->recv_overlapped.overlapped));
	retval = WSARecv(new_session->sock, &wsabuf, 1, &recvbytes, &flags, (WSAOVERLAPPED*)&new_session->recv_overlapped.overlapped, 0);

	if (retval == SOCKET_ERROR)
	{
		int WSARecv_error = WSAGetLastError();
		if (WSARecv_error != WSA_IO_PENDING)
		{

			wprintf(L"In First WSARecvError : %d  , Session ID : %lld\n", WSARecv_error, new_session->session_id);

			ReturnReference(new_session);
		}
	}
}

void LanNetworkLibraryServer::RecvProc(Session* target)
{
	if (packet_type == NET)
	{
		while (1)
		{
			int target_recv_buffer_size = target->recv_buffer.GetUseSize();
			NetPacketHeader header;
			if (target_recv_buffer_size < sizeof(header))
			{
				break;
			}

			target->recv_buffer.Peek((char*)&header, sizeof(header));


			if (!CheckLibraryPacketCode(header.byCode))
			{
				Disconnect(target->session_id);
				break;
			}

			header.bySize += 1;


			if (target_recv_buffer_size < sizeof(header) + header.bySize)
			{
				break;
			}

			unsigned int receive_dequeue_header_size = target->recv_buffer.MoveFront(sizeof(header));

			CPacket* packet_buffer = CPacket::Alloc();

			packet_buffer->MoveWritePosition(target->recv_buffer.Dequeue(packet_buffer->GetBufferPtr() + DKServerCore::PacketLibHeaderSize, header.bySize));

			OnMessage(target->session_id, packet_buffer);

			CPacket::Free(packet_buffer);
		}
	}
	else
	{
		while (1)
		{
			int target_recv_buffer_size = target->recv_buffer.GetUseSize();
			LanPacketHeader header;
			if (target_recv_buffer_size < sizeof(header))
			{
				break;
			}

			if (target->recv_buffer.Peek((char*)&header, sizeof(header)) != sizeof(header))
			{
				break;
			}

			if (target_recv_buffer_size < sizeof(header) + header.length)
			{
				break;
			}

			unsigned int receive_dequeue_header_size = target->recv_buffer.MoveFront(sizeof(header));

			CPacket* packet_buffer = CPacket::Alloc();


			unsigned int receive_dequeue_packet_size = target->recv_buffer.Dequeue(packet_buffer->GetBufferPtr() + DKServerCore::PacketLibHeaderSize, header.length);

			if (receive_dequeue_packet_size != header.length)
			{
				wprintf(L"## ReceiveQDequeuePacketSize != Header.BySize : %d \n", receive_dequeue_packet_size);
				DebugBreak();
				break;
			}
			packet_buffer->MoveWritePosition(receive_dequeue_packet_size);

			OnMessage(target->session_id, packet_buffer);
			CPacket::Free(packet_buffer);
		}
	}

}


void LanNetworkLibraryServer::Receive(Session* target)
{
	if (target->disconnect_flag == 1)
	{
		return;
	}

	int retval;
	WSABUF recv_wsabuf[2];

	recv_wsabuf[0].buf = target->recv_buffer.GetRearBufferPtr();
	recv_wsabuf[0].len = target->recv_buffer.DirectEnqueueSize();
	recv_wsabuf[1].buf = target->recv_buffer.GetStartBufferPtr();
	recv_wsabuf[1].len = target->recv_buffer.GetFreeSize() - target->recv_buffer.DirectEnqueueSize();

	DWORD recvbytes;
	DWORD flags = 0;
	ZeroMemory(&target->recv_overlapped.overlapped, sizeof(target->recv_overlapped.overlapped));
	InterlockedIncrement(&target->io_count);
	retval = WSARecv(target->sock, recv_wsabuf, 2, &recvbytes, &flags, &target->recv_overlapped.overlapped, 0);
	CheckRecvReturn(target, retval);

}

void LanNetworkLibraryServer::LanAddHeader(CPacket* packet_buffer)
{
	char* temp = packet_buffer->GetBufferPtr();
	temp += DKServerCore::PacketLibHeaderSize - header_size;
	LanPacketHeader LibHeader;
	LibHeader.length = packet_buffer->GetDataSize();

	(*(unsigned short*)(temp)) = LibHeader.length;

}

void LanNetworkLibraryServer::NetAddHeader(CPacket* packet_buffer)
{
	char* temp = packet_buffer->GetBufferPtr();
	temp += DKServerCore::PacketLibHeaderSize - header_size;
	unsigned short NetHeader;

	//MMOTCP기준
	//일단 왜인지는 모르지만 사이즈를 1 낮춰서 보내야함. 
	

	NetPacketHeader NetLibHeader;
	NetLibHeader.bySize = packet_buffer->GetDataSize() - 1;
	NetLibHeader.byCode = 0x89;

	NetHeader = NetLibHeader.bySize;
	NetHeader <<= 8;
	NetHeader |= NetLibHeader.byCode;
	(*(unsigned short*)(temp)) = NetHeader;

}


void LanNetworkLibraryServer::Release(Session* target)
{
	//여기서 IO/Count와 릴리즈 플래그를 한 번에 
	//RELEASEFLAG 사용. 

	if (InterlockedCompareExchange(&target->io_count, RELEASEFLAG, 0) != 0)
	{
		return;
	}


	for (int i = 0; i < target->buffer_count.count; ++i)
	{
		CPacket::Free(target->buffer_count.buffers[i]);
	}

	target->buffer_count.count = 0;

	target->recv_buffer.ClearBuffer();
	ClearSendBuffer(target);
	InterlockedExchange8((char*)&target->use_flag, 0);
	InterlockedExchange8((char*)&target->login_flag, 0);

	closesocket(target->sock);
	target->sock = INVALID_SOCKET;
	OnRelease(target->session_id);
	index_list.Free(target->index);
	InterlockedDecrement(&session_num);

	return;
}

void LanNetworkLibraryServer::RegisterIOCP(HANDLE new_socket, ULONG_PTR key)
{
	CreateIoCompletionPort((HANDLE)new_socket, handle_iocp, (ULONG_PTR)key, 0);
}

HANDLE LanNetworkLibraryServer::CreateIOCP(DWORD cuncurrent)
{
	return CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, cuncurrent);
}


int* LanNetworkLibraryServer::FindEmptySession()
{
	int* temp_index = index_list.Alloc();
	return temp_index;

}

void LanNetworkLibraryServer::ClearSendBuffer(Session* target)
{
	while (1)
	{
		CPacket* t;

		if (target->send_buffer.Dequeue(&t) == false)
		{
			break;
		}
		CPacket::Free(t);
	}

}

void LanNetworkLibraryServer::ReturnReference(Session* target)
{
	if (InterlockedDecrement(&target->io_count) == 0)
	{
		Release(target);
	}
}

int LanNetworkLibraryServer::SetWSABUF(Session* target, WSABUF* wsabuf)
{
	int buf_count = 0;
	while (buf_count < MAXBATCHSIZE)
	{
		CPacket* temp = nullptr;

		if (target->send_buffer.Dequeue(&temp) == false)
		{
			break;
		}
		target->buffer_count.buffers[buf_count] = temp;
		wsabuf[buf_count].buf = temp->GetBufferPtr() + DKServerCore::PacketLibHeaderSize - header_size;
		wsabuf[buf_count].len = temp->GetDataSize() + header_size;
		buf_count++;

	}
	target->buffer_count.count = buf_count;

	return buf_count;

}

void LanNetworkLibraryServer::GetClientAddress(SOCKET client_socket, sockaddr_in& client_addr, WCHAR* addrl)
{
	int addr_len = sizeof(client_addr);
	getpeername(client_socket, (SOCKADDR*)&client_addr, &addr_len);
	if (InetNtopW(AF_INET, &client_addr.sin_addr, addrl, INET_ADDRSTRLEN) == NULL)
	{

		wprintf(L"InetNtop Error \n");
		DebugBreak();

	}
}


void LanNetworkLibraryServer::RecursiveCheck(Session* target)
{
	InterlockedExchange8((char*)&target->send_flag, 0);
	if (target->send_buffer.GetSize() != 0)
	{
		SendPost(target);
	}
}

void LanNetworkLibraryServer::CheckSendReturn(Session* target, int send_return)
{

	if (send_return == SOCKET_ERROR)
	{
		int WSASendError = WSAGetLastError();

		if (WSASendError == WSA_IO_PENDING)
		{

			if (target->disconnect_flag == 1)
			{
				CancelIoEx((HANDLE)target->sock, nullptr);
			}

		}
		else
		{
			if (WSASendError == 10038)
			{
				DebugBreak();
			}
			ReturnReference(target);
		}
	}
}

void LanNetworkLibraryServer::CheckRecvReturn(Session* target, int recv_return)
{

	if (recv_return == SOCKET_ERROR)
	{
		int WSARecv_error = WSAGetLastError();
		if (WSARecv_error == WSA_IO_PENDING)
		{
			if (target->disconnect_flag == 1)
			{
				CancelIoEx((HANDLE)target->sock, nullptr);
			}
		}
		else
		{
			ReturnReference(target);

		}

	}
}

#define LIBRARYPACKETCODE 0x89
bool LanNetworkLibraryServer::CheckLibraryPacketCode(BYTE Code)
{

	if (Code != LIBRARYPACKETCODE)
	{
		return false;

	}

	return true;;
}

int LanNetworkLibraryServer::FindSession(__int64 session_ID)
{
	return (session_ID >> 48);
}

int LanNetworkLibraryServer::GetAcceptTPS()
{
	return accept_TPS;
}

int LanNetworkLibraryServer::GetRecvMessageTPS()
{
	return recv_message_TPS;
}

int LanNetworkLibraryServer::GetSendMessageTPS()
{
	return send_message_TPS;
}
