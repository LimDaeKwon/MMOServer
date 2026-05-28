#include "CSMonitoringServer.h"
#include "ContentsCPacket.h"
#include <process.h>


CSMonitoringServer::CSMonitoringServer(LoginServerMonitorData& loginServerData, GameServerMonitorData& gameServerData, ChatServerMonitorData& chatServerData, SystemMonitorData& systemData) 
	: userPool_(0), MessageDataFreeList(1000), loginServerData_(loginServerData),gameServerData_(gameServerData), chatServerData_(chatServerData), systemData_(systemData)
{
	LogicThreadHandle = (HANDLE)_beginthreadex(nullptr, 0, LogicThread, this, 0, nullptr);

	MessageEvent = CreateEvent(NULL, FALSE, FALSE,nullptr);

	if (MessageEvent == NULL)
	{
		printf("CreateEvent failed (%d)\n", GetLastError());
		DebugBreak();
	}

}

CSMonitoringServer::~CSMonitoringServer()
{

}

bool CSMonitoringServer::OnConnectionRequest(const wchar_t* server_IP, unsigned short server_port)
{
	return false;
}

void CSMonitoringServer::OnAccept(const wchar_t* server_IP, unsigned short server_port, __int64 sessionID)
{

	MessageData* msg_data = MessageDataFreeList.Alloc();
	msg_data->session_ID = sessionID;
	msg_data->contents_packet = nullptr;
	msg_data->type = ACCEPT;

	MessageQueue.Enqueue(msg_data);
	SetEvent(MessageEvent);

}

void CSMonitoringServer::OnRelease(__int64 sessionID)
{
	MessageData* msg_data = MessageDataFreeList.Alloc();
	msg_data->session_ID = sessionID;
	msg_data->contents_packet = nullptr;
	msg_data->type = RELEASE;

	MessageQueue.Enqueue(msg_data);
	SetEvent(MessageEvent);

}

void CSMonitoringServer::OnMessage(__int64 sessionID, ContentsCPacket* contentsSendPacket)
{
	MessageData* msg_data = MessageDataFreeList.Alloc();
	msg_data->session_ID = sessionID;
	msg_data->contents_packet = contentsSendPacket;
	msg_data->type = PACKET;

	MessageQueue.Enqueue(msg_data);
	SetEvent(MessageEvent);

}

void CSMonitoringServer::OnError(int errorcode, const wchar_t* error_log)
{

}

void CSMonitoringServer::OnInitializeTPS()
{

	MessageData* msg_data = MessageDataFreeList.Alloc();
	msg_data->type = TPS;
	MessageQueue.Enqueue(msg_data);
	SetEvent(MessageEvent);


}


void CSMonitoringServer::NetPacketProc_ClientLogin(User* target, char* loginSessionKey)
{

	BYTE status;

	if (memcmp(loginSessionKey, LoginSessionKey_, 32) != 0)
	{
		//키 실패

		//enum en_PACKET_CS_MONITOR_TOOL_RES_LOGIN
		//{
		//	dfMONITOR_TOOL_LOGIN_OK = 1,		// 로그인 성공
		//	dfMONITOR_TOOL_LOGIN_ERR_NOSERVER = 2,		// 서버이름 오류 (매칭미스)
		//	dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY = 3,		// 로그인 세션키 오류
		//};
		status = dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY;

		ContentsCPacket loginPakcet = ContentsCPacket::MakeContentsPacket();

		loginPakcet << (WORD)en_PACKET_CS_MONITOR_TOOL_RES_LOGIN << status;

		SendPacket(target->sessionID_, loginPakcet);

		Disconnect(target->sessionID_);//음.. 

		return;
	}
	//성공


	status = dfMONITOR_TOOL_LOGIN_OK;
	ContentsCPacket loginPakcet = ContentsCPacket::MakeContentsPacket();
	loginPakcet << (WORD)en_PACKET_CS_MONITOR_TOOL_RES_LOGIN << status;
	SendPacket(target->sessionID_, loginPakcet);
	target->loginFlag_ = 1;
	UnloginPlayer--;
	LoginPlayer++;
	

}

