
#define		PB_REQ_BASE          100000         //������ֵ����̶�,Ӧ�԰汾����

#define     PB_GET_FRIENDS_LIST					(PB_REQ_BASE +    1)  // ��ȡ�����б�
#define     PB_GET_FRIENDS_ONLINESTATUS			(PB_REQ_BASE +    2)  // ��ѯ����״̬
#define     PB_GET_FRIENDS_DEPOSIT				(PB_REQ_BASE +    3)  // ��ѯ��������
#define     PB_INVITE_FRIEND					(PB_REQ_BASE +    4)  // �������
#define     PB_SEARCH_PLAYER_INGAME				(PB_REQ_BASE +    5)  // ��ѯ�û���ǰ����λ��

#define     PB_INVITE_FRIEND_OK					(PB_REQ_BASE +   10)  // ������ѻ�Ӧ
#define     PB_INVITED_BY_FRIEND				(PB_REQ_BASE +   11)  // �������֪ͨ
#define		PB_FRIENDS_ONLINESTATUS				(PB_REQ_BASE +   12)  // ����״̬֪ͨ
#define     PB_INVITE_GIFT						(PB_REQ_BASE +   13)  // ��������֪ͨ


#define     PB_GET_AREA_ROOMS					(PB_REQ_BASE +   20)  // ��ȡ����ͷ����б�

#define		PB_NOTIFY_TO_CLIENT					(PB_REQ_BASE +   200) // ���͸��ͻ��˵�֪ͨ
#define		PB_NOTIFY_ACK						(PB_REQ_BASE +   201) // �ͻ������յ�֪ͨȷ��
#define		PB_GET_NOTIFYS						(PB_REQ_BASE +   202) // ��ȡδ�յ���֪ͨ

#define		PB_UA_LOAD_GAME						(PB_REQ_BASE +   300) // �û���Ϊ�ϱ�-������Ϸ����

//////////////////////////////////////////////////////////////////////////
// PB_REQ_BASE + 1000 ~ PB_REQ_BASE + 2000 ��Ϊ���������ӿڵ�Ԥ����Ϣ
#define		PB_BLOCK_MSG_BEGIN					(PB_REQ_BASE +   1000) // ��ʼ

#define		PB_BLOCK_LV1_BEGIN					(PB_REQ_BASE +   1000) // ��ʼ
#define		PB_HTTP_LV1_BEGIN					(PB_REQ_BASE +   1000) // ��ʼ
#define		PB_GET_GAMELIST						(PB_REQ_BASE +   1001) // ��ȡ�ϼ�������Ϸ�����б�
#define		PB_CHECK_UPDATE						(PB_REQ_BASE +   1002) // ������
#define		PB_GET_MONEY_OWN					(PB_REQ_BASE +   1003) // ��ȡ����ĲƸ���Ϣ
#define		PB_GET_GET_SIGNIN_INFO				(PB_REQ_BASE +   1004) // ��ȡǩ����Ϣ
#define		PB_GET_GET_SIGNIN_AWARD				(PB_REQ_BASE +   1005) // ��ȡǩ������
#define		PB_GET_ADVERTISMENT_ATTEND			(PB_REQ_BASE +   1006) // ������
#define		PB_GET_ADVERTISMENT_GETAWARD		(PB_REQ_BASE +   1007) // ��ȡ��潱��
#define		PB_GET_ADVERTISMENT_GETINFO			(PB_REQ_BASE +   1008) // ��ȡ�������
#define		PB_GET_REGISTER_GETAWARDINFO		(PB_REQ_BASE +   1009) // ���û�ע����ȡ���ֽ���
#define		PB_GET_INVITE_GETINFO				(PB_REQ_BASE +   1010) // ��ȡ������Ϣ������Լ��ģ�
#define		PB_GET_INVITE_ATTEND				(PB_REQ_BASE +   1011) // ������������
#define		PB_GET_INVITE_GETAWARD				(PB_REQ_BASE +   1012) // ��ȡ���뽱��
#define		PB_GET_GIFTPACKAGE_GETAWARD			(PB_REQ_BASE +   1013) // ��ȡ�������
#define		PB_GET_UPDATE_CONFIG				(PB_REQ_BASE +   1014) // ��ȡ�������ã��û��ͻ��˽���չʾ��

#define		PB_GET_BANKPROXY_BANKBALANCE		(PB_REQ_BASE +   1044) // ��ѯ�����˻����
#define		PB_GET_BANKPROXY_EXPIREINFO			(PB_REQ_BASE +   1045) // ��ѯ�����˻�������Ϣ
#define		PB_GET_PAYPROD_LIST					(PB_REQ_BASE +   1046) // ��ȡ��Ʒ�б�
#define		PB_GET_BANKPROXY_WITHDRAW			(PB_REQ_BASE +   1047) // ����ת��
#define		PB_HTTP_LV1_END						(PB_REQ_BASE +   1049) // һ��HTTP��ֹ��Ϣ��
#define		PB_BLOCK_LV1_END					(PB_REQ_BASE +   1099) // һ������ҵ���ֹ��Ϣ��

#define		PB_BLOCK_MSG_END					(PB_REQ_BASE +   1999) // ����
//////////////////////////////////////////////////////////////////////////

#define		PB_REQ_END							(PB_REQ_BASE + 9999)  //���ӵĶ������С�����ֵ


