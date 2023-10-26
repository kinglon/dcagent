
// DcAgentDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "dcagent.h"
#include "DcAgentDlg.h"
#include "afxdialogex.h"
#include "RecvDataThread.h"
#include "CollectDataThread.h"
#include "SettingManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CDcAgentDlg 对话框



CDcAgentDlg::CDcAgentDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DCAGENT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDcAgentDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CDcAgentDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()


// CDcAgentDlg 消息处理程序

BOOL CDcAgentDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	SetAutoRun();

	AfxRegisterWndClass(0);

	CRecvDataThread* recvDataThread = new CRecvDataThread();	
	recvDataThread->m_bAutoDelete = true;
	recvDataThread->CreateThread();

	CCollectDataThread* collectDataThread = new CCollectDataThread();
	collectDataThread->m_bAutoDelete = true;
	collectDataThread->CreateThread();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CDcAgentDlg::SetAutoRun()
{
	bool bAutoRun = false;

	// Get the executable file path of the current program
	TCHAR szFilePath[MAX_PATH];
	GetModuleFileName(NULL, szFilePath, MAX_PATH);

	// Get the registry key for the current user's startup programs
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		// Check if the current program's executable file path is present in the startup programs
		TCHAR szValue[MAX_PATH];
		DWORD dwValueSize = MAX_PATH;
		DWORD dwType;
		if (RegQueryValueEx(hKey, _T("dcagent"), NULL, &dwType, (LPBYTE)szValue, &dwValueSize) == ERROR_SUCCESS)
		{
			// Compare the executable file path with the value in the registry
			if (_tcsicmp(szFilePath, szValue) == 0)
			{
				bAutoRun = true;				
			}			
		}

		RegCloseKey(hKey);
	}

	if (bAutoRun)
	{
		return;
	}

	// Get the registry key for the current user's startup programs
	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		// Add the current program's executable file path to the startup programs
		RegSetValueEx(hKey, _T("dcagent"), 0, REG_SZ, (LPBYTE)szFilePath, (_tcslen(szFilePath) + 1) * sizeof(TCHAR));
		RegCloseKey(hKey);
	}
}

void CDcAgentDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else if (nID == SC_MINIMIZE)
	{
		ShowWindow(SW_HIDE);
	}
	else if (nID == SC_CLOSE)
	{
		MessageBox(L"无法关闭", L"提示", MB_OK);
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CDcAgentDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CDcAgentDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

