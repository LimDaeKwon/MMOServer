#include "LanLibrary.h"
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

#define LAN 0
#define NET 1



#pragma comment(lib,"ws2_32.lib")

#define RELEASEFLAG 0x80000000

#define dfPACKET_CODE		0x77





unsigned int __stdcall LanLibrary::AcceptThread(LPVOID this_ptr)
{
	LanLibrary* this_for_Accept = (LanLibrary*)this_ptr;
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
			//DebugBreak();

		}


		Session* new_session = this_for_Accept->SessionAlloc(this_for_Accept->FindEmptySession(), client_sock);

		if (new_session == nullptr)
		{
			//sessionfull
			this_for_Accept->DCSessionFull++;
			closesocket(client_sock);
			continue;
		}

		InterlockedIncrement(&this_for_Accept->accept_count);

		CreateIoCompletionPort((HANDLE)client_sock, this_for_Accept->handle_iocp, (ULONG_PTR)new_session, 0);

		sockaddr_in clientaddr;
		int addr_len = sizeof(clientaddr);
		getpeername(client_sock, (SOCKADDR*)&clientaddr, &addr_len);
		WCHAR addrl[INET_ADDRSTRLEN];

		if (InetNtopW(AF_INET, &clientaddr.sin_addr, addrl, INET_ADDRSTRLEN) == NULL)
		{

			wprintf(L"InetNtop Error \n");
			DebugBreak();

		}
		this_for_Accept->OnAccept(addrl, ntohs(clientaddr.sin_port), new_session->session_id);

		//wprintf(L"Connect Client : IP  = %s , PORT = %d  , Session ID : %lld\n", addrl, ntohs(clientaddr.sin_port), this_for_Accept->unique_id);

		this_for_Accept->ReceiveFirst(new_session);

	}

	return 0;
}


unsigned int __stdcall LanLibrary::WorkerThread(LPVOID this_ptr)
{
	LanLibrary* this_for_worker = (LanLibrary*)this_ptr;
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
		if (cbTransferred == 0)
		{
			//정상종료


		}
		//IO 문제 발생. 
		else if (retval == 0)
		{
			int error = WSAGetLastError();
			if (!(error == 64 || error == 995 || error == 1236 || error == 121)) // 다른 특이한 에러가 있는지 보기 위해서 하나씩 추가. 
			{
				//64 : 연결을 끊음.
				//995 : CancelIoEx에 의한 취소.
				//1236 : 로컬rst로 추정
				//121 : 연결 끊김 파악
				//__debugbreak();
			}

		}
		else
		{
			if ((overlap_ptr == NULL && cbTransferred == 2 && target != NULL))
			{
				this_for_worker->SendPost(target);
				continue;
			}
			else if ((overlap_ptr == NULL && cbTransferred == 1 && target != NULL))
			{
				this_for_worker->Release(target);
				continue;
			}
			else if (overlap_ptr->Type == RECV)
			{
				//Profile T(L"RecvCompletion");
				this_for_worker->RecvCompletion(target, cbTransferred);
			}
			else if (overlap_ptr->Type == SEND)
			{
				//Profile T(L"SendCompletion");
				this_for_worker->SendCompletion(target);
			}



		}

		if (InterlockedDecrement(&target->io_count) == 0)
		{

			this_for_worker->Release(target);
		}


	}
	return 0;
}

unsigned int __stdcall LanLibrary::MonitorThread(LPVOID this_ptr)
{
	LanLibrary* this_for_monitor = (LanLibrary*)this_ptr;

	unsigned int oldTick = timeGetTime();

	while (1)
	{

		this_for_monitor->accept_TPS = this_for_monitor->accept_count;
		this_for_monitor->recv_message_TPS = this_for_monitor->recv_message_count;
		this_for_monitor->send_message_TPS = this_for_monitor->send_message_count;

		InterlockedExchange(&this_for_monitor->accept_count, 0);
		InterlockedExchange(&this_for_monitor->recv_message_count, 0);
		InterlockedExchange(&this_for_monitor->send_message_count, 0);

		this_for_monitor->OnInitializeTPS();

		unsigned int tick = timeGetTime();
		unsigned int frame = tick - oldTick;
		oldTick += 1000;

		if (frame < 1000)
		{
			Sleep(1000 - frame);
		}

	}




	return 0;
}

