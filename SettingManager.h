#pragma once

#include <vector>
#include <json/json.h>
#include "SchedulePolicy.h"

class CSettingManager
{
public:
	CSettingManager();

public:
	static CSettingManager* GetInstance();	

public:
	int GetLogLevel() { return m_nLogLevel; }

	int GetLogBufferSize() { return m_nLogBufferSize; }
	
	UINT GetPolicyRecvPort() { return m_nPolicyRecvPort; }

	UINT GetScriptRecvPort() { return m_nScriptRecvPort; }

	std::string GetLocalIpAddress() { return m_strLocalIPAddress; }

	// 性能数据接收的主机地址
	std::string GetSendIpAddress() { return m_strSendIPAddress; }

	// 性能数据接收的主机端口
	UINT GetSendPort() { return m_nSendPort; }

	int GetSendBufferSize() { return m_nSendBufferSize; }

	int GetRecvBufferSize() { return m_nRecvBufferSize; }

	const std::vector<CSchedulePolicy>& GetSchedulePolicies() { return m_schedulePolicies; }

	void UpdateSchedulePolicies(const CSchedulePolicy& schedulePolicy);

private:
	void LoadBasicConfigure();

	void SaveBasicConfigure(const Json::Value& root);

	void LoadSchedulePolicyConfigure();

	void SaveSchedulePolicyConfigure();

	std::vector<std::string> GetLocalIPList();

private:
	int m_nLogLevel = 2; // debug 1, info 2, error 3, ...

	int m_nLogBufferSize = 4000;

	UINT m_nPolicyRecvPort = 6081;

	UINT m_nScriptRecvPort = 6082;

	std::string m_strLocalIPAddress = "127.0.0.1";

	std::string m_strSendIPAddress = "127.0.0.1";

	UINT m_nSendPort = 7084;

	std::vector<CSchedulePolicy> m_schedulePolicies;

	int m_nSendBufferSize = 8000;

	int m_nRecvBufferSize = 8000;
};
