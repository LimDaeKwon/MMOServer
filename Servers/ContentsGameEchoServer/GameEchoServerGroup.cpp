#include "ContentsNetLibrary.h"
#include "GameEchoServerGroup.h"
#include "CommonProtocol.h"
#include "ContentsCPacket.h"

AuthGroup::AuthGroup(GroupId groupId, DWORD FrameMS)
{
	frameMS_ = FrameMS;
	groupId_ = groupId;
}

AuthGroup::~AuthGroup()
{
}

void AuthGroup::OnEnter(SessionId sessionId)
{
	Player* newPlayer = static_cast<Player*>(contentsServer_->GetPlayerPointer(sessionId));

	if (newPlayer == nullptr)
	{
		//엔터가 수행되기 전 끊긴거. 그냥 리턴시키기.
		return;
	}
	authPlayerMap_.insert(std::unordered_map<__int64, Player*>::value_type(newPlayer->sessionId_, newPlayer));
}

void AuthGroup::OnLeave(SessionId sessionId)
{
	std::unordered_map<__int64, Player*>::iterator AI = authPlayerMap_.find(sessionId);
	if (AI == authPlayerMap_.end())
	{
		//끊긴겨서 인큐조차 안 된거. 그냥 보내주기.
		return;
	}
	Player* leavePlayer = AI->second;
	authPlayerMap_.erase(sessionId);

}

void AuthGroup::OnMessage(SessionId sessionId, ContentsCPacket* packet)
{
	std::unordered_map<__int64, Player*>::iterator AI = authPlayerMap_.find(sessionId);
	if (AI == authPlayerMap_.end())
	{
		// 특이사항. 
		//contentsServer_->Disconnect(sessionId);
		//_debugbreak();
	}
	Player* target = AI->second;
	WORD messageType;

	ContentsCPacket contentsPacket = packet;

	while (contentsPacket.GetDataSize())
	{
		int len = 0;
		contentsPacket >> len;
		contentsPacket >> messageType;

		if (en_PACKET_CS_GAME_REQ_LOGIN)
		{
			//그룹 옮기기..
			//별 다른 확인 없이 넘겨주기. 
			//토큰조회는 하지 않음. 
			//우리 그룹에서 삭제.후 다른 그룹으로 인큐메시지. 
			// 
			//		WORD	Type
			//
			//		INT64	AccountNo
			//		char	SessionKey[64]
			//
			//		int		Version			// 1 
			if (len != 78)
			{
				//사유
				contentsServer_->Disconnect(sessionId);
				return;
			}

			INT64 accountNo;
			char sessionKey[64];		// 인증토큰
			int version; // 버전. 
			contentsPacket >> accountNo;
			contentsPacket.GetData((char*)sessionKey, 64);
			contentsPacket >> version;

			target->accountNo_ = accountNo;

			//contentsServer_->loginlogintpsLoginTPS

			contentsServer_->MoveGroup(sessionId, 1);
		}
		else
		{
			//공격
			contentsServer_->Disconnect(sessionId);
			//Disconnect(msg_data->session_ID);
		}





	}



	//인증메시지일거야.

	//en_PACKET_CS_GAME_REQ_LOGIN

	//엔터가 왔었는지 확인해야 함..  



	


}

void AuthGroup::OnUpdate()
{
	//흠.
	InterlockedIncrement(&UpdateCount);
}

int AuthGroup::GetPlayerNum()
{
	return static_cast<int>(authPlayerMap_.size());
}

int AuthGroup::GetFPS()
{
	
	return UpdateTPS;
}

void AuthGroup::OnInitializeTPS()
{
	
	UpdateTPS = UpdateCount;
	InterlockedExchange(&UpdateCount, 0);
}



EchoGroup::EchoGroup(GroupId groupId, DWORD FrameMS)
{
	frameMS_ = FrameMS;
	groupId_ = groupId;
}

EchoGroup::~EchoGroup()
{
}

void EchoGroup::OnEnter(SessionId sessionId)
{
	Player* target = static_cast<Player*>(contentsServer_->GetPlayerPointer(sessionId));

	if (target == nullptr)
	{
		//엔터가 수행되기 전 끊긴거. 그냥 리턴시키기.
		return;
	}

	echoPlayerMap_.insert(std::unordered_map<__int64, Player*>::value_type(target->sessionId_, target));;


	//로그인 응답 메시지 쏴주기. 
	ContentsCPacket login_packet = ContentsCPacket::MakeContentsPacket();
	login_packet << (WORD)en_PACKET_CS_GAME_RES_LOGIN << BYTE(TRUE) << target->accountNo_;

	contentsServer_->SendPacket(target->sessionId_, login_packet);

}

void EchoGroup::OnLeave(SessionId sessionId)
{
	std::unordered_map<__int64, Player*>::iterator echoIter = echoPlayerMap_.find(sessionId);
	if (echoIter == echoPlayerMap_.end())
	{
		//끊긴겨서 인큐조차 안 된거. 그냥 보내주기.
		return;
	}
	Player* target = echoIter->second;
	echoPlayerMap_.erase(sessionId);

}



void EchoGroup::OnMessage(SessionId sessionId, ContentsCPacket* packet)
{

	std::unordered_map<__int64, Player*>::iterator echoIter = echoPlayerMap_.find(sessionId);
	if (echoIter == echoPlayerMap_.end())
	{
		// 특이사항. 
		//contentsServer_->Disconnect(sessionId);
		//DebugBreak();
	}
	Player* target = echoIter->second;
	WORD messageType;
	

	ContentsCPacket contentsPacket = packet;
	while (contentsPacket.GetDataSize())
	{
		int len = 0;
		contentsPacket >> len;
		contentsPacket >> messageType;


		if (en_PACKET_CS_GAME_REQ_ECHO)
		{

			//에코. 그대로 돌려줌. 
					//		WORD		Type
			//
			if (len  != 18)
			{
				//사유
				contentsServer_->Disconnect(sessionId);
				return;
			}

			INT64 accountoNo;
			LONGLONG sendTick;

			contentsPacket >> accountoNo;
			contentsPacket >> sendTick;

			if (target->accountNo_ != accountoNo)
			{
				//음?
				contentsServer_->Disconnect(sessionId);
			}


			ContentsCPacket echoPacket = ContentsCPacket::MakeContentsPacket();
			echoPacket << (WORD)en_PACKET_CS_GAME_RES_ECHO << target->accountNo_ << sendTick;

			contentsServer_->SendPacket(target->sessionId_, echoPacket);

		}

		else if (en_PACKET_CS_GAME_REQ_HEARTBEAT)
		{
			if (len != 2)
			{
				contentsServer_->Disconnect(sessionId);
				return;
			}
		}
		else
		{
			contentsServer_->Disconnect(sessionId);
		}
	}
	

}

void EchoGroup::OnUpdate()
{
	InterlockedIncrement(&UpdateCount);

}


int EchoGroup::GetPlayerNum()
{
	return static_cast<int>(echoPlayerMap_.size());
}

int EchoGroup::GetFPS()
{

	return UpdateTPS;
}

void EchoGroup::OnInitializeTPS()
{
	UpdateTPS = UpdateCount;
	InterlockedExchange(&UpdateCount, 0);
}
