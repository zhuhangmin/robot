#pragma once

// �����˴�������ID ���ڴ�����֤
extern int				g_nClientID;

// ������exe·��
extern std::string		g_curExePath;

// ������ini�ļ���
extern TCHAR			g_szIniFile[MAX_PATH];

// �����˳��ź���
extern HANDLE			g_hExitServer;

// ��Ϸ������IP����
extern std::string		g_localGameIP;

// ������ʼ����ϱ�ǩ
extern bool             g_inited;

// �����߳�ID
extern uint32_t         g_launchThreadID;

// ҵ�����߳�ID
extern uint32_t         g_mainThreadID;

//@zhuhangmin 20190225 [��������κ�]����ҵ���߼���ȫ�ֱ���
