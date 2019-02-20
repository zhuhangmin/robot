#pragma once

class CMainServer {
public:
    CMainServer();
    ~CMainServer();

public:
    int Initialize();
    void Shutdown();

protected:
    int InitBase();


};
