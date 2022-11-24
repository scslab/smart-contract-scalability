#include "rpc/rpc_address_db.h"

namespace scs {

RpcAddressDB::RpcAddressDB()
    : address_db()
    , mtx()
    , ps()
{
    std::thread th([this] {
        while (!start_shutdown) {
            ps.poll(500);
        }
        std::lock_guard lock(mtx);
        ps_is_shutdown = true;
        cv.notify_all();
    });

    th.detach();
}

void
RpcAddressDB::await_ps_shutdown()
{
    auto done_lambda = [this]() -> bool { return ps_is_shutdown; };

    std::unique_lock lock(mtx);
    if (!done_lambda()) {
        cv.wait(lock, done_lambda);
    }
}

RpcAddressDB::~RpcAddressDB()
{
    start_shutdown = true;
    await_ps_shutdown();
}

std::optional<xdr::rpc_sock>
RpcAddressDB::connect(Hash const& h)
{
    std::shared_lock lock(mtx);

    auto it = address_db.find(h);
    if (it == address_db.end()) {
        return {};
    }

    return std::make_optional<xdr::rpc_sock>(
        ps,
        xdr::tcp_connect(it->second.addr.c_str(), it->second.port.c_str())
            .release());
}

void
RpcAddressDB::add_mapping(Hash const& h, RpcAddress addr)
{
    std::lock_guard lock(mtx);

    address_db[h] = addr;
}

} // namespace scs
