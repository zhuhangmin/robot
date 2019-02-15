#pragma once

class CMainServer {
public:
    CMainServer();
    virtual ~CMainServer();

public:
    virtual BOOL Initialize();
    virtual void Shutdown();

    BOOL InitBase();
    BOOL StartServer();

private:
    virtual void TimerThreadProc();
    virtual void OnThreadTimer(std::time_t nCurrTime);
};