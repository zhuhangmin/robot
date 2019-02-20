// stdafx.h : 标准系统包含文件的包含文件，
// 或是常用但不常更改的项目特定的包含文件
//

#pragma once
#include <tcns.h>
#include <xyapi.h>
#include <afxinet.h>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <thread>
#include <fstream>
#include <iostream>
#include <functional> 
#include <ctime>
#include <hash_map>
#include <array>

#include <json.h>
#include <xysoap.h>
#include <dbghelp.h> 

#include <xygext2.h>
#include <DefSocketClient.h>
#include <DefIocpServer.h>
#include <CommonData.h>
#include <GameDef.h>
#include <MobileReq-.h>
#include <GameReq-.h>


#include <IPPort.h>		//开放IPPort.h，去掉TcyPort.h 20150506

//@zhuhangmin 20192015 请勿在预编译中加入任何具体业务逻辑相关文件！！
#include "game_base.pb.h"
#include "gamedef.h"
#include "google/protobuf/text_format.h" 


