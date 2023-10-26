#include "pch.h"
#include "PerfData.h"
#include <json/json.h>

std::string CPerfData::ToJsonString()
{
	std::string strProKey = m_strIpv4Info + m_strIndexName + std::to_string(time(0));
	Json::Value root;
	root["index_name"] = m_strIndexName;
	root["ipv4info"] = m_strIpv4Info;
	root["pro_key"] = strProKey;
	root["data"] = m_strData;

	Json::StreamWriterBuilder writer;
	std::string jsonString = Json::writeString(writer, root);
	return jsonString;
}