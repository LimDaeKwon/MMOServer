
#include "ContentsNetLibrary.h"
#include <iostream>
#include <process.h>
#include <winsock2.h>
#include "ws2tcpip.h"
#include <windows.h>
#include "Profiler.h"
#include <unordered_map>
#include "CPacket.h"
#include "ContentsCPacket.h"

#define LAN 0
#define NET 1


#pragma comment(lib,"ws2_32.lib")

#define RELEASEFLAG 0x80000000

unsigned int __stdcall ContentsNetLibrary::AcceptThread(LPVOID this_ptr)
{
	ContentsNetLibrary* this_for_Accept = (ContentsNetLibrary*)this_ptr;
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

		this_for_Accept->acceptTotal_++;
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
			//DebugBreak();

		}
		this_for_Accept->OnAccept(addrl, ntohs(clientaddr.sin_port), new_session->session_id);

		this_for_Accept->groupManager_.RegisterSession(new_session);
		//wprintf(L"Connect Client : IP  = %s , PORT = %d  , Session ID : %lld\n", addrl, ntohs(clientaddr.sin_port), this_for_Accept->unique_id);

		this_for_Accept->ReceiveFirst(new_session);

	}

	return 0;
}


unsigned int __stdcall ContentsNetLibrary::WorkerThread(LPVOID this_ptr)
{
	ContentsNetLibrary* this_for_worker = (ContentsNetLibrary*)this_ptr;
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

unsigned int __stdcall ContentsNetLibrary::MonitorThread(LPVOID this_ptr)
{
	ContentsNetLibrary* this_for_monitor = (ContentsNetLibrary*)this_ptr;

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

unsigned int __stdcall ContentsNetLibrary::HeartbeatThread(LPVOID this_ptr)
{

	ContentsNetLibrary* this_for_heartbeat = (ContentsNetLibrary*)this_ptr;
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

unsigned int __stdcall ContentsNetLibrary::SendThread(LPVOID this_ptr)
{
	ContentsNetLibrary* this_for_SendThread = (ContentsNetLibrary*)this_ptr;
	while (1)
	{
		for (unsigned int i = 0; i < this_for_SendThread->max_session; i++)
		{
			if (this_for_SendThread->session_array[i].use_flag)
			{
				this_for_SendThread->SendPost(&this_for_SendThread->session_array[i]);
			}

			
		}

	}


	return 0;
}



void ContentsNetLibrary::ThreadSendPost(Session* target)
{

	int local_count = InterlockedIncrement(&target->io_count);

	if ((local_count & RELEASEFLAG) == RELEASEFLAG)
	{
		if (InterlockedDecrement(&target->io_count) == 0)
		{
			Release(target);
		}
		return;
	}

	SendPost(target);

	if (InterlockedDecrement(&target->io_count) == 0)
	{
		Release(target);
	}

}





ContentsNetLibrary::ContentsNetLibrary()
	: accept_TPS(0), recv_message_TPS(0), send_message_TPS(0), accept_count(0),
	recv_message_count(0), send_message_count(0), max_session(0), session_num(0), threads_num(0), unique_id(0),
	index_list(0, false)

{

}

ContentsNetLibrary::~ContentsNetLibrary()
{
}


bool ContentsNetLibrary::Start(const char* server_IP, unsigned int  server_port, unsigned int worker_num, unsigned int concurrent_threads, unsigned int nagle, unsigned int sessions, unsigned int header, unsigned int sync, unsigned int sendthreads ,unsigned char packetCode)
{

	max_session = sessions;
	session_num = 0;
	session_array = new Session[max_session];
	header_size = header;
	int** temp = new int* [max_session];
	timeout = 30;
	unlogin_timeout = 3;
	packetCode_ = packetCode;


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

	if (sync)
	{
		DWORD OptionVal = 0;

		int socket_option_return = setsockopt(listen_sock, SOL_SOCKET, SO_SNDBUF, (const char*)&OptionVal, sizeof(OptionVal));
		if (socket_option_return == SOCKET_ERROR)
		{
			socket_option_return = WSAGetLastError();

			wprintf(L"SocketOptionReturn Error : %d \n", socket_option_return);
			DebugBreak();
		}

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


	sendThreads_ = sendthreads;


	for (unsigned int i = 0; i < sendthreads; i++)
	{
		(HANDLE)_beginthreadex(NULL, 0, SendThread, this, 0, NULL);
	}

	//sendThread_ = (HANDLE)_beginthreadex(NULL, 0, SendThread, this, 0, NULL);

	//if (sendThread_ == NULL)
	//{
	//	wprintf(L"_beginthreadex Failed \n");
	//	DebugBreak();
	//}

	//sendThread2_ = (HANDLE)_beginthreadex(NULL, 0, SendThread, this, 0, NULL);

	//if (sendThread2_ == NULL)
	//{
	//	wprintf(L"_beginthreadex Failed \n");
	//	DebugBreak();
	//}




	return true;
}

bool ContentsNetLibrary::Stop()
{

	PostQueuedCompletionStatus(handle_iocp, NULL, (ULONG_PTR)NULL, NULL);

	WaitForMultipleObjects(threads_num, threads, TRUE, INFINITE);

	closesocket(listen_sock);

	WaitForSingleObject(accept_thread, INFINITE);


	return true;
}



void ContentsNetLibrary::Disconnect(__int64 session_ID)
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
		PostQueuedCompletionStatus(handle_iocp, 1, (ULONG_PTR)target, NULL);
		//Release(target);
	}
	

}

Session* ContentsNetLibrary::SessionAlloc(int* empty_index, unsigned long long client_sock)
{

	if (empty_index == nullptr)
	{
		InterlockedDecrement(&session_num);
		return nullptr;
	}


	Session* new_session = &session_array[*empty_index];

	new_session->index = empty_index;
	__int64 i = *new_session->index;



	new_session->session_id = ++unique_id;
	new_session->session_id |= (i << 48);
	new_session->buffer_count.count = 0;
	new_session->sock = (SOCKET)client_sock;
	new_session->send_flag = FALSE;
	//new_session->disconnect_flag = FALSE;
	new_session->login_flag = FALSE;
	new_session->last_recv_time = GetTickCount64();
	
	InterlockedIncrement(&new_session->io_count);
	InterlockedExchange8((char*)&new_session->use_flag, 1);
	InterlockedAnd(&new_session->io_count, 0x7fffffff);
	InterlockedExchange8((char*)&new_session->disconnect_flag, 0);
	new_session->recv_overlapped.Type = RECV;
	new_session->send_overlapped.Type = SEND;


	return new_session;
}

void ContentsNetLibrary::SendCompletion(Session* target)
{

	for (int i = 0; i < target->buffer_count.count; ++i)
	{
		CPacket::Free(target->buffer_count.buffers[i]);
	}

	target->buffer_count.count = 0;
	InterlockedExchange8((char*)&target->send_flag, 0);
	SendPost(target);

}

void ContentsNetLibrary::RecvCompletion(Session* target, DWORD cbTransferred)
{
	target->login_flag = true;
	target->recv_buffer.MoveRear(cbTransferred);
	target->last_recv_time = GetTickCount64();
	RecvProc(target);
	Receive(target);
}



void ContentsNetLibrary::SendPacket(__int64 session_ID, ContentsCPacket contents_packet)
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
	CPacket* send_packet = contents_packet.packetBuffer_;

	send_packet->IncreaseRefCount();
	//인큐 , Send 
	if (!send_packet->encodingFlag_)
	{
		EnterCriticalSection(&send_packet->encodingLock_);

		if (!send_packet->encodingFlag_)
		{
			send_packet->encodingFlag_ = 1;
			NetAddHeader(send_packet);
		}
		LeaveCriticalSection(&send_packet->encodingLock_);
	}



	int enqueue_return = target->send_buffer.Enqueue(send_packet);

	InterlockedIncrement(&target->send_count);
	if (enqueue_return == false)
	{
		//SendBufferFull
		Disconnect(target->session_id); //리턴이 2든 아니든 일단 호출
		InterlockedIncrement(&DCSendBufferFull);

		if (InterlockedDecrement(&target->io_count) == 0)
		{
			PostQueuedCompletionStatus(handle_iocp, 1, (ULONG_PTR)target, NULL);
			//Release(target);
		}
		return;
	}

	if (!sendThreads_)
	{
		PostQueuedCompletionStatus(handle_iocp, 2, (ULONG_PTR)target, NULL);
	}

	//PostQueuedCompletionStatus(handle_iocp, 2, (ULONG_PTR)target, NULL);

	if (InterlockedDecrement(&target->io_count) == 0) 
	{
		PostQueuedCompletionStatus(handle_iocp, 1, (ULONG_PTR)target, NULL);
		//Release(target);
	}


}



void ContentsNetLibrary::SendPost(Session* target)
{

	if (InterlockedOr8((char*) & target->disconnect_flag, 0) == 1)
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



void ContentsNetLibrary::ReceiveFirst(Session* new_session)
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

#define MAXPACKETPACK 20

void ContentsNetLibrary::RecvProc(Session* target)
{
	int packetCount = 0;
	CPacket* packet_buffer = nullptr;
	CPacket* decodeBuffer = CPacket::Alloc();
	while (1)
	{

		int target_recv_buffer_size = target->recv_buffer.GetUseSize();
		NetPacketHeader header;

		//패킷코드 확인
		if (target_recv_buffer_size < sizeof(NetPacketHeader))
		{
			break;
		}
		if (target->recv_buffer.Peek((char*)&header, sizeof(NetPacketHeader)) != sizeof(NetPacketHeader))
		{
			//__debugbreak();
			break;
		}

		if (header.Code != packetCode_)
		{
			Disconnect(target->session_id);
			InterlockedIncrement(&DCPacketCodeError);
			break;
		}

		if (header.Len > 154) // 채팅서버에서 최대 나올 수 있는 메시지 사이즈 초과 //이런거 다 컨피그로 빼자. 라이브러리단에서 거를 수 있게
		{
			//ImpossiblePacketLength
			Disconnect(target->session_id);
			InterlockedIncrement(&DCImpossiblePacketLength);
			break;
		}

		if (target_recv_buffer_size < sizeof(header) + header.Len)
		{
			break;
		}

		if (packet_buffer == nullptr)
		{
			packet_buffer = CPacket::Alloc();
			packet_buffer->IncreaseRefCount();
		}

		unsigned int receive_dequeue_packet_size = target->recv_buffer.Dequeue(decodeBuffer->GetBufferPtr(), DKServerCore::PacketLibHeaderSize + header.Len);

		if (receive_dequeue_packet_size != header.Len + DKServerCore::PacketLibHeaderSize)
		{
			wprintf(L"## ReceiveQDequeuePacketSize != Header.BySize : %d \n", receive_dequeue_packet_size);
			//DebugBreak();
			break;
		}
		//지금 여기엔 인코딩 된 상태로 들어가 있음. 페이로드만. 
		//얘를 디코딩 해야해 

		if (decodeBuffer->Decode(decodeBuffer->GetReadPosition() - 1, header.Len + 1, header.RandKey) == false)
		{
			Disconnect(target->session_id);
			InterlockedIncrement(&DCDecodeError);
			CPacket::Free(decodeBuffer);
			break;
		}
		//decodeBuffer->IncreaseRefCount();
		decodeBuffer->MoveWritePosition(receive_dequeue_packet_size - DKServerCore::PacketLibHeaderSize);


		*packet_buffer << decodeBuffer->GetDataSize();
		packet_buffer->PutData(decodeBuffer->GetReadPosition(), decodeBuffer->GetDataSize());

		InterlockedIncrement(&recv_message_count);


		decodeBuffer->Clear();

		if (++packetCount == MAXPACKETPACK)
		{
			groupManager_.RouteMessage(target, (ContentsCPacket*)packet_buffer);

			CPacket::Free(packet_buffer);
			packet_buffer = nullptr;
			packetCount = 0;
		}
		//}
		//여기가 이제 바뀜. 
		//OnMessage(target->session_id, (ContentsCPacket*)packet_buffer);

	}
	if (packetCount)
	{
		groupManager_.RouteMessage(target, (ContentsCPacket*) packet_buffer);

		CPacket::Free(packet_buffer);
	}
	CPacket::Free(decodeBuffer);

}

void ContentsNetLibrary::Receive(Session* target)
{
	if (InterlockedOr8((char*)&target->disconnect_flag,0) == 1)
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


void ContentsNetLibrary::NetAddHeader(CPacket* packet_buffer)
{
	char* temp = packet_buffer->GetBufferPtr();
	NetPacketHeader NetHeader;
	NetHeader.Code = packetCode_;
	NetHeader.Len = packet_buffer->GetDataSize();
	NetHeader.RandKey = rand()%256;

	NetHeader.CheckSum = packet_buffer->Encode(packet_buffer->GetReadPosition(), NetHeader.Len, NetHeader.RandKey);
	memcpy_s(temp, DKServerCore::PacketLibHeaderSize, &NetHeader, DKServerCore::PacketLibHeaderSize);

}


void ContentsNetLibrary::Release(Session* target)
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
	groupManager_.LeaveGroup(target);

	InterlockedDecrement(&session_num);
	index_list.Free(target->index);
	

	return;
}



int* ContentsNetLibrary::FindEmptySession()
{
	if (InterlockedIncrement(&session_num) > max_session)
	{
		return nullptr;
	}

	return index_list.Alloc();

}

void ContentsNetLibrary::ClearSendBuffer(Session* target)
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

void ContentsNetLibrary::MoveGroup(__int64 sessionId, GroupId moveGroupId)
{
	
	unsigned int i = FindSession(sessionId);

	Session* target = &session_array[i];

	groupManager_.MoveGroup(target, moveGroupId);



}

int ContentsNetLibrary::FindSession(__int64 session_ID)
{
	return (session_ID >> 48);
}

int ContentsNetLibrary::GetAcceptTPS()
{
	return accept_TPS;
}

int ContentsNetLibrary::GetRecvMessageTPS()
{
	return recv_message_TPS;
}

int ContentsNetLibrary::GetSendMessageTPS()
{
	return send_message_TPS;
}

DWORD ContentsNetLibrary::GetDisconnectCount()
{
	return DisconnectCount;
}

DWORD ContentsNetLibrary::GetDCUnloginTimeout()
{
	return DCUnloginTimeout;
}

DWORD ContentsNetLibrary::GetDCLoginTimeout()
{
	return DCLoginTimeout;
}

DWORD ContentsNetLibrary::GetDCSendBufferFull()
{
	return DCSendBufferFull;
}

DWORD ContentsNetLibrary::GetDCPacketCodeError()
{
	return DCPacketCodeError;
}

DWORD ContentsNetLibrary::GetDCDecodeError()
{
	return DCDecodeError;
}

DWORD ContentsNetLibrary::GetDCSessionFull()
{
	return DCSessionFull;
}

DWORD ContentsNetLibrary::GetDCImpossiblePacketLength()
{
	return DCImpossiblePacketLength;
}

DWORD ContentsNetLibrary::GetSessionNum()
{
	return session_num;
}

unsigned long long ContentsNetLibrary::GetAcceptTotal()
{
	return acceptTotal_;
}
