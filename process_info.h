#pragma once

class ProcessInfoCollect {
public:
    BOOL    GetCPUUserRate(double & dCPUUserRate);

    int     GetIOBytes(ULONGLONG * read_bytes, ULONGLONG * write_bytes, ULONGLONG * wct, ULONGLONG * rct);

    BOOL    GetMemoryUsed(DWORD & dwPeakWorkingSetSize, DWORD & dwWorkingSetSize);

    BOOL    GetHandleCount(DWORD &dwHandles);


    BOOL    GetThreadCount(DWORD &dwThreads);

protected:
    BOOL    GetMemoryUsed(DWORD lProcessID, DWORD & dwPeakWorkingSetSize, DWORD & dwWorkingSetSize);

    BOOL    GetHandleCount(DWORD lProcessID, DWORD &dwHandles);

    BOOL    GetThreadCount(DWORD lProcessID, DWORD &dwThreads);

    int     GetIOBytes(DWORD lProcessID, ULONGLONG * read_bytes, ULONGLONG * write_bytes, ULONGLONG * wct, ULONGLONG * rct);

    BOOL    GetCPUUserRate(DWORD lProcessID, double & dCPUUserRate);

    BOOL    GetCPUUserRateEx(HANDLE hProccess, double & dCPUUserRate);

    int     GetIOBytesEx(HANDLE hProccess, ULONGLONG * read_bytes, ULONGLONG * write_bytes, ULONGLONG * wct, ULONGLONG * rct);

    BOOL    GetMemoryUsedEx(HANDLE hProccess, DWORD & dwPeakWorkingSetSize, DWORD & dwWorkingSetSize);

    BOOL    GetHandleCountEx(HANDLE hProccess, DWORD &dwHandles);


};
