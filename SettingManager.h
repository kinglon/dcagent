#pragma once

#include <vector>
#include "SchedulePolicy.h"

class CSettingManager
{
public:
	CSettingManager();

public:
	static CSettingManager* GetInstance();	

public:
	int GetLogLevel() { return m_nLogLevel; }
	
	UINT GetRecvPort() { return m_nRecvPort; }

	std::string GetLocalIpAddress() { return m_strLocalIPAddress; }

	// �������ݽ��յ�������ַ
	std::string GetSendIpAddress() { return m_strSendIPAddress; }

	// �������ݽ��յ������˿�
	UINT GetSendPort() { return m_nSendPort; }

	const std::vector<CSchedulePolicy>& GetSchedulePolicies() { return m_schedulePolicies; }

	void UpdateSchedulePolicies(const CSchedulePolicy& schedulePolicy);

private:
	void LoadBasicConfigure();

	void LoadSchedulePolicyConfigure();

	void SaveSchedulePolicyConfigure();

private:
	int m_nLogLevel = 2; // debug 1, info 2, error 3, ...

	UINT m_nRecvPort = 7083;

	std::string m_strLocalIPAddress = "127.0.0.1";

	std::string m_strSendIPAddress = "127.0.0.1";

	UINT m_nSendPort = 7084;

	std::vector<CSchedulePolicy> m_schedulePolicies;
};