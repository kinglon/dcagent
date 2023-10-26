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

private:
	CDcAgentSocket m_policySocket;	

	CDcAgentSocket m_scriptSocket;	

protected:
	DECLARE_MESSAGE_MAP()
};


