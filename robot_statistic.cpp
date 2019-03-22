#include "stdafx.h"
#include "robot_utils.h"
#include "robot_statistic.h"
#include "process_info.h"

int RobotStatistic::Event(const EventType& type, const EventKey& key) {
    /*CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);
    if (type == EventType::kSend) {
    send_msg_count_map[key]++;
    }

    if (type == EventType::kRecv) {
    recv_msg_count_map[key]++;
    }

    if (type == EventType::kErr) {
    error_msg_count_map[key]++;
    }*/
    return kCommSucc;
}

int RobotStatistic::SnapShotObjectStatus() {
    //#ifdef _DEBUG
    //    std::lock_guard<std::mutex> lock(mutex_);
    //    LOG_INFO("[SEND] : ");
    //    for (auto& kv : send_msg_count_map) {
    //        LOG_INFO("\t count [%I64d]\t [%s %d]",
    //                 kv.second, REQ_STR(kv.first), kv.first);
    //    }
    //
    //    LOG_INFO("[RECV] : ");
    //    for (auto& kv : recv_msg_count_map) {
    //        LOG_INFO("\t count [%I64d]\t [%s %d]",
    //                 kv.second, REQ_STR(kv.first), kv.first);
    //    }
    //
    //    LOG_INFO("[ERROR] : ");
    //    for (auto& kv : error_msg_count_map) {
    //        LOG_INFO("\t count [%d]\t [%s %d]",
    //                 kv.second, ERR_STR(kv.first), kv.first);
    //    }
    //#endif

    return kCommSucc;
}

int RobotStatistic::ProcessStatus() {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("-------------[PROCESS TEST BEG]-------------");
    ProcessInfoCollect picProcessInfoCollect;
    DWORD             nMemoryUsed;                    //内存使用(Byte)    
    DWORD            nVirtualMemoryUsed;                //虚拟内存使用(Byte)    
    DWORD            nHandleNumber;                    //句柄数量
    DWORD dwCurrentProcessThreadCount;        //线程数量    
    ULONGLONG ullIo_read_bytes;                        //IO读字节数    
    ULONGLONG ullIo_write_bytes;                    //IO写字节数    
    ULONGLONG ullIo_wct;                            //IO写次数    
    ULONGLONG ullIo_rct;                            //IO读次数        
    double dCPUUserRate = 0;                        //CPU使用的百分比        
    picProcessInfoCollect.GetCPUUserRate(dCPUUserRate);
    picProcessInfoCollect.GetMemoryUsed(nVirtualMemoryUsed, nMemoryUsed);
    picProcessInfoCollect.GetThreadCount(dwCurrentProcessThreadCount);
    picProcessInfoCollect.GetHandleCount(nHandleNumber);
    picProcessInfoCollect.GetIOBytes(&ullIo_read_bytes, &ullIo_write_bytes, &ullIo_wct, &ullIo_rct);
    LOG_INFO("cpu rate [%f], memory [%d] MB, handler count [%d], thread count [%d]",
             dCPUUserRate, (int) nMemoryUsed / 1024 /1024, (int) nHandleNumber, (int) dwCurrentProcessThreadCount);
    LOG_INFO("-------------[PROCESS TEST END]-------------");
    return kCommSucc;
}