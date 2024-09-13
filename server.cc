#include <fmt/base.h>
#include <infiniband/verbs.h>
#include <jsonrpccpp/common/procedure.h>
#include <jsonrpccpp/server.h>
#include <jsonrpccpp/server/abstractserver.h>
#include <jsonrpccpp/server/connectors/tcpsocketserver.h>

#include <cstdint>
#include <cstdio>

#include "rdma.h"

class RdmaServer : public jsonrpc::AbstractServer<RdmaServer> {
    public:
	RdmaServer(jsonrpc::TcpSocketServer &server, const RdmaConfig &config)
		: jsonrpc::AbstractServer<RdmaServer>(server),
		  server_ctx_(config)
	{
		this->bindAndAddMethod(
			jsonrpc::Procedure("ExchangeQP",
					   jsonrpc::PARAMS_BY_NAME,
					   jsonrpc::JSON_STRING, nullptr),
			&RdmaServer::ExchangeQP);
	}
	void ExchangeQP(const Json::Value &request, Json::Value &response)
	{
		auto remote_qp_info = QPInfo::parseJson(request);
		fmt::println(
			"Server receive QPInfo:\n\tlid={}\n\tqp_num={}\n\tgid={}\n\tgid_index={}",
			remote_qp_info.lid, remote_qp_info.qp_num,
			GidToStr(remote_qp_info.gid), remote_qp_info.gid_index);
		//
		QPInfo local_qp_info = { .lid = server_ctx_.port_attr_.lid,
					 .qp_num = server_ctx_.qp_->qp_num };
		local_qp_info.gid_index = kGidIndex;
		ibv_query_gid(server_ctx_.ctx_, kIBPort, kGidIndex,
			      &local_qp_info.gid);

		RdmaModifyQp2Rts(server_ctx_.qp_, local_qp_info,
				 remote_qp_info);
		// send work request to recv queue
		for (int i = 0; i < kMsgNum; i++) {
			ibv_recv_wr *bad_recv_wr = nullptr;
			ibv_sge list = { .addr = reinterpret_cast<uint64_t>(
						 server_ctx_.buf_ +
						 i * kMsgSize),
					 .length = kMsgSize,
					 .lkey = server_ctx_.mr_->lkey };
			ibv_recv_wr recv_wr = { .wr_id = static_cast<uint32_t>(
							i),
						.sg_list = &list,
						.num_sge = 1 };
			ibv_post_recv(server_ctx_.qp_, &recv_wr, &bad_recv_wr);
		}
		response["lid"] = local_qp_info.lid;
		response["qp_num"] = local_qp_info.qp_num;
		response["gid_index"] = local_qp_info.gid_index;
		response["gid"] = GidToStr(local_qp_info.gid);
	}

	~RdmaServer() override
	{
		StopListening();
	}

	RdmaContext server_ctx_;
};

int main()
{
	auto config = InitConifg(RdmaType::kServer);

	fmt::println("TcpServer listen to ip {}, port {}", config.ip,
		     config.port);
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
					fmt::println(
						"server recv wr {} successfully",
						wc[i].wr_id);
				}
			} else {
				fmt::println(stderr,
					     "server failed to recv wr {}",
					     wc[i].wr_id);
			}
		}
	}
	return 0;
}