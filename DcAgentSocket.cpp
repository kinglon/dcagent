#include "pch.h"
#include "DcAgentSocket.h"
#include "json/json.h"
#include "SchedulePolicy.h"
#include "CollectDataThread.h"
#include "Utility\ImCharset.h"

// 接收缓冲区的长度，大约1M
const int RECV_BUFFER_LENGTH = 1024000;

CDcAgentSocket::CDcAgentSocket()
{
    m_pRecvBuffer = new char[RECV_BUFFER_LENGTH];
}

CDcAgentSocket::~CDcAgentSocket()
{
    delete[] m_pRecvBuffer;
    m_pRecvBuffer = nullptr;
}

void CDcAgentSocket::OnReceive(int nErrorCode)
{
    if (nErrorCode != 0)
    {
        LOG_ERROR(L"there is an error %d when receiving data", nErrorCode);
        return;
    }

    // Receive data from a remote host
    CString strRemoteHost;
    UINT nRemotePort;
    int nBytesRead = ReceiveFrom(m_pRecvBuffer, RECV_BUFFER_LENGTH, strRemoteHost, nRemotePort);
    if (nBytesRead > 0)
    {
        LOG_INFO(L"receive data from %s:%d, length is %d", (LPCTSTR)strRemoteHost, nRemotePort, nBytesRead);
        LOG_DEBUG(L"the data is : %s", CImCharset::UTF8ToUnicode(m_pRecvBuffer, nBytesRead).c_str());
        ParseData(m_pRecvBuffer, nBytesRead);
    }
    else
    {
        LOG_ERROR(L"there is an error %d when calling ReceiveFrom", GetLastError());
    }
}

void CDcAgentSocket::ParseData(const char* pData, int nDataLength)
{
    Json::CharReaderBuilder readerBuilder;
    Json::Value root;
    std::string errors;

    std::istringstream jsonStream(std::string(pData, nDataLength));
    bool parsingSuccessful = Json::parseFromStream(readerBuilder, jsonStream, &root, &errors);
    if (parsingSuccessful)
    {
        if (root.isMember("group_name") && root.isMember("script_name") && root.isMember("is_true")\
            && root["is_true"].isBool() && root.isMember("crontab"))
        {
            CSchedulePolicy schedulePolicy;
            schedulePolicy.m_strGroupName = root["group_name"].asString().c_str();
            schedulePolicy.m_strScriptName = root["script_name"].asString().c_str();
            schedulePolicy.m_bEnable = root["is_true"].asBool();
            schedulePolicy.m_strCronTab = root["crontab"].asString().c_str();
            CCollectDataThread::GetInstance()->UpdateSchedulePolicy(schedulePolicy);
        }
        else if (root.isMember("script_name") && root.isMember("script_data"))
        {
            CScriptFile scriptFile;
            scriptFile.m_strScriptName = root["script_name"].asString().c_str();
            scriptFile.m_strScriptData = root["script_data"].asString().c_str();
            CCollectDataThread::GetInstance()->UpdateScriptFile(scriptFile);
        }
        else
        {
            LOG_ERROR(L"the data is not a schedule policy and script data");
        }
    }
    else
    {
        LOG_ERROR(L"failed to parse the data");
    }
}