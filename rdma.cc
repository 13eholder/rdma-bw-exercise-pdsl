#include "rdma.h"

#include <fmt/base.h>
#include <fmt/format.h>
#include <infiniband/verbs.h>
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

RdmaConfig InitConifg(RdmaType type)
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

    RdmaConfig conf;

    if (type == RdmaType::kClient) {
        auto client_conf = root["client"];
        ErrCheck(!client_conf.isMember("device") || !client_conf.isMember("port") || !client_conf.isMember("ip"),
                 "config.json.client must have filed 'device','port','ip' ");
        conf.device = client_conf["device"].asString();
        conf.ip = client_conf["ip"].asString();
        conf.port = client_conf["port"].asInt();
    } else {
        auto server_conf = root["server"];
        ErrCheck(!server_conf.isMember("device") || !server_conf.isMember("port"),
                 "config.json.server must have field 'device' and 'port' ");
        conf.device = server_conf["device"].asString();
        conf.port = server_conf["port"].asInt();
        conf.ip = "127.0.0.1";
    }
    return conf;
}

void RdmaModifyQp2Rts(ibv_qp* qp, uint32_t target_qp_num, uint16_t target_lid)
{
    int ret = 0;

    // change QP state to INIT
    {
        ibv_qp_attr qp_attr = {
            .qp_state = IBV_QPS_INIT,
            .qp_access_flags =
                IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_ATOMIC,
            .pkey_index = 0,
            .port_num = kIBPort,
        };

        ret = ibv_modify_qp(qp, &qp_attr, IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS);
        ErrCheck(ret != 0, "Failed to modify qp to INIT");
    }

    // change QP state to RTR
    {
        ibv_qp_attr qp_attr = {
            .qp_state = IBV_QPS_RTR,
            .path_mtu = IBV_MTU_4096,
            // receivce queue package serial num
            .rq_psn = 0,
            .dest_qp_num = target_qp_num,
            .ah_attr = {.dlid = target_lid, .sl = kIBSL, .src_path_bits = 0, .is_global = 0, .port_num = kIBPort},
            .max_dest_rd_atomic = 1,
            .min_rnr_timer = 12,
        };
        ret = ibv_modify_qp(qp, &qp_attr,
                            IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN | IBV_QP_RQ_PSN |
                                IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER);
        ErrCheck(ret != 0, "Failed to change qp to rtr.");
    }

    /* Change QP state to RTS */
    {
        struct ibv_qp_attr qp_attr = {
            .qp_state = IBV_QPS_RTS,
            .sq_psn = 0,
            .max_rd_atomic = 1,
            .timeout = 14,
            .retry_cnt = 7,
            .rnr_retry = 7,
        };

        ret = ibv_modify_qp(qp, &qp_attr,
                            IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN |
                                IBV_QP_MAX_QP_RD_ATOMIC);
        ErrCheck(ret == 0, "Failed to modify qp to RTS.");
    }
}

RdmaContext::RdmaContext(const RdmaConfig& config)
{
    // get IB device list
    dev_list_ = ibv_get_device_list(nullptr);
    ErrCheck(dev_list_ == nullptr, "Failed to get ib device list.");

    struct ibv_device** dev = dev_list_;
    while (*dev != nullptr && strcmp(ibv_get_device_name(*dev), config.device.c_str()) != 0) {
        dev++;
    }
    ErrCheck(*dev == nullptr, fmt::format("no such device named {}", config.device).c_str());
    // Open IB device
    ctx_ = ibv_open_device(*dev);
    ErrCheck(ctx_ == nullptr, "Failed to open device");
    // allocate protection domain
    pd_ = ibv_alloc_pd(ctx_);
    ErrCheck(pd_ == nullptr, "Failed to alloc protection domain");
    // query IB port attribute
    ErrCheck(ibv_query_port(ctx_, kIBPort, &port_attr_) != 0, "Failed to query port info");
    // alloc buf
    buf_ = static_cast<char*>(std::aligned_alloc(4096, kBufSize));
    ErrCheck(buf_ == nullptr, "Failed to alloc buf memory");
    // alloc memory region
    mr_ = ibv_reg_mr(pd_, static_cast<void*>(buf_), kBufSize,
                     IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
    ErrCheck(mr_ == nullptr, "Failed to register mr");
    // query IB device attr
    ErrCheck(ibv_query_device(ctx_, &dev_attr_) != 0, "Failed to query device");
    // create cq
    cq_ = ibv_create_cq(ctx_, dev_attr_.max_cqe, nullptr, nullptr, 0);
    ErrCheck(cq_ == nullptr, "Failed to create cq");
    // create qp
    ibv_qp_cap qp_cap = {
        .max_send_wr = static_cast<uint32_t>(dev_attr_.max_qp_wr),
        .max_recv_wr = static_cast<uint32_t>(dev_attr_.max_qp_wr),
        .max_send_sge = 1,
        .max_recv_sge = 1,
    };
    ibv_qp_init_attr qp_init_attr = {.send_cq = cq_, .recv_cq = cq_, .srq = nullptr, .cap = qp_cap};
    qp_ = ibv_create_qp(pd_, &qp_init_attr);
    ErrCheck(qp_ == nullptr, "Failed to create qp");
}

RdmaContext::~RdmaContext()
{
    ibv_destroy_qp(qp_);
    ibv_destroy_cq(cq_);
    ibv_dereg_mr(mr_);
    ibv_dealloc_pd(pd_);
    ibv_close_device(ctx_);
    ibv_free_device_list(dev_list_);
    free(buf_);
}