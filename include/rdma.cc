#include "rdma.h"

#include <infiniband/verbs.h>

#include <cerrno>
#include <cstdint>
#include <cstring>

#include "spdlog/spdlog.h"

namespace rdma {

Result<void> RDMADevice::init(const RDMAConfig& config) {
  dev_list_ = ibv_get_device_list(&num_devices_);
  if (dev_list_ == nullptr) {
    spdlog::error("ibv_get_device_list failed: {}", strerror(errno));
    return make_unexpected();
  }

  for (int i = 0; i < num_devices_; ++i) {
    if (config.device_name == ibv_get_device_name(dev_list_[i])) {
      device_index_ = i;
      break;
    }
  }
  if (device_index_ == -1) {
    spdlog::error("Device {} not found", config.device_name);
    return make_unexpected();
  }

  ctx_ = ibv_open_device(dev_list_[device_index_]);
  if (ctx_ == nullptr) {
    spdlog::error("ibv_open_device failed: {}", strerror(errno));
    return make_unexpected();
  }

  if (ibv_query_device(ctx_, &device_attr_)) {
    spdlog::error("ibv_query_device failed: {}", strerror(errno));
    return make_unexpected();
  }

  if (ibv_query_port(ctx_, 1, &port_attr_)) {
    spdlog::error("ibv_query_port failed: {}", strerror(errno));
    return make_unexpected();
  }

  return {};
}

RDMADevice::~RDMADevice() {
  if (ctx_) {
    ibv_close_device(ctx_);
  }

  if (dev_list_) {
    ibv_free_device_list(dev_list_);
  }
}

Result<void> RDMASocket::init() {
  ibv_pd* pd = ibv_alloc_pd(device_.context());
  if (pd == nullptr) {
    spdlog::error("ibv_alloc_pd failed: {}", strerror(errno));
    return make_unexpected();
  }

  ibv_cq* cq = ibv_create_cq(device_.context(), device_.device_attr().max_cqe,
                             nullptr, nullptr, 0);
  if (cq == nullptr) {
    spdlog::error("ibv_create_cq failed: {}", strerror(errno));
    return make_unexpected();
  }

  ibv_qp_init_attr qp_init_attr = {
      .send_cq = cq,
      .recv_cq = cq,
      .cap =
          {
              .max_send_wr =
                  static_cast<uint32_t>(device_.device_attr().max_qp_wr),
              .max_recv_wr =
                  static_cast<uint32_t>(device_.device_attr().max_qp_wr),
              .max_send_sge =
                  static_cast<uint32_t>(device_.device_attr().max_sge),
              .max_recv_sge =
                  static_cast<uint32_t>(device_.device_attr().max_sge),
          },
      .qp_type = IBV_QPT_RC,
  };
  ibv_qp* qp = ibv_create_qp(pd, &qp_init_attr);
  if (qp == nullptr) {
    spdlog::error("ibv_create_qp failed: {}", strerror(errno));
    return make_unexpected();
  }

  pd_.reset(pd);
  cq_.reset(cq);
  qp_.reset(qp);

  return {};
}

}  // namespace rdma