#pragma once

// ��ȡ�����ļ�(ini)��·��
std::string GetConfigFilePath();

// ��REQUEST����pb�ṹ
int ParseFromRequest(const REQUEST &req, google::protobuf::Message &val);

// pb�ṹתΪ�ַ���
std::string GetStringFromPb(const google::protobuf::Message& oMessage);

// �ַ�����ʽ���� ע�⣺�ڲ����泤��Ϊ1024������ַ������ȳ���1024��������ʹ�ô˺���������
std::string string_format(const char * _format, ...);