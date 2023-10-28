#pragma once

#include <map>
#include <afxsock.h>
#include "SchedulePolicy.h"

class CCollectDataThread : public CWinThread
{
	DECLARE_DYNCREATE(CCollectDataThread)

public:
	CCollectDataThread();
	virtual ~CCollectDataThread();

public:
	static CCollectDataThread* GetInstance() { return m_pInstance; }

	void UpdateSchedulePolicy(const CSchedulePolicy& schedulePolicy);

	void UpdateScriptFile(const CScriptFile& scriptFile);

	void RecvScriptOutput(const std::string& strGroupName, const std::string& strOutput);

private:
	UINT NextElapse(const CSchedulePolicy& schedulePolicy);

	UINT_PTR SetScheduleTimer(const CSchedulePolicy& schedulePolicy);

	void OnTimer(UINT_PTR nTimerId);

	void RunScript(std::string strGroupName);

	void OnScriptOutput(const std::string& strGroupName, const std::string& strOutput);

	void SendData(const std::string& strData);

	void OnSchedulePolicyChange(const CSchedulePolicy& schedulePolicy);

	std::wstring GetScriptFilePath(const std::string& strScriptFileName);

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

private:
	static CCollectDataThread* m_pInstance;

	// 采集调度定时器，key是采集信息组的名字
	std::map<UINT_PTR, std::string> m_mapTimerId2GroupName;

	// 标识采集组是否正在采集
	std::map<std::string, bool> m_mapGroupName2IsRun;

	std::map<std::string, time_t> m_mapGroupName2TriggerTime;

	CAsyncSocket m_perfSendSock;

protected:
	DECLARE_MESSAGE_MAP()
};
