#include <fmt/base.h>
#include <fmt/format.h>
#include <infiniband/verbs.h>

#include <cstring>

#include "rdma.h"

int main()
{
    auto config = InitConifg(RdmaType::kClient);

    struct ibv_device** dev_list = nullptr;
    // get IB device list
    dev_list = ibv_get_device_list(nullptr);
    ErrCheck(dev_list == nullptr, "Failed to get ib device list.");

    struct ibv_device** dev = dev_list;
    while (*dev != nullptr && strcmp(ibv_get_device_name(*dev), config.device.c_str()) != 0) {
        dev++;
    }
    ErrCheck(*dev == nullptr, fmt::format("no such device named {}", config.device).c_str());
    // Open IB device
    struct ibv_context* ctx = ibv_open_device(*dev);
    ErrCheck(ctx == nullptr, "Failed to open device");

    ibv_free_device_list(dev_list);
    return 0;
}