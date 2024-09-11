#include <json/value.h>

#include <cstdint>
#include <string>
// 常量
static const std::string kConfigPath = "config.json";

// 结构体
struct Config {
    uint16_t port{0};
    std::string device;
    std::string ip;
};

enum class RdmaType { kServer, kClient };

// 宏

// 函数
void ErrCheck(bool cond, const char*);
Config InitConifg(RdmaType type);