unsigned int __stdcall LanLibrary::HeartbeatThread(LPVOID this_ptr)
{

	LanLibrary* this_for_heartbeat = (LanLibrary*)this_ptr;
	unsigned int local_count = 0;
	while (1)
	{
		//if (local_count % this_for_heartbeat->unlogin_timeout == 0)
		//{
		//	for (unsigned int i = 0; i < this_for_heartbeat->max_session; ++i)
		//	{
		//		Session* target = &this_for_heartbeat->session_array[i];
		//		if (target->use_flag == 0)
		//		{
		//			continue;
		//		}
		//		if (target->login_flag == 0)
		//		{
		//			continue;
		//		}
		//		if (GetTickCount64() - target->last_recv_time > this_for_heartbeat->unlogin_timeout * 1000) // 3초
		//		{
		//			this_for_heartbeat->Disconnect(target->session_id);
		//			InterlockedIncrement(&this_for_heartbeat->DCUnloginTimeout);
		//		}

		//	}
		//}

		//if (local_count % this_for_heartbeat->timeout == 0)
		//{
		//	for (unsigned int i = 0; i < this_for_heartbeat->max_session; ++i)
		//	{
		//		Session* target = &this_for_heartbeat->session_array[i];
		//		if (target->use_flag == 0)
		//		{
		//			continue;
		//		}
		//		if (target->login_flag == 1)
		//		{
		//			continue;
		//		}
		//		if (GetTickCount64() - target->last_recv_time > this_for_heartbeat->timeout * 1000) // 30초
		//		{
		//			this_for_heartbeat->Disconnect(target->session_id);
		//			InterlockedIncrement(&this_for_heartbeat->DCLoginTimeout);
		//		}
		//	}

		//}


		local_count++;
		Sleep(1000);
	}




	return 0;
}






LanLibrary::LanLibrary()
	: accept_TPS(0), recv_message_TPS(0), send_message_TPS(0), accept_count(0),
	recv_message_count(0), send_message_count(0), max_session(0), session_num(0), threads_num(0), unique_id(0),
	index_list(0, false)

{

}

LanLibrary::~LanLibrary()
{
}


bool LanLibrary::Start(const char* server_IP, unsigned int  server_port, unsigned int worker_num, unsigned int concurrent_threads, unsigned int nagle, unsigned int sessions, unsigned int header)
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
	handle_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, concurrent_threads);
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
	DWORD OptionVal = 0;

	int socket_option_return = setsockopt(listen_sock, SOL_SOCKET, SO_SNDBUF, (const char*)&OptionVal, sizeof(OptionVal));
	if (socket_option_return == SOCKET_ERROR)
	{
		socket_option_return = WSAGetLastError();

		wprintf(L"SocketOptionReturn Error : %d \n", socket_option_return);
		DebugBreak();
	}



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

bool LanLibrary::Stop()
{

	PostQueuedCompletionStatus(handle_iocp, NULL, (ULONG_PTR)NULL, NULL);

	WaitForMultipleObjects(threads_num, threads, TRUE, INFINITE);

	closesocket(listen_sock);

	WaitForSingleObject(accept_thread, INFINITE);


	return true;
}



