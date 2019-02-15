#pragma once

class CMainServer
{
public:
	CMainServer();
	virtual ~CMainServer();

public:
	virtual BOOL Initialize();
	virtual void Shutdown();

	BOOL InitBase();
	BOOL StartServer();

protected:
	virtual void TimerThreadProc();

	virtual void OnThreadTimer(time_t nCurrTime);
};
