#include "stdafx.h"
#include "process_info.h"
#include <TLHELP32.H>
#include <psapi.h>
#pragma comment(lib,"psapi.lib")

BOOL  ProcessInfoCollect::GetCPUUserRate(double & dCPUUserRate) {
    HANDLE hProcess = ::GetCurrentProcess();
    return GetCPUUserRateEx(hProcess, dCPUUserRate);
}

BOOL    ProcessInfoCollect::GetCPUUserRate(DWORD lProcessID, double & dCPUUserRate) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, lProcessID);
    if (hProcess == NULL)
        return FALSE;

    BOOL bSuccess = GetCPUUserRateEx(hProcess, dCPUUserRate);
    CloseHandle(hProcess);
    return bSuccess;
}

int ProcessInfoCollect::GetIOBytes(ULONGLONG * read_bytes, ULONGLONG * write_bytes, ULONGLONG * wct, ULONGLONG * rct) {
    HANDLE hProcess = GetCurrentProcess();
    int nRet = GetIOBytesEx(hProcess, read_bytes, write_bytes, wct, rct);
    return nRet;
}

int ProcessInfoCollect::GetIOBytes(DWORD lProcessID, ULONGLONG * read_bytes, ULONGLONG * write_bytes, ULONGLONG * wct, ULONGLONG * rct) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, lProcessID);
    if (hProcess == NULL)
        return -1;

    int nRet = GetIOBytesEx(hProcess, read_bytes, write_bytes, wct, rct);
    CloseHandle(hProcess);
    return nRet;
}

BOOL    ProcessInfoCollect::GetHandleCount(DWORD &dwHandles) {
    return GetHandleCountEx(GetCurrentProcess(), dwHandles);
}
BOOL    ProcessInfoCollect::GetHandleCount(DWORD lProcessID, DWORD &dwHandles) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, lProcessID);
    if (hProcess == NULL)
        return FALSE;

    BOOL bSuccess = GetHandleCountEx(hProcess, dwHandles);
    CloseHandle(hProcess);
    return bSuccess;
}

BOOL    ProcessInfoCollect::GetMemoryUsed(DWORD & dwPeakWorkingSetSize, DWORD & dwWorkingSetSize) {
    HANDLE hProcess = GetCurrentProcess();
    return GetMemoryUsedEx(hProcess, dwPeakWorkingSetSize, dwWorkingSetSize);
}

BOOL    ProcessInfoCollect::GetMemoryUsed(DWORD lProcessID, DWORD & dwPeakWorkingSetSize, DWORD & dwWorkingSetSize) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, lProcessID);
    if (hProcess == NULL)
        return FALSE;

    BOOL bSuccess = GetMemoryUsedEx(hProcess, dwPeakWorkingSetSize, dwWorkingSetSize);
    CloseHandle(hProcess);
    return bSuccess;
}

BOOL    ProcessInfoCollect::GetCPUUserRateEx(HANDLE hProccess, double & dCPUUserRate) {
    static DWORD s_dwTickCountOld = 0;
    static LARGE_INTEGER s_lgProcessTimeOld = {0};
    static DWORD s_dwProcessorCoreNum = 0;
    if (!s_dwProcessorCoreNum) {
        SYSTEM_INFO sysInfo = {0};
        GetSystemInfo(&sysInfo);
        s_dwProcessorCoreNum = sysInfo.dwNumberOfProcessors;
    }
    double dbProcCpuPercent = 0;
    BOOL bRetCode = FALSE;
    FILETIME CreateTime, ExitTime, KernelTime, UserTime;
    LARGE_INTEGER lgKernelTime;
    LARGE_INTEGER lgUserTime;
    LARGE_INTEGER lgCurTime;
    bRetCode = GetProcessTimes(hProccess, &CreateTime, &ExitTime, &KernelTime, &UserTime);
    if (bRetCode) {
        lgKernelTime.HighPart = KernelTime.dwHighDateTime;
        lgKernelTime.LowPart = KernelTime.dwLowDateTime;
        lgUserTime.HighPart = UserTime.dwHighDateTime;
        lgUserTime.LowPart = UserTime.dwLowDateTime;
        lgCurTime.QuadPart = (lgKernelTime.QuadPart + lgUserTime.QuadPart);
        if (s_lgProcessTimeOld.QuadPart) {
            DWORD dwElepsedTime = ::GetTickCount() - s_dwTickCountOld;
            dbProcCpuPercent = (double) (((double) ((lgCurTime.QuadPart - s_lgProcessTimeOld.QuadPart) * 100)) / dwElepsedTime) / 10000;
            dbProcCpuPercent = dbProcCpuPercent / s_dwProcessorCoreNum;
        }
        s_lgProcessTimeOld = lgCurTime;
        s_dwTickCountOld = ::GetTickCount();
    }
    dCPUUserRate = dbProcCpuPercent;
    return bRetCode;
}
int     ProcessInfoCollect::GetIOBytesEx(HANDLE hProccess, ULONGLONG * read_bytes, ULONGLONG * write_bytes, ULONGLONG * wct, ULONGLONG * rct) {
    IO_COUNTERS io_counter;
    if (GetProcessIoCounters(hProccess, &io_counter)) {
        if (read_bytes) *read_bytes = io_counter.ReadTransferCount;
        if (write_bytes) *write_bytes = io_counter.WriteTransferCount;
        if (wct) *wct = io_counter.WriteOperationCount;
        if (rct) *rct = io_counter.ReadOperationCount;
        return 0;
    }
    return -1;
}

