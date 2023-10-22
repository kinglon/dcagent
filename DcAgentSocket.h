#pragma once

#include <afxsock.h>

class CDcAgentSocket : public CAsyncSocket
{
public:
	CDcAgentSocket();
	~CDcAgentSocket();

protected:
	virtual void OnReceive(int nErrorCode) override;

private:
	void ParseData(const char* pData, int nDataLength);

private:
	char* m_pRecvBuffer = nullptr;
};