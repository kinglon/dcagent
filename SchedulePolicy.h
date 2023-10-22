#pragma once

#include <string>

class CSchedulePolicy
{
public:
	std::string m_strGroupName;

	std::string m_strScriptName;

	bool m_bEnable = false;

	std::string m_strCronTab;
};

class CScriptFile
{
public:
	std::string m_strScriptName;

	std::string m_strScriptData;
};