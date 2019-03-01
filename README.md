# 机器人工具3.0

# PLATFORM
* WIN10
* VS2013 UPDATE5

# GOAL
* 500机器人,1000 SOCKET调度工具
* 类比 ：简易黑大厅 + 500个客户端管理器
* 输入 ：游戏服务器状态，配置数据
* 输出 : 机器人的精细调度行为

# 拓扑结构
* 机器人工具为前置工具，依赖后端大厅, 游戏

# 整体设计
* 按照网络连接划分数据管理类，并构建【线程安全对象】
* 业务层单线程定时器处理，组合使用【线程安全对象】开发业务
* 因为单线程且使用功能类都为线程安全，不用考虑加业务锁

# 功能类设计
* data中的数据类: user， room，  维护游戏服务器数据层的只读映射
* net/hall_net_manager （1条）   所有机器人的大厅连接和对应数据
* net/game_net_manager （1条）   游戏服务器连接和接收数据层UserMgr,RoomMgr
* net/robot_net_manager（0条）   所有机器人连接管理
* net/robot_net        （1*n条） 单个机器人连接管理
* net/robot_deposit_manager      后台补银还银管理  
* setting/setting_manager        机器人工具配置文件robot.setting
* app_delegate                   业务主线程管理类

# 使用
* 配置robot.setting
* 在 app_delegate 的 MainProcess 中开发业务逻辑
* 默认提供模式是随机指定n个机器人进入m个房间，并保持数量

# DESIGN
* 独立心跳线程，阻塞过程会让心跳超时
* 独立补银线程，补银http请求阻塞较慢
* 独立机器人服通知接收线程，维护只读数据层
* 数据层data为游戏服务器模板层user, room只读映射
* 大厅连接只有一个，所有机器人共用
* 机器人连接n个，用来执行调度行为
* 游戏服务器数据连接只有一个，同步数据状态到data
* 构建线程安全的管理类 ***_mananger
* 上层业务用线程安全对象开发
* 主业务线程为单线程不用业务锁
* 业务配置文件robot.setting
* 运维配置文件RobotTool.ini
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

# NOT SUPPORT
* 不支持多个游戏服务器
* 不支持多个游戏
* 不支持配桌等逻辑
* 不支持ipv6
* 不支持删除机器人

# SUGGESTION
* 对象状态越少越好
* 不出现delete
* 控制对象和线程可见性
* 只读，脏读, 可写数据分离
* 构建相应线程安全级别对象
* 业务逻辑开发不使用非线程安全的对象
* 每个对象可见线程越少越好
* 不应返回任何被mutex保护的数据引用
* 多线程析构对象应用智能指针管理


# TODO
* robot.setting 字段说明
* 保证代码覆盖率在main中加入测试用例
* 玩家状态推导
* 大厅，游戏服务器崩溃后, 状态恢复
* 绑定cpu单核，避免过度抢占cpu
* 内存泄露 内存大小
* 测试大量重连对游戏大厅的瞬时压力，是否有雪崩效应
* 机器人获得银子信息，及时自动补银
* 兜底机制，定时去同步游戏服务器的数据状态如30min
* 异常分支，打印LOG_ERROR或LOG_WARN级别日志和断言
* 加注释
* 每个消息的入口和出口的地方都要有调试级别的日志
* 注意过滤无用消息，避免内存暴涨
* 僵尸回收策略 TickRecoverByDeposit,TickRecoverByTimeout
* support full dump
* relase版编译调试
* DXXW
* 热更新robot.setting
* gamedef 是新模板copy,后期等模板分离
* 对象记录可见线程，非可见线程操作assert
* 静态分析
* 调度效率
* 心跳包里带服务器的身份id，如果不同应该重置数据层
* 所有日志遵循格式，room[%d], table[%d],chair[%d],user[%d]
* 运行信息类 如断线次数,内存大小，峰值
* 先申请一块内存让部分机器人先创建socket，加速登陆过程

# FAQ:


