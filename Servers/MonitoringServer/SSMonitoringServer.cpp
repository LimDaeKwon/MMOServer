#include "SSMonitoringServer.h"
#include "ContentsCPacket.h"
#include <process.h>


SSMonitoringServer::SSMonitoringServer() : userPool_(0), MessageDataFreeList(1000)
{
	
	MessageEvent = CreateEvent(NULL, FALSE, FALSE ,nullptr);

	if (MessageEvent == NULL)
	{
		DebugBreak();
	}

	LogicThreadHandle = (HANDLE)_beginthreadex(nullptr, 0, LogicThread, this, 0, nullptr);

	mysql_init(&conn);

	connection = mysql_real_connect(&conn, "127.0.0.1", "root", "dlaeornjs", "logdb", 3306, NULL, 0);


}

SSMonitoringServer::~SSMonitoringServer()
{

}

bool SSMonitoringServer::OnConnectionRequest(const wchar_t* server_IP, unsigned short server_port)
{
	return false;
}

void SSMonitoringServer::OnAccept(const wchar_t* server_IP, unsigned short server_port, __int64 sessionID)
{

	MessageData* msg_data = MessageDataFreeList.Alloc();
	msg_data->session_ID = sessionID;
	msg_data->contents_packet = nullptr;
	msg_data->type = ACCEPT;

	MessageQueue.Enqueue(msg_data);
	SetEvent(MessageEvent);

	//여기서 언로그인 세팅. 넉넉하게 10초로 짤라주자. 

}

void SSMonitoringServer::OnRelease(__int64 sessionID)
{
	MessageData* msg_data = MessageDataFreeList.Alloc();
	msg_data->session_ID = sessionID;
	msg_data->contents_packet = nullptr;
	msg_data->type = RELEASE;

	MessageQueue.Enqueue(msg_data);
	SetEvent(MessageEvent);

}

void SSMonitoringServer::OnMessage(__int64 sessionID, ContentsCPacket* contentsSendPacket)
{
	MessageData* msg_data = MessageDataFreeList.Alloc();
	msg_data->session_ID = sessionID;
	msg_data->contents_packet = contentsSendPacket;
	msg_data->type = PACKET;

	MessageQueue.Enqueue(msg_data);
	SetEvent(MessageEvent);

}

void SSMonitoringServer::OnError(int errorcode, const wchar_t* error_log)
{

}

void SSMonitoringServer::OnInitializeTPS()
{

	//이거.
	MessageData* msg_data = MessageDataFreeList.Alloc();
	msg_data->type = TPS;
	MessageQueue.Enqueue(msg_data);
	SetEvent(MessageEvent);


}


void SSMonitoringServer::NetPacketProc_ServerLogin(User* target, int serverNo)
{
	target->serverNo_ = serverNo;
}

void SSMonitoringServer::NetPacketProc_DataUpdate(User* target, BYTE dataType, int dataValue, int timeStamp)
{
	//데이터 찾아서 업데이트 응답은 주지 않는듯. 
	//갱신만 

	switch (target->serverNo_)
	{
		case 1:
		{
			if (!UpdateLoginServerMonitorData(dataType, dataValue, timeStamp))
			{
				// TODO: LoginServer invalid dataType log
			}
			break;
		}
		case 2:
		{
			if (!UpdateGameServerMonitorData(dataType, dataValue, timeStamp))
			{
				// TODO: GameServer invalid dataType log
			}
			break;
		}
		case 3:
		{
			if (!UpdateChatServerMonitorData(dataType, dataValue, timeStamp))
			{
				// TODO: ChatServer invalid dataType log
			}
			break;
		}
		case 4:
		{
			if (!UpdateSystemMonitorData(dataType, dataValue, timeStamp))
			{
				// TODO: SystemMonitor invalid dataType log
			}
			break;
		}
		default:
		{
			// TODO: invalid userNo log
			break;
		}
	}

}


