/**
 * Copyright 2023 Geoffrey Ramseyer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
