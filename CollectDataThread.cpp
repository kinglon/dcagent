// CollectDataThread.cpp: 实现文件
//

#include "pch.h"
#include "dcagent.h"
#include "CollectDataThread.h"
#include "croncpp.h"
#include "SettingManager.h"
#include "Utility/ImCharset.h"
#include "Utility/ImPath.h"
#include "PerfData.h"
#include <fstream>

const std::wstring SCRIPT_FOLDER_NAME = L"Script";

#define WM_SCRIPT_OUTPUT (WM_USER+100)
#define WM_SCHEDULE_POLICY_CHANGE (WM_USER+101)

CCollectDataThread* CCollectDataThread::m_pInstance = nullptr;

IMPLEMENT_DYNCREATE(CCollectDataThread, CWinThread)

CCollectDataThread::CCollectDataThread()
{
	m_pInstance = this;
}

CCollectDataThread::~CCollectDataThread()
{
	m_pInstance = nullptr;
}

UINT CCollectDataThread::NextElapse(const CSchedulePolicy& schedulePolicy)
{
    UINT uElapse = 0;
    try
    {
        auto cron = cron::make_cron(schedulePolicy.m_strCronTab);
        std::time_t now = std::time(0);
        auto it = m_mapGroupName2TriggerTime.find(schedulePolicy.m_strGroupName);
        if (it != m_mapGroupName2TriggerTime.end())
        {
            now = it->second;
        }
        std::time_t next = cron::cron_next(cron, now);
        uElapse = (UINT)(next - now);
        if (uElapse < 15)
        {
            // 避免频率太快
            uElapse += 15;
        }
        m_mapGroupName2TriggerTime[schedulePolicy.m_strGroupName] = next;
    }
    catch (cron::bad_cronexpr const&)
    {
        std::wstring strCronTab = CImCharset::UTF8ToUnicode(schedulePolicy.m_strCronTab.c_str(), schedulePolicy.m_strCronTab.length());
        std::wstring strGroupName = CImCharset::UTF8ToUnicode(schedulePolicy.m_strGroupName.c_str(), schedulePolicy.m_strGroupName.length());
        LOG_ERROR(L"the cron tabe (%s) of %s is wrong.", strCronTab.c_str(), strGroupName.c_str());        
    }

    return uElapse;
}

UINT_PTR CCollectDataThread::SetScheduleTimer(const CSchedulePolicy& schedulePolicy)
{
    if (!schedulePolicy.m_bEnable)
    {
        return 0;
    }

    UINT uElapse = NextElapse(schedulePolicy);
    if (uElapse == 0)
    {
        return 0;
    }

    std::wstring strGroupName = CImCharset::UTF8ToUnicode(schedulePolicy.m_strGroupName.c_str(), schedulePolicy.m_strGroupName.length());
    LOG_INFO(L"%s will trigger after %d seconds", strGroupName.c_str(), uElapse);
    UINT_PTR nTimerId = ::SetTimer(NULL, 0, uElapse * 1000, nullptr);
    if (nTimerId == 0)
    {
        LOG_ERROR(L"failed to set a timer, error is %d", GetLastError());
        return 0;
    }

    return nTimerId;
}

BOOL CCollectDataThread::InitInstance()
{	
    LOG_INFO(L"the thread of collecting data begins to run");

    AfxSocketInit();

    if (!m_perfSendSock.Create(0, SOCK_DGRAM))
    {
        LOG_ERROR(L"failed to create the sending socket, error is %d", m_perfSendSock.GetLastError());
        return FALSE;
    }

    const std::vector<CSchedulePolicy>& schedulePolicies = CSettingManager::GetInstance()->GetSchedulePolicies();
    for (const auto& schedulePolicy : schedulePolicies)
    {
        UINT_PTR nTimerId = SetScheduleTimer(schedulePolicy);
        if (nTimerId > 0)
        {
            m_mapTimerId2GroupName[nTimerId] = schedulePolicy.m_strGroupName;
        }
    }

	return TRUE;
}

void CCollectDataThread::SendData(const std::string& strData)
{
    std::string strIP = CSettingManager::GetInstance()->GetSendIpAddress();
    std::wstring strIPUnicode = CImCharset::AnsiToUnicode(strIP.c_str());
    UINT nPort = CSettingManager::GetInstance()->GetSendPort();
    if (m_perfSendSock.SendTo(strData.c_str(), strData.length(), nPort, strIPUnicode.c_str()) == SOCKET_ERROR)
    {
        LOG_ERROR(L"failed to send the performance data, error is %d", GetLastError());
    }
}

