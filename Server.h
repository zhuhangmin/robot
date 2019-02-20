#pragma once

class CMainServer {
public:
    CMainServer();
    virtual ~CMainServer();

public:
    virtual BOOL Initialize();
    virtual void Shutdown();

protected:
    BOOL InitBase();

private:
    virtual void TimerThreadProc();
    virtual void OnThreadTimer(std::time_t nCurrTime);
};
