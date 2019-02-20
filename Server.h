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


};
