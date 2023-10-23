#include "pch.h"
#include <Windows.h>
#include "LogBuffer.h"
#include <time.h>
#include <conio.h>
#include <errno.h>
#include <io.h>
#include <fcntl.h>
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include "ImCharset.h"

using namespace std;

#ifdef WIN32
const char PATH_SEPERATOR = L'\\';
#else
const char PATH_SEPERATOR = L'/';
#endif

LogBuffer::LogBuffer()
{
	ZeroMemory(m_szBuff, sizeof(m_szBuff));
	m_pBuffWritten = m_szBuff;
	m_pBuffEnd = m_pBuffWritten + ARRAYSIZE(m_szBuff) - 6;
}

LogBuffer::~LogBuffer()
{
}

void LogBuffer::LogFileName(const char* pFilePathName, unsigned int nLine)
{
	if (pFilePathName != nullptr){
		const char* pFileName = strrchr(const_cast<char*>(pFilePathName), PATH_SEPERATOR);
		if (pFileName) {
			pFileName++;
		}
		else{
			pFileName = pFilePathName;
		}

        int nWtiteDone = _snwprintf_s(m_pBuffWritten, (m_pBuffEnd - m_pBuffWritten)/sizeof(wchar_t), (m_pBuffEnd - m_pBuffWritten)/sizeof(wchar_t), L"[%S:%d] ", pFileName, nLine);
		if (nWtiteDone > 0){
			m_pBuffWritten += nWtiteDone;
		}
	}
}

void LogBuffer::LogTimeInfoAndLevel(ELogLevel logLevel)
{
    wstring strLevel = L"Info";
    if (logLevel >= ELogLevel::LOG_LEVEL_ERROR)
    {
        strLevel = L"Error";
    }
	else if (logLevel < ELogLevel::LOG_LEVEL_INFO)
	{
		strLevel = L"Debug";
	}

	SYSTEMTIME st;
	GetLocalTime(&st);
    int nWtiteDone = _snwprintf_s(m_pBuffWritten, (m_pBuffEnd - m_pBuffWritten)/sizeof(wchar_t), (m_pBuffEnd - m_pBuffWritten)/sizeof(wchar_t), L"<<%02d:%02d:%02d#%s>>", st.wHour, st.wMinute, st.wSecond, strLevel.c_str());
	if (nWtiteDone > 0)
    {
		m_pBuffWritten += nWtiteDone;
	}
}

void LogBuffer::LogTID()
{
	DWORD dwTID = GetCurrentThreadId();
    int nWtiteDone = _snwprintf_s(m_pBuffWritten, (m_pBuffEnd - m_pBuffWritten)/sizeof(wchar_t), (m_pBuffEnd - m_pBuffWritten)/sizeof(wchar_t), L"[tid:%05d] ", dwTID);
	if (nWtiteDone > 0){
		m_pBuffWritten += nWtiteDone;
	}
}

void LogBuffer::LogString(const wchar_t* pString)
{
	if (pString == nullptr){
		return;
	}

    int nWtiteDone = _snwprintf_s(m_pBuffWritten, (m_pBuffEnd - m_pBuffWritten)/sizeof(wchar_t), (m_pBuffEnd - m_pBuffWritten)/sizeof(wchar_t), L"[%s] ", pString);
	if (nWtiteDone > 0){
		m_pBuffWritten += nWtiteDone;
	}
}

void LogBuffer::LogFormat(const wchar_t* fmt, va_list ap)
{
    int nWtiteDone = _vsnwprintf_s(m_pBuffWritten, (m_pBuffEnd - m_pBuffWritten)/sizeof(wchar_t), (m_pBuffEnd - m_pBuffWritten)/sizeof(wchar_t), fmt, ap);
	if (nWtiteDone > 0){
		m_pBuffWritten += nWtiteDone;
	}
    else if (nWtiteDone == -1)  //Ìî³äÂú
    {
        m_pBuffWritten += m_pBuffEnd - m_pBuffWritten;
    }    
}

void LogBuffer::AppendLineBreak()
{
	if (m_pBuffEnd > m_pBuffWritten)
    {
        *(m_pBuffWritten++) = L'\n';
        *(m_pBuffWritten++) = L'\0';
	}
}

LogBuffer::operator const wchar_t*()
{
	return m_szBuff;
}

unsigned long LogBuffer::GetLogLength()
{
	return m_pBuffWritten - m_szBuff;
}
