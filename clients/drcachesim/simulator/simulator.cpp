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
#include "simulator_create.h"

#include <iostream>
#include <iterator>
#include <assert.h>
#include <limits.h>
#include <utility>
#include <cstdio>
#include "../common/memref.h"
#include "../common/options.h"
#include "../common/utils.h"
#include "droption.h"
#include "simulator.h"

simulator_t::simulator_t( simulator_knobs_t *knobs ) : knobs_( knobs )
{
    init_knobs( *knobs_ ); 
}



void
simulator_t::init_knobs( simulator_knobs_t &knobs )
{
    last_thread_    = 0;
    last_core_      = 0;
    cpu_counts_.resize( knobs_->num_cores, 0 );
    thread_counts_.resize(knobs_->num_cores, 0);
    thread_ever_counts_.resize(knobs_->num_cores, 0);
    //special case where marker is at PC zero or we want to record everything 
    record = ( knobs_->start_pc == 0 );

    if ( knobs_->warmup_refs > 0 && ( knobs_->warmup_fraction  > 0.0)) 
    {
        ERRMSG("Usage error: Either warmup_refs OR warmup_fraction can be set");
        success_ = false;
        return;
    }
}




bool
simulator_t::process_memref(const memref_t &memref)
{
    if ( memref.marker.type == TRACE_TYPE_MARKER &&
         memref.marker.marker_type == TRACE_MARKER_TYPE_CPU_ID && 
         knobs_->cpu_scheduling )
    {
        int cpu = (int)(intptr_t)memref.marker.marker_value;
        if( cpu < 0 )
        {
            return true;
        }
        int min_core;
        auto exists = cpu2core_.find(cpu);
        if (exists == cpu2core_.end()) {
            min_core = find_emptiest_core(cpu_counts_);
            ++cpu_counts_[min_core];
            cpu2core_[cpu] = min_core;
            if (knobs_->verbose >= 1) {
                std::cerr << "new cpu " << cpu << " => core " << min_core
                          << " (count=" << cpu_counts_[min_core] << ")" << std::endl;
            }
        } 
        else
        {
            min_core = exists->second;
        }
        auto prior = thread2core_.find(memref.marker.tid);
        if (prior != thread2core_.end())
        {
            --thread_counts_[prior->second];
        }
        thread2core_[memref.marker.tid] = min_core;
        ++thread_counts_[min_core];
        ++thread_ever_counts_[min_core];
    }
    return true;
}

int
simulator_t::find_emptiest_core(std::vector<int> &counts) const
{
    // We want to assign to the least-loaded core, measured just by
    // the number of cpus or threads already there.  We assume the #
    // of cores is small and that it's fastest to do a linear search
    // versus maintaining some kind of sorted data structure.
    int min_count = INT_MAX;
    int min_core = 0;
    for (unsigned int i = 0; i < knobs_->num_cores; i++) {
        if (counts[i] < min_count) {
            min_count = counts[i];
            min_core = i;
        }
    }
    return min_core;
}

int
simulator_t::core_and_va_activation_for_thread( const memref_t &ref )
{
    const auto tid = ref.data.tid;
    auto exists = thread2core_.find( tid );
    if (exists != thread2core_.end())
    {
        return( exists->second );
    }
    /** 
     * if thread already exists, VA space already activated, if we're 
     * here then it doesn't exist. 
     */
    auto proc_map_exists = os_process_map_.find( ref.data.pid );
    if( proc_map_exists != os_process_map_.end() )
    {
        /**
         * shouldn't be here, but if we are we recycled the PID, delete 
         * entry and add our new one. 
         */
        if( knobs_->verbose >= 1 )
        {
           std::cerr << 
            "Process in os_process_map exists, likely wrapped, deleting entry with id (" <<
                          ref.data.pid << ")\n";
        }
        /** delete ptr **/
        delete( (*proc_map_exists).second );
        /** erase entry **/
        os_process_map_.erase( proc_map_exists );
    }
    /**
     * add map/will block while it processes smaps file,
     * keeping as a sep. var for now to debug easier. 
     */
    auto *temp_map_ptr = new vm_memory_map( ref.data.pid );
    os_process_map_.emplace( 
        std::make_pair( 
            ref.data.pid, 
            temp_map_ptr
        )
    );
    

    // Either knob_cpu_scheduling_is off and we're ignoring cpu
    // markers, or there has not yet been a cpu marker for this
    // thread.  We fall back to scheduling threads directly to cores.
    int min_core = find_emptiest_core(thread_counts_);
    if( ! knobs_->cpu_scheduling && knobs_->verbose >= 1 ) 
    {
        std::cerr << "new thread " << tid << " => core " << min_core
                  << " (count=" << thread_counts_[min_core] << ")" << std::endl;
    } else if (knobs_->cpu_scheduling && knobs_->verbose >= 1) 
    {
        std::cerr << "missing cpu marker, so placing thread " << tid << " => core "
                  << min_core << " (count=" << thread_counts_[min_core] << ")"
                  << std::endl;
    }
    ++thread_counts_[       min_core];
    ++thread_ever_counts_[  min_core];
    thread2core_[tid] = min_core;
    return min_core;
}

void
simulator_t::handle_thread_exit(memref_tid_t tid)
{
    std::unordered_map< memref_tid_t, int >::iterator exists = thread2core_.find( tid );
    assert(exists != thread2core_.end() );
    assert(thread_counts_[exists->second] > 0);
    --thread_counts_[exists->second];
    if( knobs_->verbose >= 1 ) 
    {
        std::cerr << "thread " << tid << " exited from core " << exists->second
                  << " (count=" << thread_counts_[exists->second] << ")" << std::endl;
    }
    thread2core_.erase(tid);
}
