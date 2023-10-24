
// dcagent.cpp: 定义应用程序的类行为。
//

#include "pch.h"
#include "framework.h"
#include "dcagent.h"
#include "DcAgentDlg.h"
#include "Utility/LogUtil.h"
#include "Utility/DumpUtil.h"
#include "Utility/ImPath.h"
#include "SettingManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDcAgentApp

BEGIN_MESSAGE_MAP(CDcAgentApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CDcAgentApp 构造

CDcAgentApp::CDcAgentApp()
{
	// TODO: 在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}


// 唯一的 CDcAgentApp 对象

CDcAgentApp theApp;
CLogUtil* g_dllLog = nullptr;

BOOL CDcAgentApp::InitInstance()
{
	CWinApp::InitInstance();


	// 创建 shell 管理器，以防对话框包含
	// 任何 shell 树视图控件或 shell 列表视图控件。
	CShellManager *pShellManager = new CShellManager;

	// 激活“Windows Native”视觉管理器，以便在 MFC 控件中启用主题
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
	
	SetRegistryKey(_T("应用程序向导生成的本地应用程序"));	

	// 单实例
	const wchar_t* mutexName = L"{70BD9620-A373-4464-9BF9-CBF2D662359F}";
	HANDLE mutexHandle = CreateMutexW(nullptr, TRUE, mutexName);
	if (mutexHandle == nullptr || GetLastError() == ERROR_ALREADY_EXISTS)
	{
		AfxMessageBox(L"程序已经在运行");
		return FALSE;
	}

	g_dllLog = CLogUtil::GetLog(L"main");

	//初始化崩溃转储机制
	CDumpUtil::SetDumpFilePath(CImPath::GetDumpPath().c_str());
	CDumpUtil::Enable(true);

	int nLogLevel = CSettingManager::GetInstance()->GetLogLevel();
	g_dllLog->SetLogLevel((ELogLevel)nLogLevel);	

	// Initialize Winsock
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		LOG_ERROR(L"failed to initialize Winsock, error is %d", GetLastError());
		return FALSE;
	}

	CDcAgentDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: 在此放置处理何时用
		//  “确定”来关闭对话框的代码
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: 在此放置处理何时用
		//  “取消”来关闭对话框的代码
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "警告: 对话框创建失败，应用程序将意外终止。\n");
		TRACE(traceAppMsg, 0, "警告: 如果您在对话框上使用 MFC 控件，则无法 #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS。\n");
	}

	CloseHandle(mutexHandle);

	// Cleanup and shutdown Winsock
	WSACleanup();

	// 删除上面创建的 shell 管理器。
	if (pShellManager != nullptr)
	{
		delete pShellManager;
	}

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	// 由于对话框已关闭，所以将返回 FALSE 以便退出应用程序，
	//  而不是启动应用程序的消息泵。
	return FALSE;
}

