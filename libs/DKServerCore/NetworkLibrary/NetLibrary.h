
#pragma once

#include <winsock2.h>
#include "LockFreeObjectFreeList.h"
#include "ContentsCPacket.h"
#include "LockFreeQueue.h"
#include "LockFreeQueueCas2.h"
#include "MyRingBuffer.h"
#define RECV 10
#define SEND 20

#define MAXBATCHSIZE 100
class NetLibrary
{
public:
	NetLibrary();

	virtual ~NetLibrary();

	struct PacketHeader
	{
		unsigned short length;
	};

#pragma pack(push,1)
	struct NetPacketHeader
	{
		BYTE Code;
		WORD Len;
		BYTE RandKey;
		BYTE CheckSum;
	};
#pragma pack(pop)

	struct MyOverlapped
	{
		WSAOVERLAPPED overlapped;
		int Type;
	};

	struct BufferCount
	{
		CPacket* buffers[MAXBATCHSIZE];
		long count;
	};

	struct Session
	{
		TLockFreeQueue<CPacket*> send_buffer;

		MyOverlapped send_overlapped;
		MyOverlapped recv_overlapped;

		MyRingBuffer recv_buffer;

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



	bool Start(const char* server_IP, unsigned int  server_port, unsigned int threads_count, unsigned int concurrent_threads, unsigned int nagle, unsigned int sessions, unsigned int header_size, unsigned char packetCode);
	bool Stop();
	void Disconnect(__int64 session_ID);
	Session* SessionAlloc(int* empty_index, unsigned long long client_sock);

	void SendCompletion(Session* target);
	void RecvCompletion(Session* target, DWORD cbTransferred);

	void SendPacket(__int64 session_ID, ContentsCPacket send_packet);
	void SendPost(Session* Target);

	void ReceiveFirst(Session* new_session);
	void RecvProc(Session* target);
	void Receive(Session* target);
	void AddHeader(CPacket* packet_buffer);
	void NetAddHeader(CPacket* packet_buffer);

	void Release(Session* target);


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
	virtual void OnMessage(__int64 session_ID, ContentsCPacket* send_packet) = 0;
	virtual void OnError(int errorcode, const wchar_t* error_log) = 0;
	virtual void OnInitializeTPS() = 0;

	int GetAcceptTPS();
	int GetRecvMessageTPS();
	int GetSendMessageTPS();



	DWORD accept_TPS;
	DWORD recv_message_TPS;
	DWORD send_message_TPS;

	DWORD accept_count;
	DWORD recv_message_count;
	long send_message_count;


	DWORD max_session;
	DWORD session_num;
	DWORD threads_num;
	__int64 unique_id = 1;


	DWORD header_size;
	//일단 귀찮아서 하드코딩
	DWORD packet_type = 1;


	unsigned long long timeout;
	unsigned long long unlogin_timeout;

	DWORD DisconnectCount;
	DWORD DCUnloginTimeout;
	DWORD DCLoginTimeout;
	DWORD DCSendBufferFull;
	DWORD DCPacketCodeError;
	DWORD DCDecodeError;
	DWORD DCSessionFull;
	DWORD DCImpossiblePacketLength;

	DWORD GetDisconnectCount();
	DWORD GetDCUnloginTimeout();
	DWORD GetDCLoginTimeout();
	DWORD GetDCSendBufferFull();
	DWORD GetDCPacketCodeError();
	DWORD GetDCDecodeError();
	DWORD GetDCSessionFull();
	DWORD GetDCImpossiblePacketLength();

	DWORD GetSessionNum();
	unsigned long long GetAcceptTotal();





	HANDLE handle_iocp;
	SOCKET listen_sock;
	HANDLE* threads;
	HANDLE accept_thread;
	HANDLE monitor_thread;
	HANDLE heartbeat_thread;
	Session* session_array;

	LFObjectFreeList<int> index_list;
	unsigned char packetCode_ = 0;
	unsigned long long acceptTotal_ = 0;

};

