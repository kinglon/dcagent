// RecvDataThread.cpp: 实现文件
//

#include "pch.h"
#include "dcagent.h"
#include "RecvDataThread.h"

IMPLEMENT_DYNCREATE(CRecvDataThread, CWinThread)

CRecvDataThread::CRecvDataThread()
{
}

CRecvDataThread::~CRecvDataThread()
{
}

BOOL CRecvDataThread::InitInstance()
{
	LOG_INFO(L"the thread of receiving data begins to run");

    // Create UDP socket
    if (!m_udpSocket.Create(0, SOCK_DGRAM))
    {
        LOG_ERROR(L"failed to create the udp socket, error is %d", GetLastError());
        return FALSE;
    }

    // Bind the socket to a local port
    if (!m_udpSocket.Bind(m_nPort))
    {
        LOG_ERROR(L"failed to bind the port %d , error is %d", m_nPort, GetLastError());
        m_udpSocket.Close();
        return FALSE;
    }

    return TRUE;
}

int CRecvDataThread::ExitInstance()
{
    m_udpSocket.Close();
	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CRecvDataThread, CWinThread)
END_MESSAGE_MAP()