#include "rdma.h"

#include <fmt/base.h>
#include <json/config.h>
#include <json/reader.h>
#include <json/value.h>
#include <sys/types.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>

void ErrCheck(bool cond, const char* msg)
{
    if (cond) {
        fmt::println(stderr, "ErrMsg : {}", msg);
        exit(EXIT_FAILURE);
    }
}

Config InitConifg(RdmaType type)
{
    std::ifstream ifs;
    ifs.open(kConfigPath);

    Json::CharReaderBuilder builder;
    Json::Value root;
    JSONCPP_STRING errs;

    if (!Json::parseFromStream(builder, ifs, &root, &errs)) {
        ErrCheck(true, errs.c_str());
    }

    ErrCheck(!root.isMember("server") || !root.isMember("client"), "config.json must have server and client field");

    Config conf;

    if (type == RdmaType::kClient) {
        auto client_conf = root["client"];
        ErrCheck(!client_conf.isMember("device"), "config.json.client must have device field");
        conf.device = client_conf["device"].asString();

    } else {
        auto server_conf = root["server"];
        ErrCheck(!server_conf.isMember("server"), "config.json.server must have device field");
        conf.device = server_conf["device"].asString();
    }
    return conf;
}