void CSMonitoringServer::UpdateMonitorValue(MonitorValue& monitorValue, int dataValue, int timeStamp)
{
	monitorValue.value_ = dataValue;
	monitorValue.timestamp_ = timeStamp;
}

void CSMonitoringServer::SendMonitorData(User* target, BYTE serverNo, BYTE dataType, const MonitorValue& monitorValue)
{

	ContentsCPacket packet = ContentsCPacket::MakeContentsPacket();
	packet << static_cast<WORD>(en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE);
	packet << serverNo;
	packet << dataType;
	packet << monitorValue.value_;
	packet << monitorValue.timestamp_;

	SendPacket(target->sessionID_, packet);

}


void CSMonitoringServer::SendLoginServerMonitorData(User* target)
{
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_LOGIN, dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN, loginServerData_.isRunning_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_LOGIN, dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU, loginServerData_.cpuUsage_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_LOGIN, dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM, loginServerData_.memoryMBytes_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_LOGIN, dfMONITOR_DATA_TYPE_LOGIN_SESSION, loginServerData_.sessionCount_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_LOGIN, dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS, loginServerData_.authTps_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_LOGIN, dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL, loginServerData_.packetPoolUsage_);
}

void CSMonitoringServer::SendGameServerMonitorData(User* target)
{
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_GAME, dfMONITOR_DATA_TYPE_GAME_SERVER_RUN, gameServerData_.isRunning_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_GAME, dfMONITOR_DATA_TYPE_GAME_SERVER_CPU, gameServerData_.cpuUsage_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_GAME, dfMONITOR_DATA_TYPE_GAME_SERVER_MEM, gameServerData_.memoryMBytes_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_GAME, dfMONITOR_DATA_TYPE_GAME_SESSION, gameServerData_.sessionCount_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_GAME, dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER, gameServerData_.authPlayerCount_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_GAME, dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER, gameServerData_.gamePlayerCount_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_GAME, dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS, gameServerData_.acceptTps_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_GAME, dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS, gameServerData_.packetRecvTps_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_GAME, dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS, gameServerData_.packetSendTps_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_GAME, dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS, gameServerData_.dbWriteTps_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_GAME, dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG, gameServerData_.dbWriteMessageQueueCount_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_GAME, dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS, gameServerData_.authThreadFps_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_GAME, dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS, gameServerData_.gameThreadFps_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_GAME, dfMONITOR_DATA_TYPE_GAME_PACKET_POOL, gameServerData_.packetPoolUsage_);
}
void CSMonitoringServer::SendChatServerMonitorData(User* target)
{
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_CHAT, dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN, chatServerData_.isRunning_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_CHAT, dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU, chatServerData_.cpuUsage_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_CHAT, dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM, chatServerData_.memoryMBytes_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_CHAT, dfMONITOR_DATA_TYPE_CHAT_SESSION, chatServerData_.sessionCount_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_CHAT, dfMONITOR_DATA_TYPE_CHAT_PLAYER, chatServerData_.playerCount_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_CHAT, dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS, chatServerData_.updateTps_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_CHAT, dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, chatServerData_.packetPoolUsage_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_CHAT, dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, chatServerData_.updateMessagePoolUsage_);
}

void CSMonitoringServer::SendSystemMonitorData(User* target)
{
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_SYSTEM, dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL, systemData_.totalCpuUsage_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_SYSTEM, dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY, systemData_.nonPagedMemoryMBytes_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_SYSTEM, dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV, systemData_.networkRecvKBytes_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_SYSTEM, dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND, systemData_.networkSendKBytes_);
	SendMonitorData(target, dfMONITOR_SERVER_TYPE_SYSTEM, dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY, systemData_.availableMemoryMBytes_);
}

