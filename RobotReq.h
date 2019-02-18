#pragma once

#define     GR_GAME_PULSE          	 (GAME_REQ_BASE_EX + 1020)      // ��ҿͻ��������ź�
#define		GR_GET_VERSION			 (GAME_REQ_BASE_EX + 1001)		// ��ȡ�汾��Ϣ


#define     GR_RESPONE_ENTER_GAME_OK			 (GAME_REQ_BASE_EX+10200)		//�û�������Ϸ�ɹ���������Ϸ��,��ENTER_GAME_INFO�ṹ
#define     GR_RESPONE_ENTER_GAME_DXXW			 (GAME_REQ_BASE_EX+10210)		//�û�������Ϸ�ɹ����������꣬��GAME_TABLE_INFO�ṹ
#define		GR_RESPONE_ENTER_GAME_IDLE			 (GAME_REQ_BASE_EX+10220)		//�û�������Ϸ�ɹ�����Ϸ���½�������ң���GAME_TABLE_INFO�ṹ)


typedef struct _tagGAME_PULSE {
    int nUserID;
    DWORD dwAveDelay;							// 1�����ڵ�ƽ���ӳ�
    DWORD dwMaxDelay;							// ����ͨѶ����ӳ�
    int nReserved[1];
}GAME_PULSE, *LPGAME_PULSE;

typedef struct _tagGET_ROOMUSERS {
    int nAgentGroupID;
    DWORD dwGetFlags;
    int nReserved[3];
    int nRoomCount;//�ýṹ����ʱ�䳤
    int nRoomID[MAX_QUERY_ROOMS];
}GET_ROOMUSERS, *LPGET_ROOMUSERS;

typedef struct _tagITEM_COUNT {
    int nCount;
    int nStatTime;//2038��
    int nSubCount;
    DWORD dwGetFlags;
    int nReserved[1];
}ITEM_COUNT, *LPITEM_COUNT;