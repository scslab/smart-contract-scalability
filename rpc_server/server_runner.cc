#include "rpc_server/server_runner.h"

namespace scs {

void
ServerRunner::run()
{
#if USE_RPC
    server->Wait();
#endif

    std::lock_guard lock(mtx);
    done = true;
    cv.notify_all();
}

ServerRunner::ServerRunner(std::unique_ptr<ExternalCall::Service> _impl,
                           std::string addr)
#if USE_RPC
    : impl(std::move(_impl))
    , server(nullptr)
#endif
{
#if USE_RPC
    grpc::ServerBuilder builder;
    builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
    builder.RegisterService(impl.get());
    server = builder.BuildAndStart();
#endif

    std::thread([this]() { run(); }).detach();
}

ServerRunner::~ServerRunner()
{
#if USE_RPC
    server->Shutdown();
#endif

    auto done_lambda = [this]() { return done; };

    std::unique_lock lock(mtx);
    if (done_lambda()) {
        return;
    }
    cv.wait(lock, done_lambda);
}

} // namespace scs