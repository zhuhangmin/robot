// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���ǳ��õ��������ĵ���Ŀ�ض��İ����ļ�
//

#pragma once
#include <tcns.h>
#include <xyapi.h>
#include <afxinet.h>
// TODO: �ڴ˴����ó���Ҫ��ĸ���ͷ
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <thread>
#include <fstream>
#include <iostream>
#include <functional> 

#include <xygext2.h>
#include <DefIocpServer.h>
#include <CommonData.h>
#include <GameDef.h>
#include <MobileReq-.h>
#include <GameReq-.h>

#include <IPPort.h>		//����IPPort.h��ȥ��TcyPort.h 20150506

#include "RobotDef.h"
#include "RobotReq.h"
#include "SockSvr.h"
#include "DefSocketClient.h"
#include "Server.h"

#include "RobotClient.h"
#include "ExtInterface.h"
#include "RobotMgr.h"

#include "SvrInOut.h"
#ifdef _DEBUG
#pragma comment(lib,"tcElkInOut-vc12d.lib")
#else
#pragma comment(lib,"tcElkInOut-vc12.lib")
#endif 

#include "Main.h"
#include "Service.h"



