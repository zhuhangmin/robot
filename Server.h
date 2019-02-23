#pragma once

class CMainServer {
public:
    CMainServer();
    ~CMainServer();

public:
    int Init();

    void Term();

protected:

    int InitLanuch();


};
