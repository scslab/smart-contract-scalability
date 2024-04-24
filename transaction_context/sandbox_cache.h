#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "lficpp/lficpp.h"

namespace scs {

class SandboxCache
{
    std::vector<std::unique_ptr<LFIProc>> procs;
    size_t free_ptr = 0;
    LFIGlobalEngine& global_engine;
    void* ctxp;

  public:
    SandboxCache(LFIGlobalEngine& global_engine, void* ctxp)
        : procs()
        , free_ptr(0)
        , global_engine(global_engine)
        , ctxp(ctxp)
    {
    }

    LFIProc* get_new_proc()
    {
        if (free_ptr >= procs.size()) {
            std::unique_ptr<LFIProc> proc
                = std::make_unique<LFIProc>(ctxp, global_engine);
            if (!(*proc)) {
                return nullptr;
            }
            procs.emplace_back(std::move(proc));
        }
        auto* out = procs[free_ptr].get();
        free_ptr++;
        return out;
    }

    void reset() { free_ptr = 0; }
};

} // namespace scs
