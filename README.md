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
* 机器人工具为前置工具，依赖后端大厅, 游戏

# 注意！！
* 机器人工具网络库会启动大量线程，和网络连接成正比
* 千量级的线程切换过多占用大量cpu，造成同机游戏服务消息堆积

# 使用
* 配置[robot.setting], [RobotTool.ini] 见配置详细说明
* 默认提供模式是随机指定n个机器人进入m个房间，并保持数量
* 对应的游戏服需要填入机器人工具白名单 
  [RobotSvr]
  Host1=xxxIP
  Host2=xxxIP

# 业务开发：
* 在业务层 app_delegate 的定时器 [MainProcess] 中开发业务逻辑
* 不用考虑[线程安全], [数据时效性]，[网络异常]，[无业务锁]和[业务单线程]客户端类似
* 提供基础管理类都保证线程安全
* 数据管理类都是实时保持后端同步
* 网络管理类内部心跳保活重新，连接层的异常处理自恢复

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

# 热更新 robot.setting
* 只有部分字段提供热更新
* wait_time
* count_per_table
* main_interval
* deposit_interval
* deposit_active_id 
* deposit_gain_url
* deposit_back_url
* deposit_gain_amount
* deposit_back_amount

# 整体设计
* 数据层（配置，网络，运行状态）-> 资源管理类（线程安全，网络异常，数据同步）->具体业务（机器人自定义调度）
* 扩展方案：横向扩展，使用多个机器人工具，而不是扩展业务线程+业务锁容易出问题

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
* 网络库每个连接生成一条线程
* 绑定cpu单核，避免过度抢占cpu


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
* 三点： 1 线程安全, 2 数据时效性，3网络异常自恢复

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
* 连接心跳保活，自动Reset对象数据，并重新Init初始化
* 独立机器人服通知接收线程，维护只读数据层
* 独立心跳线程，阻塞过程会让心跳超时
* 均匀的心跳发送，避免500个心跳集中发送造成压力
* 消息发送错误后，自动Reset对象数据，并重新初始化
* 提供对象状态快照API : SnapShotObjectStatus


# OPTION
* 启动时所有机器人自动补银

# NOTE
* 网络库API接口非线程安全，多线程请注意加锁
* 从游戏服务获得所有房间状态配置，而不是大厅
* 向游戏服务器发送GR_VALID_ROBOTSVR消息注册合法
* 状态需要通过桌椅判断
* ini文件读写非线程安全
* ini文件为启动参数文件，不应加入任何业务配置 
* 游戏服务器提供明确数据状态，不自己推导衍生状态

# NOT_SUPPORT
* 不支持多个游戏服务器
* 不支持多个游戏
* 不支持配桌等逻辑
* 不支持ipv6
* 不支持增加或删除内存中的机器人 robot.setting 中的robot数量不可动态增减
* 不支持定时获得财富信息，后台数据查询压力过大

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


# TODO
* 配置文件应分3份：内网develop，外网内测candidate，正式release
* 避免雪崩效益和状态同步的跷跷板效应，需要发状态量来校验兜底状态
* 保证代码覆盖率在main中加入测试用例
* 注意避免死循环 不断重新发消息 和重连 导致大厅游戏收到攻击
* 延迟重连时间 限制重连次数
* 机器人消息做限流机制，防止攻击后端服务器
* 总消息的记录，避免消息过多攻击服务器
* 玩家状态推导
* 大厅，游戏服务器崩溃后, 状态恢复
* 测试大量重连对游戏大厅的瞬时压力，是否有雪崩效应
* 机器人获得银子信息，及时自动补银
* 兜底机制，定时去同步游戏服务器的数据状态如10min
* 异常分支，打印LOG_ERROR或LOG_WARN级别日志和断言
* 加注释
* 每个消息的入口和出口的地方都要有调试级别的日志
* 僵尸回收策略 TickRecoverByDeposit,TickRecoverByTimeout 需要模板支持兜底
* 卡桌子需要游戏支持兜底
* robot.setting 字段说明
* relase版编译调试
* DXXW
* 横向扩展方案
* 热更新robot.setting
* gamedef 是新模板copy,后期等模板分离
* 调度效率测试
* socket如断线次数
* 协议各种错误统计

# FAQ:


