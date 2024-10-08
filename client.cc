#include <fmt/base.h>
#include <fmt/format.h>
#include <infiniband/verbs.h>
#include <jsonrpccpp/client.h>
#include <jsonrpccpp/client/client.h>
#include <jsonrpccpp/client/connectors/tcpsocketclient.h>
#include <jsonrpccpp/common/exception.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "rdma.h"

int main()
{
	auto config = InitConifg(RdmaType::kClient);

	fmt::println("TcpClient connect to ip {}, port {}", config.ip,
		     config.port);

	jsonrpc::TcpSocketClient tcp_client(config.ip, config.port);
	jsonrpc::Client client(tcp_client);

	try {
		RdmaContext client_ctx(config);

		QPInfo remote_qp_info;
		QPInfo local_qp_info = { .lid = client_ctx.port_attr_.lid,
					 .qp_num = client_ctx.qp_->qp_num,
					 .gid_index = kDefaultGidIndex };
		ibv_query_gid(client_ctx.ctx_, kDefaultPort, kDefaultGidIndex,
			      &local_qp_info.gid);

		auto req = local_qp_info.toJson();
		fmt::println(
			"Client transport QPInfo:\n\tlid={}\n\tqp_num={}\n\tgid={}\n\tgid_index={}",
			local_qp_info.lid, local_qp_info.qp_num,
			GidToStr(local_qp_info.gid), local_qp_info.gid_index);
		auto resp = client.CallMethod("ExchangeQP", req);
		remote_qp_info = QPInfo::parseJson(resp);
		// modify qp to rts
		RdmaModifyQp2Rts(client_ctx.qp_, local_qp_info, remote_qp_info);
		// post  a list of work requests to a send queue
		for (int i = 0; i < kMsgNum; i++) {
			ibv_send_wr *bad_send_wr;
			ibv_sge list = { .addr = reinterpret_cast<uint64_t>(
						 client_ctx.buf_ +
						 i * kMsgSize),
					 .length = kMsgSize,
					 .lkey = client_ctx.mr_->lkey };
			ibv_send_wr send_wr = {
				.wr_id = static_cast<uint64_t>(i),
				.sg_list = &list,
				.num_sge = 1,
				.opcode = IBV_WR_SEND,
				.send_flags = IBV_SEND_SIGNALED
			};
			ErrCheck(ibv_post_send(client_ctx.qp_, &send_wr,
					       &bad_send_wr) != 0,
				 "Failed to post send");
		}
		//
		uint32_t send_num = kMsgNum;
		ibv_wc wc[kMaxPollSize];
		while (send_num > 0) {
			int n = ibv_poll_cq(client_ctx.cq_, kMaxPollSize, wc);
			send_num -= n;
			for (int i = 0; i < n; i++) {
				if (wc[i].status != IBV_WC_SUCCESS) {
					fmt::println(
						stderr,
						"send failed status: {},wr_id={}",
						ibv_wc_status_str(wc[i].status),
						wc[i].wr_id);
				}
			}
		}
	} catch (jsonrpc::JsonRpcException &e) {
		ErrCheck(true, e.what());
	}
	fmt::println("finished");
	return 0;
}