void LanLibrary::Disconnect(__int64 session_ID)
{


	Session* target;
	unsigned int i = FindSession(session_ID);
	target = &session_array[i];


	int local_count = InterlockedIncrement(&target->io_count);
	//릴리즈 플래그 확인.

	if ((local_count & RELEASEFLAG) == RELEASEFLAG)
	{

		if (InterlockedDecrement(&target->io_count) == 0)
		{
			//PostQueuedCompletionStatus(handle_iocp, 1, (ULONG_PTR)target, NULL);
			Release(target);
		}
		return;
	}

	//이러면 해제 이후 다시 재사용 된 상태. 다시 카운트 감소시키고 리턴시키기. 
	if (target->session_id != session_ID)
	{
		//근데 생각해보니 재사용되었고 또 삭제되어야 했을 수도 있음. 
		//목적과 다른 녀석이지만 얘도 내가 증가시켰으므로 감소 후 지워주기. 
		if (InterlockedDecrement(&target->io_count) == 0)
		{
			//PostQueuedCompletionStatus(handle_iocp, 1, (ULONG_PTR)target, NULL);
			Release(target);
		}
		return;
	}

	//if (InterlockedExchange8((char*) & target->disconnect_flag, 1) == 1)
	//{
	//	if (InterlockedDecrement(&target->io_count) == 0)
	//	{
	//		PostQueuedCompletionStatus(handle_iocp, 1, (ULONG_PTR)target, NULL);
	//		//Release(target);
	//	}
	//	return;
	//}

	InterlockedIncrement(&DisconnectCount);
	InterlockedExchange8((char*)&target->disconnect_flag, 1);
	CancelIoEx((HANDLE)target->sock, nullptr);

	if (InterlockedDecrement(&target->io_count) == 0)
	{
		//PostQueuedCompletionStatus(handle_iocp, 1, (ULONG_PTR)target, NULL);
		Release(target);
	}


}

LanLibrary::Session* LanLibrary::SessionAlloc(int* empty_index, unsigned long long client_sock)
{
	Session* new_session = &session_array[*empty_index];

	new_session->index = empty_index;



	if (InterlockedIncrement(&session_num) > max_session)
	{
		return nullptr;
	}
	__int64 i = *new_session->index;



	new_session->session_id = ++unique_id;
	new_session->session_id |= (i << 48);
	new_session->buffer_count.count = 0;
	new_session->sock = (SOCKET)client_sock;
	new_session->send_flag = FALSE;
	//new_session->disconnect_flag = FALSE;
	new_session->use_flag = TRUE;
	new_session->login_flag = FALSE;
	new_session->last_recv_time = GetTickCount64();

	InterlockedIncrement(&new_session->io_count);
	InterlockedAnd(&new_session->io_count, 0x7fffffff);
	InterlockedExchange8((char*)&new_session->disconnect_flag, 0);
	new_session->recv_overlapped.Type = RECV;
	new_session->send_overlapped.Type = SEND;


	return new_session;
}

void LanLibrary::SendCompletion(Session* target)
{

	for (int i = 0; i < target->buffer_count.count; ++i)
	{
		CPacket::Free(target->buffer_count.buffers[i]);
	}

	target->buffer_count.count = 0;
	InterlockedExchange8((char*)&target->send_flag, 0);
	SendPost(target);

}

void LanLibrary::RecvCompletion(Session* target, DWORD cbTransferred)
{
	target->login_flag = true;
	target->recv_buffer.MoveRear(cbTransferred);
	target->last_recv_time = GetTickCount64();
	RecvProc(target);
	Receive(target);
}



