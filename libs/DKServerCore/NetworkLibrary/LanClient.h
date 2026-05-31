#pragma once

#include <winsock2.h>
#include "LockFreeObjectFreeList.h"
#include "ContentsCPacket.h"
#include "LockFreeQueueCas2.h"
#include "RingBuffer.h"
#define RECV 10
#define SEND 20

#define MAXBATCHSIZE 100
class LanClient
{
public:
	LanClient();

	virtual ~LanClient();

	struct PacketHeader
	{
		unsigned short length;
	};

	struct MyOverlapped
	{
		WSAOVERLAPPED overlapped;
		int Type;
	};

	struct BufferCount
	{
		CPacket* buffers[MAXBATCHSIZE];
		long count = 0;
	};


	struct ClientSession
	{
		TLockFreeQueue<CPacket*> send_buffer;

		MyOverlapped send_overlapped;
		MyOverlapped recv_overlapped;

		RingBuffer recv_buffer;

		SOCKET sock = INVALID_SOCKET;

		bool close_flag = 0;	// 클라 자발적 종료
		bool send_flag = 0;
		long io_count = 0;
		long send_count = 0;

		BufferCount buffer_count;
	};



	bool Start(const char* server_IP, unsigned int  server_port, unsigned int nagle, unsigned int header_size);
	bool Stop();
	int GetSessionCount();
	void Close(); // Disconnect의 클라이언트 버전. 자기 자신 종료
	ClientSession* SessionAlloc(int* empty_index, unsigned long long client_sock);

	void SendCompletion(ClientSession* target);
	void RecvCompletion(ClientSession* target, DWORD cbTransferred);

	void SendPacket(ContentsCPacket send_packet);
	void SendPost(ClientSession* Target);

	void ReceiveFirst(ClientSession* new_session);
	void RecvProc(ClientSession* target);
	void Receive(ClientSession* target);
	void AddHeader(CPacket* packet_buffer);
	void Release(ClientSession* target);


	void ClearSendBuffer(ClientSession* target);

	static unsigned int WINAPI ClientWorkerThread(LPVOID this_ptr);



	virtual void OnConnect() = 0;
	virtual void OnRelease() = 0;
	virtual void OnMessage(ContentsCPacket* send_packet) = 0;
	virtual void OnError(int errorcode, const wchar_t* error_log) = 0;




	HANDLE handle_iocp = nullptr;
	HANDLE worker_thread = nullptr;

	ClientSession* client_session = nullptr;

	unsigned int header_size = 0;

	long send_message_count = 0;


};