BOOL    ProcessInfoCollect::GetMemoryUsedEx(HANDLE hProccess, DWORD & dwPeakWorkingSetSize, DWORD & dwWorkingSetSize) {
    if (hProccess) {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        OSVERSIONINFO osvi;
        memset(&osvi, 0, sizeof(OSVERSIONINFO));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&osvi);
        if (osvi.dwMajorVersion < 6) {
            PROCESS_MEMORY_COUNTERS pmc;
            pmc.cb = sizeof(PROCESS_MEMORY_COUNTERS);

            if (::GetProcessMemoryInfo(hProccess, &pmc, sizeof(pmc))) {
                dwWorkingSetSize = pmc.PagefileUsage;
                dwPeakWorkingSetSize = pmc.PeakWorkingSetSize;
            }
        } else {
            DWORD dwMemProcess = 0;
            PSAPI_WORKING_SET_INFORMATION workSet;
            memset(&workSet, 0, sizeof(workSet));
            BOOL bOk = QueryWorkingSet(hProccess, &workSet, sizeof(workSet));
            if (bOk || (!bOk && GetLastError() == ERROR_BAD_LENGTH)) {
                int nSize = sizeof(workSet.NumberOfEntries) + workSet.NumberOfEntries * sizeof(workSet.WorkingSetInfo);
                char* pBuf = new char[nSize];
                if (pBuf) {
                    QueryWorkingSet(hProccess, pBuf, nSize);
                    PSAPI_WORKING_SET_BLOCK* pFirst = (PSAPI_WORKING_SET_BLOCK*) (pBuf + sizeof(workSet.NumberOfEntries));
                    DWORD dwMem = 0;
                    for (ULONG_PTR nMemEntryCnt = 0; nMemEntryCnt < workSet.NumberOfEntries; nMemEntryCnt++, pFirst++) {
                        if (pFirst->Shared == 1) dwMem += si.dwPageSize;
                    }
                    delete pBuf;
                    if (workSet.NumberOfEntries > 0) {
                        dwMemProcess = dwMem;
                        dwWorkingSetSize = dwMemProcess;
                        dwPeakWorkingSetSize = dwMemProcess;
                    }
                }
            } else {
                return FALSE;
            }
        }
    } else {
        int ret = GetLastError();
        return FALSE;
    }
    return TRUE;
}


BOOL ProcessInfoCollect::GetHandleCountEx(HANDLE hProccess, DWORD &dwHandles) {
    return GetProcessHandleCount(hProccess, &dwHandles);
}


BOOL    ProcessInfoCollect::GetThreadCount(DWORD &dwThreads) {
    return GetThreadCount(GetCurrentProcessId(), dwThreads);
}
BOOL ProcessInfoCollect::GetThreadCount(DWORD lProcessID, DWORD &dwThreads) {

    HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
        return FALSE;

    BOOL bFind = FALSE;
    char szFilePath[MAX_PATH] = {0};
    PROCESSENTRY32 stProcessEntry32 = {0};
    stProcessEntry32.dwSize = sizeof(stProcessEntry32);
    BOOL bSucceed = ::Process32First(hProcessSnap, &stProcessEntry32);;
    for (;;) {
        if (!bSucceed)
            break;

        if (stProcessEntry32.th32ProcessID == lProcessID) {
            dwThreads = stProcessEntry32.cntThreads;
            bFind = TRUE;
            break;
        }
        bSucceed = ::Process32Next(hProcessSnap, &stProcessEntry32);
    }
    ::CloseHandle(hProcessSnap);
    return bFind;
}
