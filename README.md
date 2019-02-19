#  Ctrl+Shift+P, mdp
#  PLAN:

* 1.22 		机器人流程图，基于最新学东模板流程
* 1.23 - 1.27 开发
* 1.28 		接入新版协议pb
* 1.31 		完成编码
年后配合调试游戏大厅，补测试用例

# TIMELINE
* 1.24	线程同步完备集 thread, mutex, condition


# CONFIG:
	VS2013 UPDATE5


# Objects:

	Server:
		socket（gamesvr）

	HallManager：
		socket （hall）

	ClientsManger：
		client1 :
				socket (gamesvr)
		client2 :
				socket (gamesvr)
		...


# Threads:

* 1 main (Init Objects)

* 1 client scheduler (visiable to all client, 可扩展到n条线程）

* 1 hall pluse keepalive (身份 ，visiable to hall connectin manager 独立，别的阻塞过程会让心跳超时)	

* 1 client pluse keepalive (身份 ，独立，别的阻塞过程会让心跳超时)	

* 1 hall notify recv (visiable to hall connectin manager)

* 1 game notify recv (visiable to all client)

# DESIGN
* 一些关键信息的变化会触发CheckCondition，进一步触发Robot的行为，注意全局锁
* 单独补银线程，因为补银为http请求阻塞较慢，不适合放在Robot中
* 单独心跳线程
* 机器人服务单独connection 用于接受消息
* gamedef 先copy出来用
* 问游戏服务器要roomdata 而不是 大厅
* 用validate消息来区分获取游戏服务器状态的链接
* 状态需要通过桌椅判断，没有（walking,seat,waitting,playing）
// 玩家状态推导：
// 玩家信息中椅子号为0则说明在旁观；
// 玩家信息中token为0则说明玩家离线；
// 有椅子号则查看桌子状态，桌子waiting -> 玩家waiting
// 桌子playing && 椅子playing -> 玩家playing
// 桌子playing && 椅子waiting -> 等待下局游戏开始（原空闲玩家）
enum UserStatus {
    kUserWaiting = 0x00000001,		// 等待游戏开始
    kUserPlaying = 0x00000002,		// 游戏中
    kUserLooking = 0x00000004,		// 旁观
    kUserLeaved = 0x00000008,		// 离开游戏
    kUserOffline = 0x10000000		// 断线
};

* 是否需要补充或消耗银子通过得到的银子数 和房间配置的上下限来触发



机器人讨论： 
1 机器人服务器链接还需要一个特定表示， 
2 client_id 先复用大厅client_id



# 对象和线程可见性
* 一个connection对象 只能一个特定线程可见 （网络库要求）
* 每个对象对可见的线程越少越好
* 每个对象那些方法对那些线程可见，同样的方法应该暴露给尽量少线程


# 重构后：
* 去除没用用到的代码
* Socket 连接数减半，网络库压力更小，调度更快
* 根据用户行为精细化调度机器人

# 拓扑结构：
* 新版机器人服务器定位为前置服务器，依赖后端服务器 1大厅 ， 2 游戏

# 核心业务功能：
* 1000机器人的调度
* 机器人调度的输入参数，游戏里各个玩家状态，配置文件指定参数
* 输出就是调度结果 机器人的行为


# 功能点：	
* Client manager 可以类比黑大厅客户端。但只针对特点游戏特点游戏服务器
* 只有一个hall connection（hall manager），所有client共同管理hall socket
* 每个client '自己管理 connection 连接游戏服务器
* 从大厅获得各个房间状态如人数，从游戏服务器获得具体每个人状态
* client 发现钱比较少 low watermark就去补充银子，而不是在进入前， 影响体验
* 补银队列不会记住刚才想进的房间 和 历史行为解耦，client重新排队
* 不写大client成功失败立即返回， 及时更新到管理器中
* socket error，socket close，数据无法发送就状态reset data 数据层 类似黑大厅
* 任何一方重启说明就不是以前的实体，需要reset从后端拿到的数据层
* 整个clientmanager 没有socket 需要管理
* 对象hallconnectionmanager 管理 hall socket 
* 每个client自己管理socket 链接游戏服务器
* 对象 mainsvr 管理socket连接 游戏服务器
* 大厅链接被manager 和所有client共用
* 加入运行信息类 ，如链接游戏服务器的断线重置次数
* 先申请一块内存让部分机器人先创建socket，加速登陆过程
* 在client里 OnRequest 不要调用OnDestroyEx, 线程里不能销毁自己
* 机器人无感dxxw ，直连游戏服务器，由游戏服务器决定是否dxxw
* ini 文件非线程安全，把非经常修改运营的字段放入json中热更新
* ini 文件为启动参数文件, 运维修改，不应放入任何业务相关参数,运维和运营不修改同一个文件
* 消灭全局变量g_xxx to appConfigManger read from robot.setting
* 支持termianl输入命令自定义调试
* support full dump

# 选项支持：
* 实时性要求不高的机器人可以进程绑定特定cpu，避免抢占同机其他服务CPU

# 参考
* muduo的实现 https://github.com/chenshuo/muduo/tree/cpp11

# 工程风格：
* C++11
* 类的noncopyable
* 可测试，一级二级api组合设计
* 类unix错误码支持
* 使用google protobuffer（模板买道具）
* 安全单例 std::call_once
* 不返回任何被mutex保护的数据
* 对象里记录可见线程，非可见此线程操作就assert报错
* 函数颗粒尽量小
* 前置申明解耦
* 不出现delete
* 使用更高效的google string 类 StringPiece.h  replace CString
* https://github.com/chenshuo/muduo/blob/master/muduo/base/StringPiece.h
* 加入test
* YQ (yiqing) namespace
* 数据驱动
* std::string  不是“字符串”，是“字节串”
* std::string  容器和算法分离， 不是大而全的对象，而是容器，通过迭代器和算法algorithm模块结合来实现trim，split功能，
* std::string  没有编码信息，因为是“字节串”，但c++11 可以通过std::codecvt
* std::string  性能确实是问题，因为每次修改都copy，但可以通过google的实现stringPiece来解决
* DCHECK,CHECK, 两种 CHECK 是let it crash， DCHECK 是debug的时候起作用
* 定时器类

# Code Style
* Filenames should be all lowercase: my_useful_class.cc
* C++ files should end in .cc and header files should end in .h.
* Type names : MyExcitingClass, MyExcitingEnum.
* const int kAndroid8_0_0 = 24;  // Android 8.0.0
* struct UrlTableProperties { string name; }
* Avoid using forward declarations where possible. Just #include the headers you need.
* You may not use a using-directive to make all names from a namespace available.
* Use a struct only for passive objects that carry data; everything else is a class.
* All parameters passed by lvalue reference must be labeled const.

# 不支持：
* 不支持多个游戏服务器
* 不支持多个游戏
* 不支持配桌等逻辑
* 不支持ipv6
* 不支持删除机器人


# 分析测试：
* 内存分析 静态分析 运营效率 内存泄露
* python写后端服务器mock
* 三次握手完成，第一个data发送前，后端崩溃了，没发socket close, socket error
* 注意同时大量重连对游戏服务器，大厅的瞬时压力，雪崩效应
* 关键流程测试覆盖

# FAQ:
* 获得机器人具体数据 如deposit? 大厅消息 QUERY_USER_GAMEINFO SOLO_PLAYER nDeposit
* client 的socket 消息在哪里接受？notify recv thread （register create connection）
* Socket close, error, 发送数据失败都要销毁 DestroyEx？ pluse keep alive 服务器身份标识
* 像ludosvr的服务器是哪里销毁destroyex 的？ 和create 同一个线程
* 大量client的心跳如何处理？

# 讨论：
* 心跳包里带服务器的身份id，表明是否是同一个服务器，如果不是同一个应该重置对应的数据层
* 网络库的connection所有的操作特定一个线程可见如create，detroyex（用了客户端网络库，非线程安全）
* 模板层会给机器人服务器连接提供一个特定标识，类似黑大厅
* 需要有兜底机制，定时去同步后端服务器的数据状态
* 后端服务器提供的数据状态明确，前端服务器不自己推导衍生状态
* 抽象逻辑和特定自定义逻辑分层，数据配置驱动支持自定义逻辑，是否要主动配桌
* 所有异常的分支，都应该打印LOG_ERROR或LOG_WARN级别日志和添加断言。
* 所有日志打印房间，桌子，椅子，玩家的地方，遵循格式，room[%d], table[%d],chair[%d],player[%d]。
* 注意添加注释。特别是一些魔法数或者魔法状态的地方，一定要加注释。
* 每个消息的入口和出口的地方都要有调试级别的日志
* 运行期间，读取配置文件，增删机器人数量、账号。不用重启服务。
* 注意接受太多消息，内存暴涨
* condition variable 需要std::unique<std::mutex>
* 在wait时,cv 会调用 lock , 等待条件满足后 调用 unlock
* 不使用lock_guard是因为没有 lock, unlock api


# 机器人笔记：
MAX_PROCESSORS_SUPPORT	4 最多4* 2八个工作线程

在connection 创建时注册特定的 notify thread （game notify ， hall notify）
virtual BOOL Create(LPCTSTR lpszIP, int nPort, DWORD dwWaitSeconds = DEF_CONNECT_WAIT,
							HWND hNotifyWnd = NULL, DWORD dwNotifyThrd = 0, 
							BOOL bConnectWithEcho = TRUE, void* pHelloData = NULL, int nHelloLen = 0,
							DWORD dwFirstWait = ACCEPT_OK_WAIT, int LocalPort = 0);


2 江林支持不同ip的部署ini配置
3 后台配置
机器人账号
机器人账号需要CP方自行解决，并把机器人账号上报至 区浩、高云峰 处登记备案
海外 ： 账号 和 密码 和 昵称 和 头像
{"Account":12321,"Password":"1101101","NickName":"hello","Portrait":"long long url"},
机器人活动（补银）
每个游戏需要独立申请一个机器人活动 用来补银 内网活动id 33
lpPlayer->nUserType USER_TYPE_ROBOT

4 【socket连接】
 一个大厅连接， 一个“新游戏服务器”连接同步整个服务器状态， 每个client一个连接

CrobotMgr 维护信息：
1 所有机器人客户端
2 所有游戏服务器，和 房间服务器
3 特定的机器人在哪个房间和游戏服务器上
CrobotClient 机器人客户端， 玩家状态，连游戏服务器和房间服务器（多个）
4 m_mapGRoomSett 房间信息 mode，value
5 m_mapAcntSett 机器人 信息
6 CRobotClient::SendEnterGame 进游戏
流程：（大量同步过程）
主定时器2秒触发
TimerThreadProc DEF_TIMER_INTERVAL 
房间子流程 15 秒触发一次
OnTimerCtrlRoomActiv 
define MAIN_ROOM_ACTIV_CHECK_GAP_TIME (15) 
登陆房间流程 ApplyRobotForRoom
触发大厅登陆 RobotLogonHall
触发连接房间ConnectRoom PORT 30629 默认
进房间 CRobotClient::SendEnterRoom （GR_ENTER_ROOM）


房间服务器机器人配置RoomSvr.ini
回收策略
TickRecoverByDeposit 钱不够
TickRecoverByTimeout 超时
每20s回收不在玩的机器人，为了解决无法识别成为僵尸的机器人
Roomsvr.ini中 RecoveHowLong_Room 字段控制

2 “玩“状态的机器人无法清除（机器人，房间，游戏服务器三方飞服后，状态同步问题）
