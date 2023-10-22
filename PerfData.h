#pragma once

#include <string>

class CPerfData
{
public:
	// 调度策略的group name
	std::string m_strIndexName;

	std::string m_strIpv4Info;	

	std::string m_strData;

public:
	std::string ToJsonString();
};
