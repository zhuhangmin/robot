# 机器人工具3.0

# 开发平台
* WIN10
* VS2013 UPDATE5

# 目标
* 500机器人, 500 SOCKET调度工具
* 类比 ：简易黑大厅 + 500个客户端管理器
* 输入 ：游戏服务器状态，配置数据
* 输出 : 机器人的精细调度行为

# 拓扑结构
*  机器人工具为前置工具，强依赖后端大厅服务器, 游戏服务器
*  若后端没有启动,机器人工具无法启动

# [!注意!]线程切换cpu消耗
* 机器人工具网络库会启动大量线程，和网络连接成正比
* 千量级的线程切换过多占用大量cpu，造成同机游戏服务消息堆积
* 机器人越多，网络接口会越慢，如果业务设计10s一个机器人，因考虑此效应参数设计成7-8s

# [!注意!]网络库reponse造成SocketClose
* 如果网络库sessionid解析成负数。检查后端服务器是否发了一个不存在的response给机器人工具
* 如果游戏服务模拟发了需要应答的request的消息, 最后需要过滤掉reponse, 不用发给机器人工具
* 发送不存在的response会造成机器人网络库触发onError，应用层收到【UR_SOCKET_ERROR】消息
* 机器人除了进房间调度消息以外，应该只接受通知

# [!注意!]n个机器人集中发送消息请求对后端服务器压力
* 对集中消息发送，需要做限流处理，典型的如大量机器人启动时，断线重连，集中心跳
* 使用 #define CURRENT_DELAY // 限流延时标签 默认为100ms间隔

# [!注意已知bug!]300个机器人测试时发现已知问题
* 1600多次enter game消息中1次失败，原因是机器人网络自己产生了一个Socket Error。

# [!注意已知bug!]2个小时的400机器人的暴力测试，机器人服务总连接收到20次 Socket Error
* 因是机器人网络自己产生了Socket Error，目前使用这个已知bug来兜底重新同步游戏服务器所有数据

# 设计图

