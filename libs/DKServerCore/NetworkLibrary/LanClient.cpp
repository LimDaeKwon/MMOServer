#include "LanClient.h"

#include "winsock2.h"
#include "ws2tcpip.h"
#pragma comment(lib,"Ws2_32.lib")

//TODO : 이런 Define들은 헤더파일로 옮기기.

#define RELEASEFLAG 0x80000000

LanClient::LanClient()
{
}

LanClient::~LanClient()
{
	Stop();
}

bool LanClient::Start(const char* server_IP, unsigned int  server_port, unsigned int nagle, unsigned int header)
{
	header_size = header;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return false;
	}

	handle_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
	if (handle_iocp == NULL)
	{
		return false;
	}

	client_session = new ClientSession;

	client_session->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client_session->sock == INVALID_SOCKET)
	{
		int error = WSAGetLastError();
		wprintf(L"socket Error %d", error);
		return false;
	}

	DWORD zero = 0;
	setsockopt(client_session->sock, SOL_SOCKET, SO_SNDBUF, (const char*)&zero, sizeof(zero));

	LINGER linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;
	setsockopt(client_session->sock, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(linger));

	if (nagle)
	{
		DWORD NoDelay = 1;
		setsockopt(client_session->sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&NoDelay, sizeof(NoDelay));
	}

	// 주소
	SOCKADDR_IN server_address;
	ZeroMemory(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	InetPtonA(AF_INET, server_IP, &server_address.sin_addr);
	server_address.sin_port = htons(server_port);

	//3회까지 시도
	int connect_return = 0;
	for (int i = 0; i < 3; i++)
	{
		connect_return = connect(client_session->sock, (sockaddr*)&server_address, sizeof(server_address));
		if (connect_return == 0)
		{
			break;
		}

		connect_return = WSAGetLastError();
	}

	if (connect_return != 0)
	{
		DebugBreak();
	}



	if (CreateIoCompletionPort((HANDLE)client_session->sock, handle_iocp, (ULONG_PTR)client_session, 0) == 0)
	{
		int error = GetLastError();
		wprintf(L"CreateIoCompletionPort Error %d", error);
		DebugBreak();
		return false;
	}

	client_session->recv_overlapped.Type = RECV;
	client_session->send_overlapped.Type = SEND;
	client_session->io_count = 1;


	ReceiveFirst(client_session);


	worker_thread = (HANDLE)_beginthreadex(NULL, 0, ClientWorkerThread, this, 0, NULL);
	if (worker_thread == NULL)
	{
		wprintf(L"_beginthreadex Failed");
		DebugBreak();
		return false;
	}

	OnConnect();
	return true;

}

bool LanClient::Stop()
{
	if (handle_iocp)
		PostQueuedCompletionStatus(handle_iocp, 0, 0, NULL);

	if (worker_thread)
	{
		WaitForSingleObject(worker_thread, INFINITE);
		CloseHandle(worker_thread);
		worker_thread = NULL;
	}

	if (handle_iocp)
	{
		CloseHandle(handle_iocp);
		handle_iocp = NULL;
	}


	if (client_session->sock != INVALID_SOCKET)
	{
		closesocket(client_session->sock);
	}
		
	delete client_session;
	client_session = nullptr;


	WSACleanup();
	return true;
}

void LanClient::Close()
{

	if (InterlockedExchange8((char*)&client_session->close_flag, 1) == 1)
	{
		return;
	}

	CancelIoEx((HANDLE)client_session->sock, nullptr);
}



unsigned int __stdcall LanClient::ClientWorkerThread(LPVOID this_ptr)
{
	LanClient* this_for_worker = (LanClient*)this_ptr;

	while (1)
	{
		DWORD cbTransferred = 0;
		MyOverlapped* overlap_ptr;
		ClientSession* target = nullptr;
		int retval;
		retval = GetQueuedCompletionStatus(this_for_worker->handle_iocp, &cbTransferred, (PULONG_PTR)&target, (LPOVERLAPPED*)&overlap_ptr, INFINITE);

		//IOCP 종료 메시지 이건 누가날려주지? 소멸자에서 날려주나? 
		if (overlap_ptr == NULL && cbTransferred == NULL && target == NULL)
		{
			break;
		}

		//IO 문제 발생. 
		if (retval == 0)
		{
			int error = WSAGetLastError();
			if (!(error == 64 || error == 995)) // 다른 특이한 에러가 있는지 보기 위해서 하나씩 추가.
			{
				//64 : 연결을 끊음.
				//995 : CancelIoEx에 의한 취소.
				//__debugbreak();
			}

		}
		else
		{
			if (overlap_ptr->Type == RECV)
			{
				this_for_worker->RecvCompletion(target, cbTransferred);
			}
			if (overlap_ptr->Type == SEND)
			{
				this_for_worker->SendCompletion(target);
			}
		}

		if (InterlockedDecrement(&target->io_count) == 0)
		{
			this_for_worker->Release(target);
			//Release시점은 서버와의 연결 종료시점..? 
		}


	}
	return 0;
}


