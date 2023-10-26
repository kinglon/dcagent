#include "pch.h"
#include "SettingManager.h"
#include <fstream>
#include <json/json.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include "Utility/ImPath.h"

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

#define SCHEDULE_POLICY_CONF_FILE_NAME L"schedule_policy.json"

CSettingManager::CSettingManager()
{
    LoadBasicConfigure();

    LoadSchedulePolicyConfigure();
}

CSettingManager* CSettingManager::GetInstance()
{
	static CSettingManager* pInstance = new CSettingManager();
	return pInstance;
}

void CSettingManager::LoadBasicConfigure()
{
    std::wstring strConfFilePath = CImPath::GetConfPath() + L"configs.json";    
    std::ifstream inputFile(strConfFilePath);
    if (!inputFile.is_open())
    {
        LOG_ERROR(L"failed to open the basic configure file : %s", strConfFilePath.c_str());
        return;
    }

    std::string jsonString;
    std::string line;
    while (std::getline(inputFile, line))
    {
        jsonString += line;
    }
    inputFile.close();

    Json::Value root;
    Json::CharReaderBuilder builder;
    Json::CharReader* reader = builder.newCharReader();
    std::string errors;
    bool parsingSuccessful = reader->parse(jsonString.c_str(), jsonString.c_str() + jsonString.size(), &root, &errors);
    delete reader;
    if (!parsingSuccessful)
    {
        LOG_ERROR(L"failed to parse the basic configure");
        return;
    }

    if (root.isMember("log_level"))
    {
        m_nLogLevel = root["log_level"].asInt();
    }

    std::vector<std::string> localIPs;
    if (root.isMember("local_ip") && root["local_ip"].isArray())
    {
        for (const Json::Value& value : root["local_ip"]) 
        {
            localIPs.push_back(value.asString());
        }
    }

    if (root.isMember("policy_receive_port"))
    {
        m_nPolicyRecvPort = root["policy_receive_port"].asInt();
    }

    if (root.isMember("script_receive_port"))
    {
        m_nScriptRecvPort = root["script_receive_port"].asInt();
    }

    if (root.isMember("data_server_ip"))
    {
        m_strSendIPAddress = root["data_server_ip"].asString();
    }

    if (root.isMember("data_server_port"))
    {
        m_nSendPort = root["data_server_port"].asInt();
    }

    bool bFound = false;
    std::vector<std::string> localNicIPs = GetLocalIPList();
    for (auto& ip : localNicIPs)
    {
        for (auto& ip2 : localIPs)
        {
            if (ip == ip2)
            {
                m_strLocalIPAddress = ip;
                bFound = true;
                break;
            }
        }
    }
    if (!bFound)
    {
        LOG_ERROR(L"the configured local ip is not existed");
    }
}

void CSettingManager::LoadSchedulePolicyConfigure()
{
    std::wstring strConfFilePath = CImPath::GetConfPath() + SCHEDULE_POLICY_CONF_FILE_NAME;
    std::ifstream inputFile(strConfFilePath);
    if (!inputFile.is_open())
    {
        LOG_ERROR(L"failed to open the configure file of schedule policy : %s", strConfFilePath.c_str());
        return;
    }

    std::string jsonString;
    std::string line;
    while (std::getline(inputFile, line))
    {
        jsonString += line;
    }
    inputFile.close();

    Json::Value root;
    Json::CharReaderBuilder builder;
    Json::CharReader* reader = builder.newCharReader();
    std::string errors;
    bool parsingSuccessful = reader->parse(jsonString.c_str(), jsonString.c_str() + jsonString.size(), &root, &errors);
    delete reader;
    if (!parsingSuccessful)
    {
        LOG_ERROR(L"failed to parse the basic configure");
        return;
    }    

    m_schedulePolicies.clear();
    if (root.isObject())
    {        
        for (const auto& key : root.getMemberNames())
        {
            const Json::Value& childValue = root[key];
            if (childValue.isMember("group_name") && childValue.isMember("script_name") &&
                childValue.isMember("is_true") && childValue.isMember("crontab"))
            {
                CSchedulePolicy schedulePolicy;
                schedulePolicy.m_strGroupName = childValue["group_name"].asString();
                schedulePolicy.m_strScriptName = childValue["script_name"].asString();
                schedulePolicy.m_strCronTab = childValue["crontab"].asString();
                schedulePolicy.m_bEnable = childValue["is_true"].asBool();
                m_schedulePolicies.push_back(schedulePolicy);
            }
        }
    }
}

void CSettingManager::SaveSchedulePolicyConfigure()
{
    Json::Value root = Json::objectValue;
    for (auto& item : m_schedulePolicies)
    {
        Json::Value childValue;
        childValue["group_name"] = item.m_strGroupName;
        childValue["script_name"] = item.m_strScriptName;
        childValue["crontab"] = item.m_strCronTab;
        childValue["is_true"] = item.m_bEnable;
        root[item.m_strGroupName] = childValue;
    }

    std::wstring strConfFilePath = CImPath::GetConfPath() + SCHEDULE_POLICY_CONF_FILE_NAME;
    std::ofstream outputFile(strConfFilePath);
    if (outputFile.is_open())
    {
        Json::StreamWriterBuilder writer;
        std::string jsonString = Json::writeString(writer, root);
        outputFile << jsonString;
        outputFile.close();
    }
    else
    {
        LOG_ERROR(L"failed to open the configure file of schedule policy : %s", strConfFilePath.c_str());
    }
}

void CSettingManager::UpdateSchedulePolicies(const CSchedulePolicy& schedulePolicy)
{
    bool bExist = false;
    for (auto& item : m_schedulePolicies)
    {
        if (item.m_strGroupName == schedulePolicy.m_strGroupName)
        {
            item = schedulePolicy;
            bExist = true;
            break;
        }
    }

    if (!bExist)
    {
        m_schedulePolicies.push_back(schedulePolicy);
    }

    SaveSchedulePolicyConfigure();
}

std::vector<std::string> CSettingManager::GetLocalIPList()
{
    std::vector<std::string> ipList;
    ULONG bufferSize = 0;
    DWORD result = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, nullptr, &bufferSize);
    if (result != ERROR_BUFFER_OVERFLOW)
    {
        LOG_ERROR(L"failed to call GetAdaptersAddresses, error is %d", result);
        return ipList;
    }

    IP_ADAPTER_ADDRESSES* adapterAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(new char[bufferSize]);
    result = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, adapterAddresses, &bufferSize);
    if (result != ERROR_SUCCESS)
    {
        LOG_ERROR(L"failed to call GetAdaptersAddresses, error is %d", result);
        delete[] adapterAddresses;
        return ipList;
    }

    IP_ADAPTER_ADDRESSES* adapter = adapterAddresses;
    while (adapter != nullptr)
    {
        IP_ADAPTER_UNICAST_ADDRESS* unicastAddress = adapter->FirstUnicastAddress;
        while (unicastAddress != nullptr)
        {
            char ipAddress[INET6_ADDRSTRLEN];
            DWORD ipAddressLength = sizeof(ipAddress);
            sockaddr* address = unicastAddress->Address.lpSockaddr;
            if (address->sa_family == AF_INET)
            {
                sockaddr_in* ipv4Address = reinterpret_cast<sockaddr_in*>(address);
                inet_ntop(AF_INET, &(ipv4Address->sin_addr), ipAddress, ipAddressLength);
                ipList.push_back(ipAddress);
            }   
            unicastAddress = unicastAddress->Next;
        }

        adapter = adapter->Next;
    }

    delete[] adapterAddresses;
    return ipList;
}