/* **********************************************************
 * Copyright (c) 2017-2018 Google, Inc.  All rights reserved.
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

/* cache simulator creation */

#ifndef _CACHE_SIMULATOR_CREATE_H_
#define _CACHE_SIMULATOR_CREATE_H_ 1

#include <cstdint>
#include <string>
#include <limits>
#include "simulator_create.h"
#include "analysis_tool.h"

/**
 * @file drmemtrace/cache_simulator_create.h
 * @brief DrMemtrace cache simulator creation.
 */

/**
 * The options for cache_simulator_create().
 * The options are currently documented in \ref sec_drcachesim_ops.
 */
// The options are currently documented in ../common/options.cpp.
struct cache_simulator_knobs_t : simulator_knobs_t /** base set of knobs **/
{
    
    cache_simulator_knobs_t() = default;
    

    cache_simulator_knobs_t( const std::uint64_t start_pc, 
                             const std::uint64_t stop_pc ) : simulator_knobs_t( start_pc, 
                                                                                stop_pc )
    {
    }


    unsigned int line_size          = 64;
    uint64_t L1I_size               = 32 * 1024U;
    uint64_t L1D_size               = 32 * 1024U;
    unsigned int L1I_assoc          = 8;
    unsigned int L1D_assoc          = 8;
    uint64_t LL_size                = 8 * 1024 * 1024;
    unsigned int LL_assoc           = 16;
    std::string LL_miss_file        = "";
    bool model_coherence            = false;
    std::string replace_policy      = "LRU";
    std::string data_prefetcher     = "nextline";
    bool op_cache_line_utilization  = false; 
};

/** Creates an instance of a cache simulator with a 2-level hierarchy. */
analysis_tool_t *
cache_simulator_create( cache_simulator_knobs_t *knobs );


/**
 * Creates an instance of a cache simulator using a cache hierarchy defined
 * in a configuration file.
 */
analysis_tool_t *
cache_simulator_create(const std::string &config_file);


//FIXME - move these at some point
/** Creates an instance of a cache miss analyzer. */
analysis_tool_t *
cache_miss_analyzer_create( cache_simulator_knobs_t *knobs,
                            unsigned int miss_count_threshold, 
                            double miss_frac_threshold,
                            double confidence_threshold);
#endif /* _CACHE_SIMULATOR_CREATE_H_ */
