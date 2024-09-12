#include <fmt/base.h>
#include <infiniband/verbs.h>
#include <jsonrpccpp/common/procedure.h>
#include <jsonrpccpp/server.h>
#include <jsonrpccpp/server/abstractserver.h>
#include <jsonrpccpp/server/connectors/tcpsocketserver.h>

#include <cstdint>
#include <cstdio>

#include "rdma.h"

class RdmaServer : public jsonrpc::AbstractServer<RdmaServer>
{
public:
    RdmaServer(jsonrpc::TcpSocketServer &server, const RdmaConfig &config)
        : jsonrpc::AbstractServer<RdmaServer>(server), server_ctx_(config)
    {
        this->bindAndAddMethod(jsonrpc::Procedure("ExchangeQP", jsonrpc::PARAMS_BY_NAME, nullptr),
                               &RdmaServer::ExchangeQP);
    }
    void ExchangeQP(const Json::Value &request, Json::Value &response)
    {
        auto remote_qp_info = QPInfo::parseJson(request);
        RdmaModifyQp2Rts(server_ctx_.qp_, remote_qp_info.qp_num, remote_qp_info.lid);
        // send work request to recv queue
        for (int i = 0; i < kMsgNum; i++) {
            ibv_recv_wr *bad_recv_wr = nullptr;
            ibv_sge list = {.addr = reinterpret_cast<uint64_t>(server_ctx_.buf_ + i * kMsgSize),
                            .length = kMsgSize,
                            .lkey = server_ctx_.mr_->lkey};
            ibv_recv_wr recv_wr = {.wr_id = static_cast<uint32_t>(i), .sg_list = &list, .num_sge = 1};
            ibv_post_recv(server_ctx_.qp_, &recv_wr, &bad_recv_wr);
        }
        //
        QPInfo local_qp_info = {.lid = server_ctx_.port_attr_.lid, .qp_num = server_ctx_.qp_->qp_num};
        response["lid"] = local_qp_info.lid;
        response["qp_num"] = local_qp_info.qp_num;
    }

    RdmaContext server_ctx_;
};

int main()
{
    auto config = InitConifg(RdmaType::kServer);

    jsonrpc::TcpSocketServer tcp_server(config.ip, config.port);
    RdmaServer server(tcp_server, config);

    server.StartListening();
    fmt::println("server start listening");

    ibv_wc wc[kMaxPollSize];
    int recv_cnt = 0;
    while (recv_cnt < kMsgNum) {
        int n = ibv_poll_cq(server.server_ctx_.cq_, kMaxPollSize, wc);
        recv_cnt += n;

        for (int i = 0; i < n; i++) {
            if (wc[i].status == IBV_WC_SUCCESS) {
                if (wc[i].opcode == IBV_WC_RECV) {
                    fmt::println("server recv wr {} successfully", wc[i].wr_id);
                }
            } else {
                fmt::println(stderr, "server failed to recv wr {}", wc[i].wr_id);
            }
        }
    }
    return 0;
}