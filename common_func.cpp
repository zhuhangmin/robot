#include "stdafx.h"
#include "common_func.h"


std::string GetConfigFilePath() {
    static std::string config_path;

    if (0 == config_path.length()) {
        char root_path[MAX_PATH] = {0};
        GetModuleFileName(NULL, root_path, MAX_PATH);

        config_path = root_path;
        config_path = config_path.substr(0, config_path.find_last_of(('.')));
        config_path += ".ini";
    }

    return config_path;
}

int ParseFromRequest(const REQUEST &req, google::protobuf::Message &val) {
    const auto repeated = req.head.nRepeated;
    const auto len = req.nDataLen - repeated * sizeof(CONTEXT_HEAD);
    if (len <= 0) {
        LOG_WARN("Invalid req. repeated  = [%d], datalen  = [%d]", repeated, req.nDataLen);
        return kCommFaild;
    }

    char *data = (char *) (req.pDataPtr) + repeated * sizeof(CONTEXT_HEAD);
    if (!val.ParseFromArray(data, len)) {
        LOG_WARN("ParseArray faild. req  [%d]");
        return kCommFaild;
    }

    return kCommSucc;
}

std::string GetStringFromPb(const google::protobuf::Message& val) {
    std::string msg;
    const auto is_true = google::protobuf::TextFormat::PrintToString(val, &msg);
    if (!is_true) {
        LOG_WARN("PrintToString return false.");
        return msg;
    }

    msg.erase(std::remove(msg.begin(), msg.end(), '\n'), msg.end());
    return msg;
}

std::string string_format(const char * _format, ...) {
#define DESCRIB_TEMP 512
    char msg[DESCRIB_TEMP] = {0};

    va_list marker = NULL;
    va_start(marker, _format);

    (void) vsprintf_s(msg, DESCRIB_TEMP - 1, _format, marker);

    va_end(marker);

    return msg;
}