![image](https://github.com/zhuhangmin/robot/blob/master/image/design.png)


# 使用
* 配置[robot.setting], [RobotTool.ini] 见配置详细说明
* 默认提供模式是随机指定n个机器人进入m个房间，并保持数量
* 最大最小房间银子 和 桌银应自洽，如 桌银max < 进房间最大银子max，让补完桌银的机器人可以再次进入房间
* 补银线程时间间隔应
* 对应的游戏服需要填入机器人工具白名单 
  [RobotSvr]
  Host1=xxxIP
  Host2=xxxIP
* robot.setting 中所有房间配置的ip , game port 应一致
* ReleaseS 需要在ini文件中填写xxx具体游戏后缀 EX:
  service_name=robottoolxxx（robottoolrumm）
  display_name=robottoolxxx（robottoolrumm）
* 游戏监听端口和黑大厅game port 应一致， telnet检测

# robot.setting 业务配置说明
* game_id               // 游戏id
* main_interval         // 业务主线程定时器间隔 单位ms
* deposit_interval      // 补银单线程定时器间隔 单位ms
* deposit_active_id     // 后台配置补银还银活动ID
* deposit_gain_url      // 后台补银服务地址
* deposit_back_url      // 后台还银服务地址
* deposit_gain_amount   // 默认单次补银数量
* deposit_back_amount   // 默认单次还银数量
* 房间配置 roomid       // 需要机器人的房间ID
* 房间配置 wait_time    // 机器人等待时间 注意每个游戏有前置业务条件
* 房间配置 count_per_table  // 每桌最多上多少机器人
* 机器人 userid        // 机器人平台用户id
* 机器人 password      // 机器人平台用户密码

# 机器人的头像，昵称已废弃
* 由后台同事负责,机器人工具不用配置

# robottool.ini 运维同事配置
*大厅服务ip port
 [hall_server]
 ip=192.168.103.108
 port=30626
*连接游戏ip, 不支持域名, 网络库不支持ipv6解析
[local_ip]
ip=192.168.44.41
*正式部署的服务名,显示名
[service]
service_name=robottoolrumm
display_name=robottoolrumm


# 线程类型定义：
* 启动线程1条：可以获得任意锁，但只启动和退出时，(服务模式会单独启动一条线程)
* 业务线程1条：可以获得任意锁，在启动完毕之后
* 内部线程n条：只能获取对象内部锁和（最后获得）银子锁，不允许获得别的数据锁

# 业务开发：
* 每次编译前先更新到模板最新的pb定义 game_base.pb.h game_base.pb.cc game_base.proto
* 在业务层 app_delegate 的定时器 [MainProcess] 中开发业务逻辑
* 不用考虑[线程安全], [数据时效性]，[网络异常]，[无业务锁]和[业务单线程]客户端类似
* 只使用提供的基础管理类开发业务，保证线程安全
* 不直接使用应该被锁保护的引用对象
* 数据管理类都是实时保持后端同步
* 网络管理类内部心跳保活重新，连接层的异常处理自恢复
* [MainProcess]业务开发过程不关心网络消息接口


# 关于银子
* 银子只用大厅和游戏
* 银子数据锁只内部线程使用时，保证调用顺序，是最后一把锁
* 银子的生命周期常驻对应平台数据层，不随大厅和游戏的内存变化而变化
* 桌银区间 左闭右开 [20000, 20001) 
* 补银大小限制  (1,000,000,000后台确认)
* 补银过程： 业务主线程-》http补银-》大厅拉去最新值-》设置data层
* 银子请只使用deposit_data_manager 为准

# 调试
* 在main函数 TEST CASE 下添加单元测试用例，请注意测试用例非线程安全
* 开启Application Veritfy 检查堆错误
* resharper pvs-studio 静态分析 
* DEBUG_NEW 检测内存泄露
* Terminal 中输入'P' 打印进程使用的资源信息
* Terminal 中输入'S' 打印业务类数据状态
* Terminal 中输入'M' 打印发送消息数
* TRACE_STACK 打印调用栈
* TRACE_ASSERT 开启严格调试模式, 错误时打印调用栈TRACE_STACK, 并assert断言
* CHECK_XXX 参数合法性检查
* LOG_ROUTE 业务流程日志
* ERR_STR(x) 详细错误码信息
* REQ_STR(x) 协议定义
* 协议以及各种错误统计
* 开启多核编译
* Properties -> Configuration Properties -> C/C++ -> Code Generation -> Enable Minimal Rebuild -> No(/Gm-)
* Properties -> Configuration Properties -> C/C++ -> Geneal -> Multi-processor Compilation -> Yes(/MP)

# 部署运维
* 如果机器人和游戏服部署在不同时区 需要矫正时区(TODO)
* 上传业务配置robot.setting。填写对应外网房间，补银服务
* 上传运维配置RobotTool.ini。填写对应游戏服务ip地址
* 对应的游戏服务器需要加上机器人白名单
* 房间ip和game port应于监听实际端口一致
* 房间银上下限区间 > 桌银上下限区间
* 桌银上下限区间[左闭右开）
* 注意机器人是默认随机单核运行

# 后端拓扑结构，决定了IO结构，
* 如大厅网络IO，游戏网络IO，后台，本地文件
* IO输入输出产生程序的内存数据，加锁封装成mutex对象，如文件IO
* 从IO角度划分数据对象, 对应数据锁, 控制数据生命周期


# 整体设计
* 数据层（配置，网络，运行状态）-> 资源管理类（线程安全，网络异常，数据同步）->具体业务（机器人自定义调度）
* 扩展方案：横向扩展，使用多个机器人工具，而不是扩展业务线程+业务锁容易出问题

# 封装的线程执行顺序含义
* 封装可以强制代码执行顺序 ，如只出现A->B , 不会同时出现A->B 和 B->A，导致死锁

# 数据层
* 业务配置文件robot.setting
* 运维配置文件RobotTool.ini
* 数据层data为游戏服务器模板层user, room,table, chair 只读映射
* 大厅登陆状态
* 补银还银队列
* 原始数据 setting.setting， robottool.ini, data
* 衍生数据 

# 网络连接
* 大厅连接只有1个，所有机器人共用
* 游戏连接只有1个，同步游戏服务器数据层
* 机器人连接n个，执行调度行为
* 补银还银http，通过后台活动实现

# 资源和消息控制
* 网络库每个连接生成一条线程，导致500的线程频繁切换
* 绑定cpu单核，避免过度抢占所有cpu

# 数据管理类
* common/robot_utils             无状态类,无任何成员变量线程安全
* RobotTool.ini                  运维配置文件，不含任何具体游戏业务
* setting/setting_manager        机器人工具配置文件robot.setting
* data/user_manager              管理游戏服务器所有用户数据（包括机器人）
* data/room_mamager              管理游戏服务器所有房间数据（桌椅等）
* net/hall_net_manager （1条）   所有机器人的大厅连接和对应数据
* net/game_net_manager （1条）   游戏服务器连接和接收数据层UserMgr,RoomMgr()
* net/robot_net_manager（0条）   所有机器人连接管理
* net/robot_net        （1*n条） 单个机器人连接管理
* net/robot_deposit_manager（http）后台补银还银管理  
* app_delegate                   业务主线程管理类

# 对象设计：
* 四点： 1 线程安全, 2 数据时效性，3网络异常自恢复, 4 数据调用层次封装

# 线程安全
* 保持一个对象一把锁对应关系,防止多把锁嵌套造成死锁
* 发现必须有多把锁时说明对象数据聚合太多，需要分成多个对象
* 抽象接口不含具体消息号，实例化接口处理具体消息
* 请勿在抽象接口中调用实例化接口，反之可以
* 线程分两种, 只对内部线程可见，对外部线程也可见
* 无WithLock后缀函数：如果方法有多线程可见数据 一般都需要加锁，除非业务层次允许脏读
* 组合 lock + WithLock 函数组合而成, 保证不会获得多次mutex，std::mutex 为非递归锁
* 限定方法可见线程，非可见线程操作返回

# 数据时效性 网络异常自恢复
* Init自动获取必要数据初始化对象数据
* 通过Notify同步后端服务器实时状态
* 10min一次的游戏服务器所有用户和房间桌椅数据同步兜底
* 连接心跳保活，自动Reset对象数据，并重新Init初始化
* 独立机器人服通知接收线程，维护只读数据层
* 独立心跳线程，阻塞过程会让心跳超时
* 均匀的心跳发送，避免500个心跳集中发送造成压力
* 消息发送错误后，自动Reset对象数据，并重新初始化
* 提供对象状态快照API : SnapShotObjectStatus

# 数据调用层次封装
* 管理类封装只读数据对象，保证了从逻辑线程调用时的强制顺序
* 逻辑线程-> 管理类（mutex）-> 只读数据类（无锁）
* 封装保证执行流的顺序，强制保证了各个锁的获取顺序，避免死锁

# 机器人工具在线重启后
* 1 所有机器人登陆大厅
* 2 原先在游戏的机器人EnterGame 触发游戏服务器中的断线续玩流程

# 后端游戏服务器在线重启后
* 1 重新建立游戏服务器连接
* 2 重新建立所有在线的机器人游戏连接

# NOTE
* 网络库API接口非线程安全，多线程请注意加锁
* 从游戏服务获得所有房间状态配置，而不是大厅
* 向游戏服务器发送GR_VALID_ROBOTSVR消息注册合法
* 状态需要通过桌椅判断
* ini文件读写非线程安全
* ini文件为启动参数文件，不应加入任何业务配置 
* 游戏服务器提供明确数据状态，不自己推导衍生状态
* game net manager类不应该出现具体业务字段
* 业务定义字段应出现在room table chair user 具体数据类
* 具体数据类负责如room table chair user跟踪游戏服务器数据状态的变化
* 游戏模板默认游戏开始后有倒计时5秒
* 注意游戏开始后，会有桌子上的玩家下局准备玩的玩家，状态是waitting
* TableUserInfo 包括旁观
* 机器人离开游戏时，模板层主动断开连接，降低游戏连接数
* 内部线程里请无调用其他有锁对象（银子除外），保证业务独立，避免死锁

# NOT_SUPPORT
* 500个机器人不发心跳,因为机器人工具部署在内网，不通过代理服务器
* 不支持robot.setting的热更新
* 不支持robot.setting中配置的房间ip port不一致，必须一样
* 不支持多个游戏服务器
* 不支持多个游戏
* 不支持配桌等逻辑
* 不支持ipv6
* 不支持增加或删除内存中的机器人 robot.setting 中的robot数量不可动态增减
* 不支持定时获得财富信息，后台数据查询压力过大
* 不支持昵称，头像

# SUGGESTION
* 只读，脏读, 可写数据分离
* 状态越少越好
* 状态可见线程越少越好
* 控制对象和线程可见性
* 构建相应线程安全级别对象
* 业务逻辑开发不使用非线程安全的对象
* 尽量不返回被mutex保护的数据引用
* 不直接操作raw memory如new和delete
* 注意智能指针pitfall
* 多线程析构对象应用智能指针管理
* Macro宏只用在参数检查，打印日志，不做业务逻辑
* const的使用 参数，方法，返回值三处都应添加
* const的方法对多线程“读”安全，可重入
* 检查所有用户user类型为kRobot，防止误操作真实用户
* 记录本机HardID方便大厅排查问题
* 数据对象，管理器，业务层次封装分明，强制保证执行流顺序，避免死锁

# TODO
* release网络库无日志的错误
* 机器人进入游戏失败统计
* 接入钉钉报警如cpu过高,机器人消耗完等
* thread local 重构错误码
* 抽象mixin, 如timer, connect, config代码复用度更高

