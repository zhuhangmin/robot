#pragma once

// 获取配置文件(ini)的路径
std::string GetConfigFilePath();

// 从REQUEST解析pb结构
int ParseFromRequest(const REQUEST &req, google::protobuf::Message &val);

// pb结构转为字符串
std::string GetStringFromPb(const google::protobuf::Message& oMessage);

// 字符串格式化。 注意：内部缓存长度为1024，如果字符串长度超过1024，则不允许使用此函数！！！
std::string string_format(const char * _format, ...);