unsigned int WINAPI CSMonitoringServer::LogicThread(LPVOID thisPtr)
{
	CSMonitoringServer* server = static_cast<CSMonitoringServer*>(thisPtr);

	while (true)
	{
		WaitForSingleObject(server->MessageEvent, INFINITE);

		while (true)
		{
			MessageData* msg = nullptr;
			if (!server->MessageQueue.Dequeue(&msg))
			{
				continue;
			}

			switch (msg->type)
			{
				case CSMonitoringServer::ACCEPT:
				{
					server->AcceptProc(msg);
					break;
				}
				case CSMonitoringServer::PACKET:
				{
					server->MessageProc(msg);
					break;
				}
				case CSMonitoringServer::RELEASE:
				{
					server->ReleaseProc(msg);
					break;
				}
				case CSMonitoringServer::TPS:
				{
					server->TPSProc(msg);
					break;
				}
				default:
				{
					break;
				}
			}

			server->MessageDataFreeList.Free(msg);
		}
	}
	return 0;
}

void CSMonitoringServer::AcceptProc(MessageData* msg_data)
{
	User* newUser = userPool_.Alloc();
	newUser->sessionID_ = msg_data->session_ID;//아직
	newUser->loginFlag_ = 0;
	newUser->timeOut_ = GetTickCount64();

	
	userMap_.insert(std::unordered_map<__int64, User*>::value_type(newUser->sessionID_, newUser));
	UnloginPlayer++;
}

void CSMonitoringServer::MessageProc(MessageData* msgData)
{

	WORD MessageType;

	ContentsCPacket contentsPacket = msgData->contents_packet;

	contentsPacket >> MessageType;

	User* targetUser;

	std::unordered_map<__int64, User*>::iterator userIterator = userMap_.find(msgData->session_ID);
	if (userIterator == userMap_.end())
	{
		//왜 없니
		//__debugbreak();
	}
	targetUser = userIterator->second;

	switch (MessageType)
	{
		case en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN:
		{
			char loginSessionKey[32];

			if (contentsPacket.GetDataSize() != 32)
			{
				InterlockedIncrement(&DCWrongPacket);
				Disconnect(targetUser->sessionID_);
				break;
			}

			contentsPacket.GetData((char*)loginSessionKey, 32);

			NetPacketProc_ClientLogin(targetUser, loginSessionKey);

			break;
		}
		default:
		{
			//잘못된 데이터
			InterlockedIncrement(&DCWrongPacket);
			Disconnect(targetUser->sessionID_);
			break;
		}

	}
}

void CSMonitoringServer::ReleaseProc(MessageData* msg_data)
{

	User* targetUser;
	std::unordered_map<__int64, User*>::iterator userIterator = userMap_.find(msg_data->session_ID);
	if (userIterator == userMap_.end())
	{
		//왜 없니
		//__debugbreak();
	}

	targetUser = userIterator->second;

	if (targetUser->loginFlag_)
	{
		LoginPlayer--;
	}
	else
	{
		UnloginPlayer--;
	}

	userMap_.erase(targetUser->sessionID_);
	userPool_.Free(targetUser);

}

void CSMonitoringServer::TPSProc(MessageData* msg_data)
{

	
	//전체를 순회하긴 하니까. 여기서 타임도 체크하자. 
	std::unordered_map<__int64, User*>::iterator userIter;
	for (userIter = userMap_.begin(); userIter != userMap_.end(); ++userIter)
	{

		User* target = userIter->second;
		if (!target->loginFlag_)
		{
			if (GetTickCount64() - target->timeOut_ > 10000)
			{
				Disconnect(target->sessionID_);
				DCUnloginTimeout++;
			}
			//1초단위의 타임아웃처리. 
			continue;
		}

		SendLoginServerMonitorData(target);
		SendGameServerMonitorData(target);
		SendChatServerMonitorData(target);
		SendSystemMonitorData(target);
	}

}



DWORD CSMonitoringServer::GetDCWrongPacket()
{
	return DCWrongPacket;
}

DWORD CSMonitoringServer::GetDCAuthFailed()
{
	return DCAuthFailed;
}

DWORD CSMonitoringServer::GetDCDuplicateLogin()
{
	return DCDuplicateLogin;
}

DWORD CSMonitoringServer::GetLoginPlayer()
{
	return LoginPlayer;
}

DWORD CSMonitoringServer::GetUnloginPlayer()
{
	return UnloginPlayer;
}

