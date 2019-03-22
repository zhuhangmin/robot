#pragma once

// 机器人大厅连接ID 用于大厅验证
extern int				g_nClientID;

// 机器人exe路径
extern std::string		g_curExePath;

// 机器人ini文件名
extern TCHAR			g_szIniFile[MAX_PATH];

// 进程退出信号量
extern HANDLE			g_hExitServer;

// 游戏服务器IP配置
extern std::string		g_localGameIP;

// 启动初始化完毕标签
extern bool             g_inited;

// 启动线程ID
extern uint32_t         g_launchThreadID;

// 业务主线程ID
extern uint32_t         g_mainThreadID;

//@zhuhangmin 20190225 [请勿添加任何]具体业务逻辑的全局变量
