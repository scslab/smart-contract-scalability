#pragma once

namespace scs
{

// This function should be called by each thread that uses LFI
// Second call from a thread is a no-op
void signal_init();

}
