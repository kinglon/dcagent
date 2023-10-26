// RecvDataThread.cpp: 实现文件
//

#include "pch.h"
#include "dcagent.h"
#include "RecvDataThread.h"
#include "SettingManager.h"

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

    AfxSocketInit();

    // Create UDP socket
    UINT nPolicyPort = CSettingManager::GetInstance()->GetPolicyRecvPort();
    if (!m_policySocket.Create(nPolicyPort, SOCK_DGRAM))
    {
        LOG_ERROR(L"failed to create the policy socket (%d), error is %d", nPolicyPort, GetLastError());
        return FALSE;
    }

    UINT nScriptPort = CSettingManager::GetInstance()->GetScriptRecvPort();
    if (!m_scriptSocket.Create(nScriptPort, SOCK_DGRAM))
    {
        LOG_ERROR(L"failed to create the script socket (%d), error is %d", nScriptPort, GetLastError());
        return FALSE;
    }

    return TRUE;
}

int CRecvDataThread::ExitInstance()
{
    m_policySocket.Close();
	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CRecvDataThread, CWinThread)
END_MESSAGE_MAP()