void LanLibrary::SendPacket(__int64 session_ID, ContentsCPacket contents_packet)
{

	Session* target;
	unsigned int i = FindSession(session_ID);

	target = &session_array[i];
	int local_count = InterlockedIncrement(&target->io_count);

	if ((local_count & RELEASEFLAG) == RELEASEFLAG)
	{
		if (InterlockedDecrement(&target->io_count) == 0)
		{
			PostQueuedCompletionStatus(handle_iocp, 1, (ULONG_PTR)target, NULL);
			//Release(target);
		}
		return;
	}

	//이러면 해제 이후 다시 재사용 된 상태. 다시 카운트 감소시키고 리턴시키기. 
	if (target->session_id != session_ID)
	{
		//근데 생각해보니 재사용되었고 또 삭제되어야 했을 수도 있음. 
		//목적과 다른 녀석이지만 얘도 내가 증가시켰으므로 감소 후 지워주기. 
		if (InterlockedDecrement(&target->io_count) == 0)
		{
			PostQueuedCompletionStatus(handle_iocp, 1, (ULONG_PTR)target, NULL);
			//Release(target);
		}
		return;
	}
	CPacket* send_packet = contents_packet.packet_buffer;

	send_packet->IncreaseRefCount();
	int enqueue_return = target->send_buffer.Enqueue(send_packet);

	InterlockedIncrement(&target->send_count);
	if (enqueue_return == false)
	{
		//SendBufferFull
		wprintf(L"EnqueueFail in SendPacketUnicast Session Id : %lld\n", target->session_id);
		Disconnect(target->session_id); //리턴이 2든 아니든 일단 호출
		InterlockedIncrement(&DCSendBufferFull);

		if (InterlockedDecrement(&target->io_count) == 0)
		{
			PostQueuedCompletionStatus(handle_iocp, 1, (ULONG_PTR)target, NULL);
		}
		return;
	}

	PostQueuedCompletionStatus(handle_iocp, 2, (ULONG_PTR)target, NULL);

	if (InterlockedDecrement(&target->io_count) == 0)
	{
		PostQueuedCompletionStatus(handle_iocp, 1, (ULONG_PTR)target, NULL);
	}


}



void LanLibrary::SendPost(Session* target)
{

	if (InterlockedOr8((char*)&target->disconnect_flag, 0) == 1)
	{
		return;
	}

	long local_count = InterlockedIncrement(&target->io_count);

	if ((local_count & RELEASEFLAG) == RELEASEFLAG)
	{
		if (InterlockedDecrement(&target->io_count) == 0)
		{
			Release(target);
		}
		return;
	}



	int WSASend_return;

	if (InterlockedExchange8((char*)&target->send_flag, 1) == 0)
	{

		WSABUF local_wsabuf[MAXBATCHSIZE];

		int buf_count = 0;


		while (buf_count < MAXBATCHSIZE)
		{
			CPacket* temp = nullptr;

			if (target->send_buffer.Dequeue(&temp) == false)
			{
				break;
			}
			target->buffer_count.buffers[buf_count] = temp;
			local_wsabuf[buf_count].buf = temp->GetBufferPtr() + DKServerCore::PacketLibHeaderSize - header_size;
			local_wsabuf[buf_count].len = temp->GetDataSize() + header_size;
			buf_count++;

		}
		target->buffer_count.count = buf_count;

		if (buf_count == 0)
		{
			InterlockedExchange8((char*)&target->send_flag, 0);
			if (target->send_buffer.GetSize() != 0)
			{
				SendPost(target);
			}

			//send를 안 할꺼니까 감소시키기.
			if (InterlockedDecrement(&target->io_count) == 0)
			{
				Release(target);
			}

			return;
		}

		InterlockedAdd(&send_message_count, target->send_count);
		InterlockedExchange(&target->send_count, 0);


		DWORD sendbytes = 0;
		ZeroMemory(&target->send_overlapped.overlapped, sizeof(target->send_overlapped.overlapped));

		WSASend_return = WSASend(target->sock, local_wsabuf, buf_count, &sendbytes, 0, &target->send_overlapped.overlapped, NULL);
		if (WSASend_return == SOCKET_ERROR)
		{
			int WSASendError = WSAGetLastError();

			if (WSASendError == WSA_IO_PENDING)
			{

				if (InterlockedOr8((char*)&target->disconnect_flag, 0) == 1)
				{
					CancelIoEx((HANDLE)target->sock, nullptr);
				}

			}

			if (WSASendError != WSA_IO_PENDING)
			{
				if (WSASendError == 10038)
				{
					//DebugBreak();
				}
				if (InterlockedDecrement(&target->io_count) == 0)
				{
					//PostQueuedCompletionStatus(handle_iocp, 1, (ULONG_PTR)target, NULL);
					Release(target);
					return;
				}
			}
		}

	}
	else
	{
		if (InterlockedDecrement(&target->io_count) == 0)
		{
			Release(target);
		}
	}

}



