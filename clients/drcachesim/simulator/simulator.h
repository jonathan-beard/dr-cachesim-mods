/* **********************************************************
 * Copyright (c) 2015-2020 Google, Inc.  All rights reserved.
 * **********************************************************/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Google, Inc. nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL VMWARE, INC. OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

/* simulator: represent a simulator of a set of caching devices.
 */

#ifndef _SIMULATOR_H_
#define _SIMULATOR_H_ 1

#include <unordered_map>
#include <vector>
#include <atomic>
#include <map>
#include "simulator_create.h"
#include "caching_device_stats.h"
#include "caching_device.h"
#include "analysis_tool.h"
#include "memorymap.hpp"
#include "memref.h"


class simulator_t : public analysis_tool_t {
    using knob_t = simulator_knobs_t;
public:
   
    simulator_t() = default;
    simulator_t( simulator_knobs_t *knobs ); 

    virtual ~simulator_t()  = default;

    bool
    process_memref(const memref_t &memref) override;

protected:
    // Initialize knobs. Success or failure is indicated by setting/resetting
    // the success variable.
    virtual void init_knobs( simulator_knobs_t &knobs );

    int
    find_emptiest_core(std::vector<int> &counts) const;
    
    virtual int
    core_and_va_activation_for_thread( const memref_t &ref );
    
    virtual void
    handle_thread_exit(memref_tid_t tid);

    simulator_knobs_t   *knobs_ = nullptr; 

    /** FIXME - finish sdt implementation and set this to false **/
    atomic_bool_t   record = { false };

    memref_tid_t    last_thread_;
    int             last_core_      = 0;

    // For thread mapping to cores:
    std::unordered_map<int, int>             cpu2core_;
    std::unordered_map<memref_tid_t, int>    thread2core_;
    std::vector<int>                         cpu_counts_;
    std::vector<int>                         thread_counts_;
    std::vector<int>                         thread_ever_counts_;
    std::map< memref_pid_t, vm_memory_map* > os_process_map_;
};

#endif /* _SIMULATOR_H_ */
