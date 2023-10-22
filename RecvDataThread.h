#pragma once

#include "DcAgentSocket.h"

class CRecvDataThread : public CWinThread
{
	DECLARE_DYNCREATE(CRecvDataThread)

public:
	CRecvDataThread();
	virtual ~CRecvDataThread();

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

public:
	void SetRecvPort(UINT nPort) { m_nPort = nPort;  }

private:
	CDcAgentSocket m_udpSocket;

	UINT m_nPort = 7081;

protected:
	DECLARE_MESSAGE_MAP()
};