void LanLibrary::ReceiveFirst(Session* new_session)
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

			if (InterlockedDecrement(&new_session->io_count) == 0)
			{
				Release(new_session);
			}
		}
	}
}


//
//void LanLibrary::RecvProc(Session* target)
//{
//
//	//dfPACKET_CODE
//	while (1)
//	{
//		int target_recv_buffer_size = target->recv_buffer.GetUseSize();
//		NetPacketHeader header;
//
//		//패킷코드 확인
//		if (target_recv_buffer_size < sizeof(header.Code))
//		{
//			break;
//		}
//		if (target->recv_buffer.Peek((char*)&header.Code, sizeof(header.Code)) != sizeof(header.Code))
//		{
//			__debugbreak();
//			break;
//		}
//		if (header.Code != dfPACKET_CODE)
//		{
//			Disconnect(target->session_id);
//			InterlockedIncrement(&DCPacketCodeError);
//			break;
//		}
//		//Code(1byte) - Len(2byte) - RandKey(1byte) - CheckSum(1byte) - Payload(Len byte)
//
//		if (target_recv_buffer_size < sizeof(header))
//		{
//			break;
//		}
//
//		if (target->recv_buffer.Peek((char*)&header, sizeof(header)) != sizeof(header))
//		{
//			break;
//		}
//
//		if (target_recv_buffer_size < sizeof(header) + header.Len)
//		{
//			break;
//		}
//
//		if (header.Len > 154) // 채팅서버에서 최대 나올 수 있는 메시지 사이즈 초과 
//		{
//			//ImpossiblePacketLength
//			Disconnect(target->session_id);
//			InterlockedIncrement(&DCImpossiblePacketLength);
//			break;
//		}
//
//		//unsigned int receive_dequeue_header_size = target->recv_buffer.MoveFront(sizeof(header));
//
//
//		CPacket* packet_buffer = CPacket::Alloc();
//
//		unsigned int receive_dequeue_packet_size = target->recv_buffer.Dequeue(packet_buffer->GetBufferPtr(), LIBHEADERSIZE + header.Len);
//
//		if (receive_dequeue_packet_size != header.Len + LIBHEADERSIZE)
//		{
//			wprintf(L"## ReceiveQDequeuePacketSize != Header.BySize : %d \n", receive_dequeue_packet_size);
//			DebugBreak();
//			break;
//		}
//		//지금 여기엔 인코딩 된 상태로 들어가 있음. 페이로드만. 
//		//얘를 디코딩 해야해 
//		
//		if (packet_buffer->Decode(packet_buffer->GetReadPosition() - 1, header.Len + 1, header.RandKey) == false)
//		{
//			Disconnect(target->session_id);
//			InterlockedIncrement(&DCDecodeError);
//			CPacket::Free(packet_buffer);
//			break;
//		}
//		packet_buffer->IncreaseRefCount();
//		packet_buffer->MoveWritePosition(receive_dequeue_packet_size - LIBHEADERSIZE);
//		InterlockedIncrement(&recv_message_count);
//		OnMessage(target->session_id, (ContentsCPacket*)packet_buffer);
//		CPacket::Free(packet_buffer);
//	}
//
//
//}