int CCollectDataThread::ExitInstance()
{	
	return CWinThread::ExitInstance();
}

BOOL CCollectDataThread::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_TIMER)
    {
        OnTimer((UINT_PTR)pMsg->wParam);
        return TRUE;
    }
    else if (pMsg->message == WM_SCRIPT_OUTPUT)
    {
        std::tuple<std::string, std::string>* lpParam = (std::tuple<std::string, std::string>*)(pMsg->lParam);
        const std::string& strGroupName = std::get<0>(*lpParam);
        const std::string& strOutput = std::get<1>(*lpParam);
        OnScriptOutput(strGroupName, strOutput);
        delete lpParam;
        lpParam = nullptr;
        return TRUE;
    }
    else if (pMsg->message == WM_SCHEDULE_POLICY_CHANGE)
    {
        CSchedulePolicy* lpSchedulePolicy = (CSchedulePolicy*)pMsg->lParam;
        OnSchedulePolicyChange(*lpSchedulePolicy);
        delete lpSchedulePolicy;
        return TRUE;
    }

    return CWinThread::PreTranslateMessage(pMsg);
}

void CCollectDataThread::OnTimer(UINT_PTR nTimerId)
{
    KillTimer(NULL, nTimerId);
    auto it = m_mapTimerId2GroupName.find(nTimerId);
    if (it == m_mapTimerId2GroupName.end())
    {
        return;
    }

    std::string strGroupName = it->second;
    m_mapTimerId2GroupName.erase(it);    

    const std::vector<CSchedulePolicy>& schedulePolicies = CSettingManager::GetInstance()->GetSchedulePolicies();
    for (const auto& schedulePolicy : schedulePolicies)
    {
        if (schedulePolicy.m_strGroupName == strGroupName)
        {
            UINT_PTR nTimerId = SetScheduleTimer(schedulePolicy);
            if (nTimerId > 0)
            {
                m_mapTimerId2GroupName[nTimerId] = schedulePolicy.m_strGroupName;
            }

            RunScript(strGroupName, schedulePolicy.m_strScriptName);

            break;
        }
    }
}

DWORD WINAPI RecvScriptOutputThreadProc(LPVOID lpParam)
{
    std::tuple<std::string, HANDLE>* pParamTuple = (std::tuple<std::string, HANDLE>*)lpParam;
    std::string strGroupName = std::get<0>(*pParamTuple);
    HANDLE hReadPipe = std::get<1>(*pParamTuple);
    delete pParamTuple;
    pParamTuple = nullptr;

    const int BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    DWORD bytesRead;
    std::string output;
    while (ReadFile(hReadPipe, buffer, BUFFER_SIZE - 1, &bytesRead, NULL) && bytesRead != 0)
    {
        buffer[bytesRead] = '\0';
        output += buffer;
    }
    CloseHandle(hReadPipe);

    CCollectDataThread::GetInstance()->RecvScriptOutput(strGroupName, output);

    return 0;
}

std::wstring CCollectDataThread::GetScriptFilePath(const std::string& strScriptFileName)
{
    static std::wstring strScriptPath;
    if (strScriptPath.empty())
    {
        strScriptPath = CImPath::GetSoftInstallPath() + SCRIPT_FOLDER_NAME + L'\\';
        if (!PathFileExists(strScriptPath.c_str()))
        {
            CreateDirectory(strScriptPath.c_str(), NULL);
        }
    }

    std::wstring strScriptFileUnicode = CImCharset::UTF8ToUnicode(strScriptFileName.c_str(), strScriptFileName.length());
    std::wstring strScriptFilePath = strScriptPath + strScriptFileUnicode;
    return strScriptFilePath;
}