void LanClient::SendPacket(ContentsCPacket contents_packet)
{

	ClientSession* target = client_session;
	int local_count = InterlockedIncrement(&target->io_count);

	if ((local_count & RELEASEFLAG) == RELEASEFLAG)
	{
		if (InterlockedDecrement(&target->io_count) == 0)
		{
			Release(target);
		}
		return;
	}


	//인큐 , Send 
	CPacket* send_packet = contents_packet.packetBuffer_;

	AddHeader(send_packet);
	send_packet->IncreaseRefCount();

	int enqueue_return = target->send_buffer.Enqueue(send_packet);

	InterlockedIncrement(&target->send_count);
	if (enqueue_return == false)
	{
		wprintf(L"EnqueueFail in SendPacketUnicast Client \n");
		//DebugBreak();
		return;
	}

	SendPost(target);

	if (InterlockedDecrement(&target->io_count) == 0)
	{
		Release(target);
	}

}


void LanClient::ReceiveFirst(ClientSession* new_session)
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

			wprintf(L"In First WSARecvError : %d  Client \n", WSARecv_error);

			if (InterlockedDecrement(&new_session->io_count) == 0)
			{
				Release(new_session);
			}
		}
	}
}


void LanClient::Release(ClientSession* target)
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


	closesocket(target->sock);
	target->sock = INVALID_SOCKET;



	return;
}
void LanClient::SendPost(ClientSession* target)
{
	
	
	
	
	if (target->close_flag == 1)
	{
		return;
	}


	if (InterlockedExchange8((char*)&target->send_flag, 1) != 0)
	{
		return;

	}


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
		local_wsabuf[buf_count].buf = temp->GetBufferPtr();
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
		return;
	}
	InterlockedAdd(&send_message_count, target->send_count);

	InterlockedExchange(&target->send_count, 0);
	

	DWORD sendbytes = 0;
	InterlockedIncrement(&target->io_count);


	ZeroMemory(&target->send_overlapped.overlapped, sizeof(target->send_overlapped.overlapped));

	int WSASend_return;
	WSASend_return = WSASend(target->sock, local_wsabuf, buf_count, &sendbytes, 0, &target->send_overlapped.overlapped, NULL);
	if (WSASend_return == SOCKET_ERROR)
	{
		int WSASendError = WSAGetLastError();

		if (WSASendError == WSA_IO_PENDING)
		{

			if (target->close_flag == 1)
			{
				CancelIoEx((HANDLE)target->sock, nullptr);


			}

		}
		else
		{
			if (WSASendError == 10038)
			{
				//DebugBreak();
			}
			if (InterlockedDecrement(&target->io_count) == 0)
			{
				Release(target);
				return;
			}
		}
	}

}

void LanClient::SendCompletion(ClientSession* target)
{
	for (int i = 0; i < target->buffer_count.count; ++i)
	{
		CPacket::Free(target->buffer_count.buffers[i]);
	}
	target->buffer_count.count = 0;
	InterlockedExchange8((char*)&target->send_flag, 0);
	SendPost(target);
}

void LanClient::RecvCompletion(ClientSession* target, DWORD cbTransferred)
{

	target->recv_buffer.MoveRear(cbTransferred);
	RecvProc(target);
	Receive(target);
}

void LanClient::RecvProc(ClientSession* target)
{
	while (1)
	{
		int target_recv_buffer_size = target->recv_buffer.GetUseSize();
		PacketHeader header;
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

		target->recv_buffer.MoveFront(sizeof(header));

		CPacket* packet_buffer = CPacket::Alloc();
		unsigned int receive_dequeue_packet_size = target->recv_buffer.Dequeue(packet_buffer->GetBufferPtr() + DKServerCore::PacketLibHeaderSize, header.length);

		if (receive_dequeue_packet_size != header.length)
		{
			OnError(0, L"## ReceiveQDequeuePacketSize != Header.BySize (Client)");
			CPacket::Free(packet_buffer);
			break;
		}

		packet_buffer->IncreaseRefCount();
		packet_buffer->MoveWritePosition(receive_dequeue_packet_size);
		OnMessage((ContentsCPacket*)packet_buffer);
		CPacket::Free(packet_buffer);
	}
}


void LanClient::Receive(ClientSession* target)
{
	if (target->close_flag)
	{
		return;
	}

	WSABUF recv_wsabuf[2];
	recv_wsabuf[0].buf = target->recv_buffer.GetRearBufferPtr();
	recv_wsabuf[0].len = target->recv_buffer.DirectEnqueueSize();
	recv_wsabuf[1].buf = target->recv_buffer.GetStartBufferPtr();
	recv_wsabuf[1].len = target->recv_buffer.GetFreeSize() - target->recv_buffer.DirectEnqueueSize();

	DWORD recvbytes = 0;
	DWORD flags = 0;

	ZeroMemory(&target->recv_overlapped.overlapped, sizeof(target->recv_overlapped.overlapped));
	InterlockedIncrement(&target->io_count);

	int retval = WSARecv(target->sock, recv_wsabuf, 2, &recvbytes, &flags, &target->recv_overlapped.overlapped, 0);
	if (retval == SOCKET_ERROR)
	{
		int WSARecv_error = WSAGetLastError();
		if (WSARecv_error == WSA_IO_PENDING)
		{
			if (target->close_flag == 1)
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

void LanClient::AddHeader(CPacket* packet_buffer)
{
	char* temp = packet_buffer->GetBufferPtr();

	unsigned short len = (unsigned short)packet_buffer->GetDataSize();
	(*(unsigned short*)(temp)) = len;

}

void LanClient::ClearSendBuffer(ClientSession* target)
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