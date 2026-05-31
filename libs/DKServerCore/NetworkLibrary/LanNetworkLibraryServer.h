#pragma once

#include <winsock2.h>
#include "LockFreeObjectFreeList.h"
#include "ContentsCPacket.h"
#include "LockFreeQueueCas2.h"
#include "RingBuffer.h"
#define RECV 10
#define SEND 20
#define LAN 0
#define NET 1
#define MAXBATCHSIZE 250

class LanNetworkLibraryServer
{
public:
	LanNetworkLibraryServer();

	virtual ~LanNetworkLibraryServer();

	struct LanPacketHeader
	{
		unsigned short length;
	};

#pragma pack(push,1)
	struct NetPacketHeader
	{
		BYTE byCode;			// 패킷코드 0x89 고정.
		BYTE bySize;			// 패킷 사이즈.
	};
#pragma pack(pop)

	struct MyOverlapped
	{
		WSAOVERLAPPED overlapped;
		int Type;
	};

	struct BufferCount
	{
		CPacket* buffers[250];
		long count;
	};

	struct Session
	{
		TLockFreeQueue<CPacket*> send_buffer;

		MyOverlapped send_overlapped;
		MyOverlapped recv_overlapped;

		RingBuffer recv_buffer;

		SOCKET sock;
		__int64 session_id = 0;
		bool disconnect_flag = 0;
		bool send_flag = 0;
		long io_count = 0;
		long send_count = 0; // sendTPS용

		BufferCount buffer_count; // 직렬화버퍼 지우기용.
		
		unsigned long long last_recv_time = 0; // Heartbeat용
		bool use_flag = 0;
		bool login_flag = 0;

		int* index;//인덱스 저장용

	};

	

	bool Start(const char* server_IP, unsigned int  server_port, unsigned int threads_count, unsigned int concurrent_threads, unsigned int nagle, unsigned int sessions , unsigned int header_size);
	bool Stop();
	int GetSessionCount();
	void Disconnect(__int64 session_ID);
	Session* SessionAlloc(int* empty_index , unsigned long long client_sock);

	void SendCompletion(Session* target);
	void RecvCompletion(Session* target , DWORD cbTransferred);

	void SendPacket(__int64 session_ID, CPacket* send_packet);
	void SendPost(Session* Target);

	void ReceiveFirst(Session* new_session);
	void RecvProc(Session* target);
	void Receive(Session* target);
	void LanAddHeader(CPacket* packet_buffer);
	void NetAddHeader(CPacket* packet_buffer);

	void Release(Session* target);

	//IOCP 생성과 등록 구분하기. 
	void RegisterIOCP(HANDLE new_socket, ULONG_PTR key);
	HANDLE CreateIOCP(DWORD cuncurrent);


	//소유권 반환
	void ReturnReference(Session* target);

	//WSA버퍼 세팅
	int SetWSABUF(Session* target, WSABUF* wsabuf);

	//
	void GetClientAddress(SOCKET client_socket, sockaddr_in& client_addr, WCHAR* addrl);

	//
	void RecursiveCheck(Session* target);

	//
	void CheckSendReturn(Session* target, int send_return);

	//
	void CheckRecvReturn(Session* target, int recv_return);

	//
	bool CheckLibraryPacketCode(BYTE Code);

	int FindSession(__int64 session_ID);
	int* FindEmptySession();

	void ClearSendBuffer(Session* target);


	static unsigned int WINAPI AcceptThread(LPVOID this_ptr);
	static unsigned int WINAPI WorkerThread(LPVOID this_ptr);
	static unsigned int WINAPI MonitorThread(LPVOID this_ptr);
	static unsigned int WINAPI HeartbeatThread(LPVOID this_ptr);


	virtual bool OnConnectionRequest(const wchar_t* server_IP, unsigned short server_port) = 0;
	virtual void OnAccept(const wchar_t* server_IP, unsigned short server_port, __int64 session_ID) = 0;
	virtual void OnRelease(__int64 session_ID) = 0;

	virtual void OnMessage(__int64 session_ID, CPacket* send_packet) = 0;

	virtual void OnError(int errorcode, const wchar_t* error_log) = 0;

	int GetAcceptTPS();
	int GetRecvMessageTPS();
	int GetSendMessageTPS();



	unsigned int accept_TPS;
	unsigned int recv_message_TPS;
	unsigned int send_message_TPS;

	unsigned int accept_count;
	unsigned int recv_message_count;
	long send_message_count;


	unsigned int max_session;
	unsigned int session_num;
	unsigned int threads_num;
	__int64 unique_id = 1;


	unsigned int header_size;
	//일단 귀찮아서 하드코딩
	unsigned int packet_type = 1;


	unsigned long long timeout;
	unsigned long long unlogin_timeout;



	HANDLE handle_iocp;
	SOCKET listen_sock;
	HANDLE* threads;
	HANDLE accept_thread;
	HANDLE monitor_thread;
	HANDLE heartbeat_thread;
	Session* session_array;

	LFObjectFreeList<int> index_list;



};