void CCollectDataThread::RunScript(std::string strGroupName, std::string strScriptFileName)
{
    auto it = m_mapGroupName2IsRun.find(strGroupName);
    if (it != m_mapGroupName2IsRun.end() && it->second)
    {
        LOG_INFO(L"the %s is collecting.", CImCharset::AnsiToUnicode(strGroupName.c_str()).c_str());
        return;
    }
    
    std::wstring strScriptPath = GetScriptFilePath(strScriptFileName);
    LOG_DEBUG(L"run script : %s", strScriptPath.c_str());
    DWORD attributes = GetFileAttributes(strScriptPath.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        LOG_ERROR(L"the file (%s) does not exist.", strScriptPath.c_str());
        return;
    }

    wchar_t command[MAX_PATH];
    _snwprintf_s(command, MAX_PATH, L"cscript.exe \"%s\" ", strScriptPath.c_str());

    // Create anonymous pipe for capturing output
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
    {
        LOG_ERROR(L"failed to create a pipe, error is %d", GetLastError());
        return;
    }

    // Set STDOUT of the child process to the write end of the pipe
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;

    // Create the child process
    if (!CreateProcess(NULL, (LPWSTR)command, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
    {
        LOG_ERROR(L"failed to create a process, error is %d", GetLastError());
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return;
    }
    
    CloseHandle(hWritePipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // Start a thread to receive the script's output
    m_mapGroupName2IsRun[strGroupName] = true;
    std::tuple<std::string, HANDLE>* pThreadParam = new  std::tuple<std::string, HANDLE>();
    std::get<0>(*pThreadParam) = strGroupName;
    std::get<1>(*pThreadParam) = hReadPipe;
    HANDLE hThread = ::CreateThread(NULL, 0, RecvScriptOutputThreadProc, (LPVOID)pThreadParam, 0, NULL);
    if (hThread == NULL)
    {
        LOG_ERROR(L"failed to start the thread of receiving script's output, error is %d", GetLastError());
        m_mapGroupName2IsRun[strGroupName] = false;
        CloseHandle(hReadPipe);
        return;
    }
    CloseHandle(hThread);
}

void CCollectDataThread::RecvScriptOutput(const std::string& strGroupName, const std::string& strOutput)
{
    std::tuple<std::string, std::string>* lpParam = new std::tuple<std::string, std::string>();
    std::get<0>(*lpParam) = strGroupName;
    std::get<1>(*lpParam) = strOutput;
    this->PostThreadMessage(WM_SCRIPT_OUTPUT, 0, (LPARAM)lpParam);
}

void CCollectDataThread::OnScriptOutput(const std::string& strGroupName, const std::string& strOutput)
{
    m_mapGroupName2IsRun[strGroupName] = false;

    CPerfData perfData;
    perfData.m_strIndexName = strGroupName;
    perfData.m_strIpv4Info = CSettingManager::GetInstance()->GetLocalIpAddress();
    perfData.m_strData = strOutput;
    std::string perfJsonString = perfData.ToJsonString();
    SendData(perfJsonString);
}

void CCollectDataThread::UpdateSchedulePolicy(const CSchedulePolicy& schedulePolicy)
{
    CSchedulePolicy* lpSchedulePolicy = new CSchedulePolicy();
    *lpSchedulePolicy = schedulePolicy;
    this->PostThreadMessage(WM_SCHEDULE_POLICY_CHANGE, 0, (LPARAM)lpSchedulePolicy);
}

void CCollectDataThread::UpdateScriptFile(const CScriptFile& scriptFile)
{
    std::wstring strScriptFilePath = GetScriptFilePath(scriptFile.m_strScriptName);
    std::ofstream outputFile;
    outputFile.open(strScriptFilePath.c_str());
    if (outputFile.is_open())
    {
        outputFile << scriptFile.m_strScriptData.c_str();        
        outputFile.close();
        std::wstring strScriptNameUnicode = CImCharset::AnsiToUnicode(scriptFile.m_strScriptName.c_str());
        LOG_INFO(L"successfully update the script of %s", strScriptNameUnicode.c_str());
    }
    else
    {
        LOG_ERROR(L"failed to write the script file : %s", strScriptFilePath.c_str());
    }
}

void CCollectDataThread::OnSchedulePolicyChange(const CSchedulePolicy& schedulePolicy)
{
    CSettingManager::GetInstance()->UpdateSchedulePolicies(schedulePolicy);
    
    // 清除已经设置的定时器
    for (auto it = m_mapTimerId2GroupName.begin(); it != m_mapTimerId2GroupName.end(); it++)
    {
        if (it->second == schedulePolicy.m_strGroupName)
        {
            KillTimer(NULL, it->first);
            m_mapTimerId2GroupName.erase(it);
            break;
        }
    }
    m_mapGroupName2TriggerTime.erase(schedulePolicy.m_strGroupName);

    // 设置新的定时器
    UINT_PTR nTimerId = SetScheduleTimer(schedulePolicy);
    if (nTimerId > 0)
    {
        m_mapTimerId2GroupName[nTimerId] = schedulePolicy.m_strGroupName;
    }
}

BEGIN_MESSAGE_MAP(CCollectDataThread, CWinThread)
END_MESSAGE_MAP()