void LanLibrary::RecvProc(Session* target)
{

	//dfPACKET_CODE
	while (1)
	{
		int target_recv_buffer_size = target->recv_buffer.GetUseSize();
		PacketHeader header;

		//패킷코드 확인
		if (target_recv_buffer_size < sizeof(PacketHeader))
		{
			break;
		}
		if (target->recv_buffer.Peek((char*)&header, sizeof(PacketHeader)) != sizeof(PacketHeader))
		{
			//__debugbreak();
			break;
		}

		if (target_recv_buffer_size < sizeof(header) + header.length)
		{
			break;
		}


		//unsigned int receive_dequeue_header_size = target->recv_buffer.MoveFront(sizeof(header));


		CPacket* packetBuffer = CPacket::Alloc();
		packetBuffer->InitLan();

		unsigned int receive_dequeue_packet_size = target->recv_buffer.Dequeue(packetBuffer->GetBufferPtr(),header.length+2);

		if (receive_dequeue_packet_size != header.length + 2)
		{
			wprintf(L"## ReceiveQDequeuePacketSize != Header.BySize : %d \n", receive_dequeue_packet_size);
			//DebugBreak();
			break;
		}

		packetBuffer->IncreaseRefCount();
		packetBuffer->MoveWritePosition(receive_dequeue_packet_size - 2);
		InterlockedIncrement(&recv_message_count);
		OnMessage(target->session_id, (ContentsCPacket*)packetBuffer);
		CPacket::Free(packetBuffer);
	}


}

void LanLibrary::Receive(Session* target)
{
	if (InterlockedOr8((char*)&target->disconnect_flag, 0) == 1)
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
	int WSARecv_error;



	ZeroMemory(&target->recv_overlapped.overlapped, sizeof(target->recv_overlapped.overlapped));
	InterlockedIncrement(&target->io_count);
	retval = WSARecv(target->sock, recv_wsabuf, 2, &recvbytes, &flags, &target->recv_overlapped.overlapped, 0);
	if (retval == SOCKET_ERROR)
	{
		WSARecv_error = WSAGetLastError();
		if (WSARecv_error == WSA_IO_PENDING)
		{
			if (InterlockedOr8((char*)&target->disconnect_flag, 0) == 1)
			{
				CancelIoEx((HANDLE)target->sock, nullptr);
			}
		}
		else
		{
			if (InterlockedDecrement(&target->io_count) == 0)
			{
				Release(target);
			}

		}

	}

}

void LanLibrary::AddHeader(CPacket* packet_buffer)
{
	char* temp = packet_buffer->GetBufferPtr();
	temp += DKServerCore::PacketLibHeaderSize - header_size;
	PacketHeader LibHeader;
	LibHeader.length = packet_buffer->GetDataSize();

	(*(unsigned short*)(temp)) = LibHeader.length;
}

void LanLibrary::Release(Session* target)
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



int* LanLibrary::FindEmptySession()
{
	int* temp_index = index_list.Alloc();
	return temp_index;

}

void LanLibrary::ClearSendBuffer(Session* target)
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

int LanLibrary::FindSession(__int64 session_ID)
{
	return (session_ID >> 48);
}

int LanLibrary::GetAcceptTPS()
{
	return accept_TPS;
}

int LanLibrary::GetRecvMessageTPS()
{
	return recv_message_TPS;
}

int LanLibrary::GetSendMessageTPS()
{
	return send_message_TPS;
}

DWORD LanLibrary::GetDisconnectCount()
{
	return DisconnectCount;
}

DWORD LanLibrary::GetDCUnloginTimeout()
{
	return DCUnloginTimeout;
}

DWORD LanLibrary::GetDCLoginTimeout()
{
	return DCLoginTimeout;
}

DWORD LanLibrary::GetDCSendBufferFull()
{
	return DCSendBufferFull;
}

DWORD LanLibrary::GetDCPacketCodeError()
{
	return DCPacketCodeError;
}

DWORD LanLibrary::GetDCDecodeError()
{
	return DCDecodeError;
}

DWORD LanLibrary::GetDCSessionFull()
{
	return DCSessionFull;
}

DWORD LanLibrary::GetDCImpossiblePacketLength()
{
	return DCImpossiblePacketLength;
}

DWORD LanLibrary::GetSessionNum()
{
	return session_num;
}
