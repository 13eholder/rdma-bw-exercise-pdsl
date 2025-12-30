#pragma once

#include <infiniband/verbs.h>

#include <cstdint>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include "tl/expected.hpp"

namespace rdma {

template <typename T>
using Result = tl::expected<T, std::error_code>;

inline tl::unexpected<std::error_code> make_unexpected() {
  return tl::make_unexpected(std::error_code{errno, std::generic_category()});
}

struct RDMAConfig {
  std::string device_name;
  std::string ip;
  uint16_t port;
  uint16_t gid_index;
};

struct RDMADeleter {
  void operator()(ibv_pd* pd) const { ibv_dealloc_pd(pd); }
  void operator()(ibv_mr* mr) const { ibv_dereg_mr(mr); }
  void operator()(ibv_cq* cq) const { ibv_destroy_cq(cq); }
  void operator()(ibv_qp* qp) const { ibv_destroy_qp(qp); }
};

class RDMADevice {
 public:
  RDMADevice() = default;
  ~RDMADevice();

  Result<void> init(const RDMAConfig& config);

  [[nodiscard]] ibv_context* context() const { return ctx_; }
  [[nodiscard]] const ibv_device_attr& device_attr() const {
    return device_attr_;
  }
  [[nodiscard]] const ibv_port_attr& port_attr() const { return port_attr_; }

 private:
  int num_devices_{0};
  int device_index_{-1};
  ibv_device** dev_list_{nullptr};
  ibv_context* ctx_{nullptr};
  ibv_device_attr device_attr_{};
  ibv_port_attr port_attr_{};
};

class RDMABuffer {
 public:
 private:
  std::unique_ptr<char[]> data_;
  std::unique_ptr<ibv_mr, RDMADeleter> mr_;
};

class RDMASocket {
 public:
  explicit RDMASocket(RDMADevice& device) : device_(device) {}
  Result<void> init();

 private:
  RDMADevice& device_;
  std::unique_ptr<ibv_pd, RDMADeleter> pd_;
  std::unique_ptr<ibv_cq, RDMADeleter> cq_;
  std::unique_ptr<ibv_qp, RDMADeleter> qp_;
  std::vector<RDMABuffer> buffers_;
};

struct RDMASocketInfo {
  uint8_t gid[16];       // GID
  uint16_t lid;          // LID of the IB port
  uint32_t buffer_size;  // buffer length
  uint32_t qp_num;       // QP number
};

}  // namespace rdma
