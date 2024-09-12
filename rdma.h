#include <infiniband/verbs.h>
#include <json/value.h>

#include <cstdint>
#include <string>
// 常量
static const std::string kConfigPath = "config.json";
static constexpr uint32_t kMsgSize = 4 * 1024 * 1024;  // 4MB
static constexpr uint32_t kMsgNum = 64;
static constexpr uint32_t kBufSize = kMsgSize * kMsgNum;
static constexpr int kQpNum = 1;
static constexpr uint8_t kIBPort = 1;
static constexpr uint8_t kIBSL = 0;
static constexpr uint32_t kMaxPollSize = 16;
// 结构体
struct RdmaConfig {
    uint16_t port{0};
    std::string device;
    std::string ip;
};

enum class RdmaType { kServer, kClient };

struct QPInfo {
    uint16_t lid;
    uint32_t qp_num;
    [[nodiscard]] Json::Value toJson() const
    {
        Json::Value value;
        value["lid"] = lid;
        value["qp_num"] = qp_num;
        return value;
    }

    static QPInfo parseJson(const Json::Value& v)
    {
        return {.lid = static_cast<uint16_t>(v["lid"].asInt()), .qp_num = static_cast<uint32_t>(v["qp_num"].asInt())};
    }
};

class RdmaContext
{
public:
    // func
    explicit RdmaContext(const RdmaConfig& config);
    ~RdmaContext();
    // field
    ibv_device** dev_list_{nullptr};
    ibv_context* ctx_{nullptr};
    ibv_pd* pd_{nullptr};
    ibv_mr* mr_{nullptr};
    ibv_cq* cq_{nullptr};
    ibv_qp* qp_{nullptr};
    char* buf_{nullptr};
    ibv_device_attr dev_attr_;
    ibv_port_attr port_attr_;
};

// 宏

// 函数
void ErrCheck(bool cond, const char*);
RdmaConfig InitConifg(RdmaType type);
void RdmaModifyQp2Rts(ibv_qp* qp, uint32_t target_qp_num, uint16_t target_lid);
