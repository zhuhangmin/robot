#pragma once

#define     GR_GAME_PULSE          	 (GAME_REQ_BASE_EX + 1020)      // 玩家客户端脉搏信号
#define		GR_GET_VERSION			 (GAME_REQ_BASE_EX + 1001)		// 获取版本信息


#define     GR_RESPONE_ENTER_GAME_OK			 (GAME_REQ_BASE_EX+10200)		//用户进入游戏成功，不在游戏中,带ENTER_GAME_INFO结构
#define     GR_RESPONE_ENTER_GAME_DXXW			 (GAME_REQ_BASE_EX+10210)		//用户进入游戏成功，断线续完，带GAME_TABLE_INFO结构
#define		GR_RESPONE_ENTER_GAME_IDLE			 (GAME_REQ_BASE_EX+10220)		//用户进入游戏成功，游戏中新进来的玩家，带GAME_TABLE_INFO结构)


typedef struct _tagGAME_PULSE {
    int nUserID;
    DWORD dwAveDelay;							// 1分钟内的平均延迟
    DWORD dwMaxDelay;							// 单次通讯最大延迟
    int nReserved[1];
}GAME_PULSE, *LPGAME_PULSE;

typedef struct _tagGET_ROOMUSERS {
    int nAgentGroupID;
    DWORD dwGetFlags;
    int nReserved[3];
    int nRoomCount;//该结构发送时变长
    int nRoomID[MAX_QUERY_ROOMS];
}GET_ROOMUSERS, *LPGET_ROOMUSERS;

typedef struct _tagITEM_COUNT {
    int nCount;
    int nStatTime;//2038秒
    int nSubCount;
    DWORD dwGetFlags;
    int nReserved[1];
}ITEM_COUNT, *LPITEM_COUNT;