void SSMonitoringServer::UpdateMonitorValue(MonitorValue& monitorValue, int dataValue, int timeStamp)
{
	monitorValue.value_ = dataValue;
	monitorValue.timestamp_ = timeStamp;

	if (monitorValue.sampleCount_ == 0)
	{
		monitorValue.sum_ = dataValue;
		monitorValue.min_ = dataValue;
		monitorValue.max_ = dataValue;
		monitorValue.sampleCount_ = 1;
		return;
	}

	monitorValue.sum_ += dataValue;

	if (dataValue < monitorValue.min_)
	{
		monitorValue.min_ = dataValue;
	}

	if (dataValue > monitorValue.max_)
	{
		monitorValue.max_ = dataValue;
	}

	++monitorValue.sampleCount_;
	

}

bool SSMonitoringServer::UpdateLoginServerMonitorData(BYTE dataType, int dataValue, int timeStamp)
{
	switch (dataType)
	{
		case dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN:
		{
			UpdateMonitorValue(loginServerData_.isRunning_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU:
		{
			UpdateMonitorValue(loginServerData_.cpuUsage_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM:
		{
			UpdateMonitorValue(loginServerData_.memoryMBytes_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_LOGIN_SESSION:
		{
			UpdateMonitorValue(loginServerData_.sessionCount_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS:
		{
			UpdateMonitorValue(loginServerData_.authTps_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL:
		{
			UpdateMonitorValue(loginServerData_.packetPoolUsage_, dataValue, timeStamp);
			return true;
		}
		default:
		{
			return false;
		}
	}
}

bool SSMonitoringServer::UpdateGameServerMonitorData(BYTE dataType, int dataValue, int timeStamp)
{
	switch (dataType)
	{
		case dfMONITOR_DATA_TYPE_GAME_SERVER_RUN:
		{
			UpdateMonitorValue(gameServerData_.isRunning_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_GAME_SERVER_CPU:
		{
			UpdateMonitorValue(gameServerData_.cpuUsage_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_GAME_SERVER_MEM:
		{
			UpdateMonitorValue(gameServerData_.memoryMBytes_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_GAME_SESSION:
		{
			UpdateMonitorValue(gameServerData_.sessionCount_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER:
		{
			UpdateMonitorValue(gameServerData_.authPlayerCount_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER:
		{
			UpdateMonitorValue(gameServerData_.gamePlayerCount_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS:
		{
			UpdateMonitorValue(gameServerData_.acceptTps_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS:
		{
			UpdateMonitorValue(gameServerData_.packetRecvTps_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS:
		{
			UpdateMonitorValue(gameServerData_.packetSendTps_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS:
		{
			UpdateMonitorValue(gameServerData_.dbWriteTps_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG:
		{
			UpdateMonitorValue(gameServerData_.dbWriteMessageQueueCount_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS:
		{
			UpdateMonitorValue(gameServerData_.authThreadFps_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS:
		{
			UpdateMonitorValue(gameServerData_.gameThreadFps_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_GAME_PACKET_POOL:
		{
			UpdateMonitorValue(gameServerData_.packetPoolUsage_, dataValue, timeStamp);
			return true;
		}
		default:
		{
			return false;
		}
	}
}

bool SSMonitoringServer::UpdateChatServerMonitorData(BYTE dataType, int dataValue, int timeStamp)
{
	switch (dataType)
	{
		case dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN:
		{
			UpdateMonitorValue(chatServerData_.isRunning_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU:
		{
			UpdateMonitorValue(chatServerData_.cpuUsage_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM:
		{
			UpdateMonitorValue(chatServerData_.memoryMBytes_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_CHAT_SESSION:
		{
			UpdateMonitorValue(chatServerData_.sessionCount_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_CHAT_PLAYER:
		{
			UpdateMonitorValue(chatServerData_.playerCount_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS:
		{
			UpdateMonitorValue(chatServerData_.updateTps_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL:
		{
			UpdateMonitorValue(chatServerData_.packetPoolUsage_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL:
		{
			UpdateMonitorValue(chatServerData_.updateMessagePoolUsage_, dataValue, timeStamp);
			return true;
		}
		default:
		{
			return false;
		}
	}
}

bool SSMonitoringServer::UpdateSystemMonitorData(BYTE dataType, int dataValue, int timeStamp)
{
	switch (dataType)
	{
		case dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL:
		{
			UpdateMonitorValue(systemData_.totalCpuUsage_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY:
		{
			UpdateMonitorValue(systemData_.nonPagedMemoryMBytes_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV:
		{
			UpdateMonitorValue(systemData_.networkRecvKBytes_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND:
		{
			UpdateMonitorValue(systemData_.networkSendKBytes_, dataValue, timeStamp);
			return true;
		}
		case dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY:
		{
			UpdateMonitorValue(systemData_.availableMemoryMBytes_, dataValue, timeStamp);
			return true;
		}
		default:
		{
			return false;
		}
	}
}

void SSMonitoringServer::InitLoginServerMonitorData()
{
	loginServerData_.authTps_.value_ = 0;
	loginServerData_.cpuUsage_.value_ = 0;
	loginServerData_.isRunning_.value_ = 0;
	loginServerData_.memoryMBytes_.value_ = 0;
	loginServerData_.packetPoolUsage_.value_ = 0;
	loginServerData_.sessionCount_.value_ = 0;
}

void SSMonitoringServer::InitGameServerMonitorData()
{
	gameServerData_.acceptTps_.value_ = 0;
	gameServerData_.authPlayerCount_.value_ = 0;
	gameServerData_.authThreadFps_.value_ = 0;
	gameServerData_.cpuUsage_.value_ = 0;
	gameServerData_.dbWriteMessageQueueCount_.value_ = 0;;
	gameServerData_.dbWriteTps_.value_ = 0;
	gameServerData_.gamePlayerCount_.value_ = 0;
	gameServerData_.gameThreadFps_.value_ = 0;
	gameServerData_.isRunning_.value_ = 0;
	gameServerData_.memoryMBytes_.value_ = 0;
	gameServerData_.packetPoolUsage_.value_ = 0;
	gameServerData_.packetRecvTps_.value_ = 0;
	gameServerData_.packetSendTps_.value_ = 0;
	gameServerData_.sessionCount_.value_ = 0;
	
}

void SSMonitoringServer::InitChatServerMonitorData()
{
	chatServerData_.cpuUsage_.value_ = 0;
	chatServerData_.isRunning_.value_ = 0;
	chatServerData_.memoryMBytes_.value_ = 0;
	chatServerData_.packetPoolUsage_.value_ = 0;
	chatServerData_.playerCount_.value_ = 0;
	chatServerData_.sessionCount_.value_ = 0;
	chatServerData_.updateMessagePoolUsage_.value_ = 0;
	chatServerData_.updateTps_.value_ = 0;
}

void SSMonitoringServer::InitSystemMonitorData()
{
	systemData_.availableMemoryMBytes_.value_ = 0;
	systemData_.networkRecvKBytes_.value_ = 0;
	systemData_.networkSendKBytes_.value_ = 0;
	systemData_.nonPagedMemoryMBytes_.value_ = 0;
	systemData_.totalCpuUsage_.value_ = 0;
}

void SSMonitoringServer::SendMonitorData(User* target, BYTE serverNo, BYTE dataType, const MonitorValue& monitorValue)
{

	ContentsCPacket packet = ContentsCPacket::MakeContentsPacket();
	packet << static_cast<WORD>(en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE);
	packet << serverNo;
	packet << dataType;
	packet << monitorValue.value_;
	packet << monitorValue.timestamp_;

	SendPacket(target->sessionID_, packet);

}


unsigned int WINAPI SSMonitoringServer::LogicThread(LPVOID thisPtr)
{
	SSMonitoringServer* server = static_cast<SSMonitoringServer*>(thisPtr);

	while (true)
	{
		WaitForSingleObject(server->MessageEvent, INFINITE);

		while (true)
		{
			MessageData* msg = nullptr;
			if (!server->MessageQueue.Dequeue(&msg))
			{
				break;
			}

			switch (msg->type)
			{
				case SSMonitoringServer::ACCEPT:
				{
					server->AcceptProc(msg);
					break;
				}
				case SSMonitoringServer::PACKET:
				{
					server->MessageProc(msg);
					break;
				}
				case SSMonitoringServer::RELEASE:
				{
					server->ReleaseProc(msg);
					break;
				}
				case SSMonitoringServer::TPS:
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

void SSMonitoringServer::AcceptProc(MessageData* msg_data)
{
	User* newUser = userPool_.Alloc();
	newUser->serverNo_ = 0;//아직
	newUser->sessionID_ = msg_data->session_ID;//아직

	userMap_.insert(std::unordered_map<__int64, User*>::value_type(newUser->sessionID_, newUser));
}

void SSMonitoringServer::MessageProc(MessageData* msgData)
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
		case en_PACKET_SS_MONITOR_LOGIN:
		{

			int	serverNo;
			if (contentsPacket.GetDataSize() != 4)
			{
				InterlockedIncrement(&DCWrongPacket);
				Disconnect(targetUser->sessionID_);
				break;
			}

			contentsPacket >> serverNo;

			NetPacketProc_ServerLogin(targetUser, serverNo);

			break;
		}
		case en_PACKET_SS_MONITOR_DATA_UPDATE:
		{

			BYTE dataType;
			int	dataValue;
			int	timeStamp;

			if (contentsPacket.GetDataSize() != 9)
			{
				InterlockedIncrement(&DCWrongPacket);
				Disconnect(targetUser->sessionID_);
				break;
			}

			contentsPacket >> dataType;
			contentsPacket >> dataValue;
			contentsPacket >> timeStamp;

			NetPacketProc_DataUpdate(targetUser, dataType, dataValue, timeStamp);

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

void SSMonitoringServer::ReleaseProc(MessageData* msg_data)
{

	User* target;
	std::unordered_map<__int64, User*>::iterator userIterator = userMap_.find(msg_data->session_ID);
	if (userIterator == userMap_.end())
	{
		//왜 없니
		//__debugbreak();
	}
	//해당 서버의 모니터링 데이터 초기화.
	target = userIterator->second;

	switch (target->serverNo_)
	{
	case dfMONITOR_SERVER_TYPE_SYSTEM:
	{
		InitSystemMonitorData();
		break;
	}
	case dfMONITOR_SERVER_TYPE_LOGIN:
	{
		InitLoginServerMonitorData();
		break;
	}
	case dfMONITOR_SERVER_TYPE_GAME:
	{
		InitGameServerMonitorData();
		break;
	}
	case dfMONITOR_SERVER_TYPE_CHAT:
	{
		InitChatServerMonitorData();
		break;
	}

	default:
	{
		// TODO: invalid userNo log
		break;
	}
	}


	userMap_.erase(target->sessionID_);
	userPool_.Free(target);

}

void SSMonitoringServer::TPSProc(MessageData* msg_data)
{
	
	if (++storeCount_ == 300)
	{
		FlushAllMonitorLogData();
		storeCount_ = 0;

	}


	//시스템 업데이트 

	int timeStamp = static_cast<int>(time(nullptr));

	systemData_.availableMemoryMBytes_.value_ = static_cast<int>(systemMonitoring_.GetServerAvailableMBytes());
	systemData_.availableMemoryMBytes_.timestamp_ = timeStamp;

	systemData_.nonPagedMemoryMBytes_.value_ = static_cast<int>(systemMonitoring_.GetServerNonPagedMBytes());
	systemData_.nonPagedMemoryMBytes_.timestamp_ = timeStamp;

	//모니터링 데이터들 형변환은 알아서. 
	//네트워크는 일단 마지막으로 냅두기. 

	systemData_.networkRecvKBytes_.value_ = static_cast<int>(systemMonitoring_.GetServerNetRecvKBytes());
	systemData_.networkRecvKBytes_.timestamp_ = timeStamp;
	systemData_.networkSendKBytes_.value_ = static_cast<int>(systemMonitoring_.GetServerNetSendKBytes());
	systemData_.networkSendKBytes_.timestamp_ = timeStamp;

	systemData_.totalCpuUsage_.value_ = static_cast<int>(systemMonitoring_.ProcessorTotal());
	systemData_.totalCpuUsage_.timestamp_ = timeStamp;


}

bool SSMonitoringServer::InsertMonitorValueLog(int logTime, int serverNo, int type, const MonitorValue& monitorValue)
{
	int avgValue = 0;
	if (monitorValue.sampleCount_ != 0)
	{
		avgValue = static_cast<int>(monitorValue.sum_ / monitorValue.sampleCount_);
	}
	

	char query[1024];

	sprintf_s(query, sizeof(query), "INSERT INTO monitorlog " "(logtime, serverno, type, avg_value, min_value, max_value) " "VALUES " "(FROM_UNIXTIME(%d), %d, %d, %d, %d, %d)", logTime, serverNo, type, avgValue, monitorValue.min_, monitorValue.max_);

	if (mysql_query(connection, query) != 0)
	{
		//DebugBreak();
		return false;
	}

	return true;

}

void SSMonitoringServer::ResetMonitorValueLog(MonitorValue& monitorValue)
{
	monitorValue.sum_ = 0;
	monitorValue.min_ = 0;
	monitorValue.max_ = 0;
	monitorValue.sampleCount_ = 0;
}

void SSMonitoringServer::FlushLoginServerMonitorLog(int logTime)
{
	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_LOGIN,
		dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN,
		loginServerData_.isRunning_))
	{
		ResetMonitorValueLog(loginServerData_.isRunning_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_LOGIN,
		dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU,
		loginServerData_.cpuUsage_))
	{
		ResetMonitorValueLog(loginServerData_.cpuUsage_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_LOGIN,
		dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM,
		loginServerData_.memoryMBytes_))
	{
		ResetMonitorValueLog(loginServerData_.memoryMBytes_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_LOGIN,
		dfMONITOR_DATA_TYPE_LOGIN_SESSION,
		loginServerData_.sessionCount_))
	{
		ResetMonitorValueLog(loginServerData_.sessionCount_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_LOGIN,
		dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS,
		loginServerData_.authTps_))
	{
		ResetMonitorValueLog(loginServerData_.authTps_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_LOGIN,
		dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL,
		loginServerData_.packetPoolUsage_))
	{
		ResetMonitorValueLog(loginServerData_.packetPoolUsage_);
	}
}

void SSMonitoringServer::FlushGameServerMonitorLog(int logTime)
{
	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_GAME,
		dfMONITOR_DATA_TYPE_GAME_SERVER_RUN,
		gameServerData_.isRunning_))
	{
		ResetMonitorValueLog(gameServerData_.isRunning_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_GAME,
		dfMONITOR_DATA_TYPE_GAME_SERVER_CPU,
		gameServerData_.cpuUsage_))
	{
		ResetMonitorValueLog(gameServerData_.cpuUsage_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_GAME,
		dfMONITOR_DATA_TYPE_GAME_SERVER_MEM,
		gameServerData_.memoryMBytes_))
	{
		ResetMonitorValueLog(gameServerData_.memoryMBytes_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_GAME,
		dfMONITOR_DATA_TYPE_GAME_SESSION,
		gameServerData_.sessionCount_))
	{
		ResetMonitorValueLog(gameServerData_.sessionCount_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_GAME,
		dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER,
		gameServerData_.authPlayerCount_))
	{
		ResetMonitorValueLog(gameServerData_.authPlayerCount_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_GAME,
		dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER,
		gameServerData_.gamePlayerCount_))
	{
		ResetMonitorValueLog(gameServerData_.gamePlayerCount_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_GAME,
		dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS,
		gameServerData_.acceptTps_))
	{
		ResetMonitorValueLog(gameServerData_.acceptTps_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_GAME,
		dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS,
		gameServerData_.packetRecvTps_))
	{
		ResetMonitorValueLog(gameServerData_.packetRecvTps_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_GAME,
		dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS,
		gameServerData_.packetSendTps_))
	{
		ResetMonitorValueLog(gameServerData_.packetSendTps_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_GAME,
		dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS,
		gameServerData_.dbWriteTps_))
	{
		ResetMonitorValueLog(gameServerData_.dbWriteTps_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_GAME,
		dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG,
		gameServerData_.dbWriteMessageQueueCount_))
	{
		ResetMonitorValueLog(gameServerData_.dbWriteMessageQueueCount_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_GAME,
		dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS,
		gameServerData_.authThreadFps_))
	{
		ResetMonitorValueLog(gameServerData_.authThreadFps_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_GAME,
		dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS,
		gameServerData_.gameThreadFps_))
	{
		ResetMonitorValueLog(gameServerData_.gameThreadFps_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_GAME,
		dfMONITOR_DATA_TYPE_GAME_PACKET_POOL,
		gameServerData_.packetPoolUsage_))
	{
		ResetMonitorValueLog(gameServerData_.packetPoolUsage_);
	}

}

void SSMonitoringServer::FlushChatServerMonitorLog(int logTime)
{
	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_CHAT,
		dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN,
		chatServerData_.isRunning_))
	{
		ResetMonitorValueLog(chatServerData_.isRunning_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_CHAT,
		dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU,
		chatServerData_.cpuUsage_))
	{
		ResetMonitorValueLog(chatServerData_.cpuUsage_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_CHAT,
		dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM,
		chatServerData_.memoryMBytes_))
	{
		ResetMonitorValueLog(chatServerData_.memoryMBytes_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_CHAT,
		dfMONITOR_DATA_TYPE_CHAT_SESSION,
		chatServerData_.sessionCount_))
	{
		ResetMonitorValueLog(chatServerData_.sessionCount_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_CHAT,
		dfMONITOR_DATA_TYPE_CHAT_PLAYER,
		chatServerData_.playerCount_))
	{
		ResetMonitorValueLog(chatServerData_.playerCount_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_CHAT,
		dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS,
		chatServerData_.updateTps_))
	{
		ResetMonitorValueLog(chatServerData_.updateTps_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_CHAT,
		dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL,
		chatServerData_.packetPoolUsage_))
	{
		ResetMonitorValueLog(chatServerData_.packetPoolUsage_);
	}

	if (InsertMonitorValueLog(
		logTime,
		dfMONITOR_SERVER_TYPE_CHAT,
		dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL,
		chatServerData_.updateMessagePoolUsage_))
	{
		ResetMonitorValueLog(chatServerData_.updateMessagePoolUsage_);
	}
}

void SSMonitoringServer::FlushSystemMonitorLog(int logTime)
{
	if (InsertMonitorValueLog(
		logTime,
		4,
		dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL,
		systemData_.totalCpuUsage_))
	{
		ResetMonitorValueLog(systemData_.totalCpuUsage_);
	}

	if (InsertMonitorValueLog(
		logTime,
		4,
		dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY,
		systemData_.nonPagedMemoryMBytes_))
	{
		ResetMonitorValueLog(systemData_.nonPagedMemoryMBytes_);
	}

	if (InsertMonitorValueLog(
		logTime,
		4,
		dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV,
		systemData_.networkRecvKBytes_))
	{
		ResetMonitorValueLog(systemData_.networkRecvKBytes_);
	}

	if (InsertMonitorValueLog(
		logTime,
		4,
		dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND,
		systemData_.networkSendKBytes_))
	{
		ResetMonitorValueLog(systemData_.networkSendKBytes_);
	}

	if (InsertMonitorValueLog(
		logTime,
		4,
		dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY,
		systemData_.availableMemoryMBytes_))
	{
		ResetMonitorValueLog(systemData_.availableMemoryMBytes_);
	}
}

void SSMonitoringServer::FlushAllMonitorLogData()
{
	const int logTime = static_cast<int>(time(nullptr));

	if (mysql_query(connection, "START TRANSACTION") != 0)
	{
		DebugBreak();
	}

	FlushLoginServerMonitorLog(logTime);
	FlushGameServerMonitorLog(logTime);
	FlushChatServerMonitorLog(logTime);
	FlushSystemMonitorLog(logTime);

	if (mysql_query(connection, "COMMIT") != 0)
	{
		DebugBreak();
	}

}



DWORD SSMonitoringServer::GetDCWrongPacket()
{
	return DCWrongPacket;
}

DWORD SSMonitoringServer::GetDCAuthFailed()
{
	return DCAuthFailed;
}

DWORD SSMonitoringServer::GetDCDuplicateLogin()
{
	return DCDuplicateLogin;
}

DWORD SSMonitoringServer::GetLoginPlayer()
{
	return LoginPlayer;
}

DWORD SSMonitoringServer::GetUnloginPlayer()
{
	return UnloginPlayer;
}

