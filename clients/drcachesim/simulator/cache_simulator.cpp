/* **********************************************************
 * Copyright (c) 2015-2021 Google, Inc.  All rights reserved.
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

#include <iostream>
#include <iterator>
#include <string>
#include <assert.h>
#include <limits.h>
#include <stdint.h> /* for supporting 64-bit integers*/
#include "../common/memref.h"
#include "../common/options.h"
#include "../common/utils.h"
#include "../reader/config_reader.h"
#include "../reader/file_reader.h"
#include "../reader/ipc_reader.h"
#include "cache_stats.h"
#include "cache.h"
#include "cache_lru.h"
#include "cache_fifo.h"
#include "cache_simulator.h"
#include "droption.h"

#include "snoop_filter.h"
#include "cache_settings.h"

analysis_tool_t *
cache_simulator_create( cache_simulator_knobs_t *knobs )
{
    return new cache_simulator_t(knobs);
}

analysis_tool_t *
cache_simulator_create(const std::string &config_file)
{
    std::ifstream fin;
    fin.open(config_file);
    if (!fin.is_open()) {
        ERRMSG("Failed to open the config file '%s'\n", config_file.c_str());
        return nullptr;
    }
    analysis_tool_t *sim = new cache_simulator_t(&fin);
    fin.close();
    return sim;
}

cache_simulator_t::cache_simulator_t( cache_simulator_knobs_t *knobs )
    : simulator_t( knobs )
    , page_stats_impl()                   
{
    // XXX i#1703: get defaults from hardware being run on.
    const auto stat_dir_name = knobs_->stats_dir;
    page_stats_impl::init();
    // This configuration allows for one shared LLC only.
    auto *local_knobs = reinterpret_cast< knob_t* >( knobs_ );
    cache_t *llc = create_cache( local_knobs->replace_policy );
    if (llc == nullptr) 
    {
        error_string_ = "create_cache failed for the LLC";
        success_ = false;
        return;
    }

    std::string cache_name = "LL";
    all_caches_[cache_name] = llc;
    llcaches_[cache_name] = llc;

    if (local_knobs->data_prefetcher != PREFETCH_POLICY_NEXTLINE &&
        local_knobs->data_prefetcher != PREFETCH_POLICY_NONE) {
        // Unknown value.
        error_string_ = " unknown data_prefetcher: '" + local_knobs->data_prefetcher + "'";
        success_ = false;
        return;
    }

    bool warmup_enabled_ = ((local_knobs->warmup_refs > 0) || (local_knobs->warmup_fraction > 0.0));

    if (!llc->init( cache_settings_t( local_knobs->LL_assoc, 
                                      local_knobs->line_size, 
                                      local_knobs->LL_size, 
                                      local_knobs->op_cache_line_utilization, 
                                      &record ),
                    NULL,
                    new cache_stats_t(stat_dir_name, 
                                     cache_name,
                                     local_knobs->op_cache_line_utilization,
                                     (int)local_knobs->line_size, 
                                     local_knobs->LL_miss_file,
                                     warmup_enabled_ ) ) ) 
    {
        error_string_ =
            "Usage error: failed to initialize LL cache.  Ensure sizes and "
            "associativity are powers of 2, that the total size is a multiple "
            "of the line size, and that any miss file path is writable.";
        success_ = false;
        return;
    }
    
    llc->set_as_last_level();

    l1_icaches_ = new cache_t *[local_knobs->num_cores];
    l1_dcaches_ = new cache_t *[local_knobs->num_cores];
    unsigned int total_snooped_caches = 2 * local_knobs->num_cores;
    snooped_caches_ = new cache_t *[total_snooped_caches];
    if (local_knobs->model_coherence) {
        snoop_filter_ = new snoop_filter_t;
    }

    for (unsigned int i = 0; i < local_knobs->num_cores; i++) {
        l1_icaches_[i] = create_cache(local_knobs->replace_policy);
        if (l1_icaches_[i] == NULL) {
            error_string_ = "create_cache failed for an l1_icache";
            success_ = false;
            return;
        }
        snooped_caches_[2 * i] = l1_icaches_[i];
        l1_dcaches_[i] = create_cache(local_knobs->replace_policy);
        if (l1_dcaches_[i] == NULL) {
            error_string_ = "create_cache failed for an l1_dcache";
            success_ = false;
            return;
        }
        snooped_caches_[(2 * i) + 1] = l1_dcaches_[i];

        const auto cache_name_l1i = "L1_I_Cache_" + std::to_string(i);
        const auto cache_name_l1d = "L1_D_Cache_" + std::to_string(i);
       
        if (!l1_icaches_[i]->init(
                 cache_settings_t( local_knobs->L1I_assoc, 
                                   local_knobs->line_size, 
                                   local_knobs->L1I_size, 
                                   local_knobs->op_cache_line_utilization,
                                   &record,
                                   false /*inclusive*/, 
                                   local_knobs->model_coherence,
                                   2 * i /** id **/
                                   ),
                 llc,
                new cache_stats_t(  stat_dir_name,
                                    cache_name_l1i,
                                    local_knobs->op_cache_line_utilization,
                                    (int)local_knobs->line_size, 
                                    "", 
                                    warmup_enabled_,
                                    local_knobs->model_coherence),
                nullptr /*prefetcher*/, 
                snoop_filter_ ) ||
            !l1_dcaches_[i]->init(
                cache_settings_t( local_knobs->L1D_assoc, 
                                  local_knobs->line_size, 
                                  local_knobs->L1D_size, 
                                  local_knobs->op_cache_line_utilization,
                                  &record,
                                  false                         /** inclusive **/, 
                                  local_knobs->model_coherence  /** coherent? **/, 
                                  (2 * i) + 1                   /** id **/
                                  ),
                llc,
                new cache_stats_t(  stat_dir_name,
                                    cache_name_l1d,
                                    local_knobs->op_cache_line_utilization,
                                    (int)local_knobs->line_size, 
                                    "", 
                                    warmup_enabled_,
                                    local_knobs->model_coherence ),
                local_knobs->data_prefetcher == PREFETCH_POLICY_NEXTLINE ? new prefetcher_t((int)local_knobs->line_size)
                    : nullptr,
                snoop_filter_ ) ) 
       {
            error_string_ = "Usage error: failed to initialize L1 caches.  Ensure sizes "
                            "and associativity are powers of 2 "
                            "and that the total sizes are multiples of the line size.";
            success_ = false;
            return;
        }
        all_caches_[cache_name_l1i] = l1_icaches_[i];
        all_caches_[cache_name_l1d] = l1_dcaches_[i];
    }

    if (local_knobs->model_coherence &&
        !snoop_filter_->init(snooped_caches_, total_snooped_caches)) {
        ERRMSG("Usage error: failed to initialize snoop filter.\n");
        success_ = false;
        return;
    }
}

cache_simulator_t::cache_simulator_t(std::istream *config_file)
    : simulator_t()
{
#if 0
    std::map<std::string, cache_params_t> cache_params;
    config_reader_t config_reader;
    if (!config_reader.configure(config_file, knobs_, cache_params)) {
        error_string_ = "Usage error: Failed to read/parse configuration file";
        success_ = false;
        return;
    }

    init_knobs( knobs ); 

    if (knobs_.data_prefetcher != PREFETCH_POLICY_NEXTLINE &&
        knobs_.data_prefetcher != PREFETCH_POLICY_NONE) {
        // Unknown prefetcher type.
        success_ = false;
        return;
    }
    //set name from knobs for below
    const auto stat_dir_name = knobs_.stats_dir;

    bool warmup_enabled_ = ((knobs_.warmup_refs > 0) || (knobs_.warmup_fraction > 0.0));
        
    l1_icaches_ = new cache_t *[knobs_.num_cores];
    l1_dcaches_ = new cache_t *[knobs_.num_cores];

    // Create all the caches in the hierarchy.
    for (const auto &cache_params_it : cache_params) {
        std::string cache_name = cache_params_it.first;
        const auto &cache_config = cache_params_it.second;

        cache_t *cache = create_cache(cache_config.replace_policy);
        if (cache == NULL) {
            success_ = false;
            return;
        }

        all_caches_[cache_name] = cache;
    }

    int num_LL = 0;
    unsigned int total_snooped_caches = 0;
    std::string lowest_shared_cache = "";
    if (knobs_.model_coherence) {
        snoop_filter_ = new snoop_filter_t;
        std::string LL_name;
        /* This block determines where in the cache hierarchy to place the snoop filter.
         * If there is more than one LLC, the snoop filter is above those.
         */
        for (const auto &cache_it : all_caches_) {
            std::string cache_name = cache_it.first;
            const auto &cache_config = cache_params.find(cache_name)->second;
            if (cache_config.parent == CACHE_PARENT_MEMORY) {
                num_LL++;
                LL_name = cache_config.name;
            }
        }
        if (num_LL == 1) {
            /* There is one LLC, so we find highest cache with
             * multiple children to place the snoop filter.
             * Fully shared caches are marked as non-coherent.
             */
            cache_params_t current_cache = cache_params.find(LL_name)->second;
            non_coherent_caches_[LL_name] = all_caches_[LL_name];
            while (current_cache.children.size() == 1) {
                std::string child_name = current_cache.children[0];
                non_coherent_caches_[child_name] = all_caches_[child_name];
                current_cache = cache_params.find(child_name)->second;
            }
            if (current_cache.children.size() > 0) {
                lowest_shared_cache = current_cache.name;
                total_snooped_caches = (unsigned int)current_cache.children.size();
            }
        } else {
            total_snooped_caches = num_LL;
        }
        snooped_caches_ = new cache_t *[total_snooped_caches];
    }

    // Initialize all the caches in the hierarchy and identify both
    // the L1 caches and LLC(s).
    int snoop_id = 0;
    for (const auto &cache_it : all_caches_) {
        std::string cache_name = cache_it.first;
        cache_t *cache = cache_it.second;

        // Locate the cache's configuration.
        const auto &cache_config_it = cache_params.find(cache_name);
        if (cache_config_it == cache_params.end()) {
            error_string_ =
                "Error locating the configuration of the cache: " + cache_name;
            success_ = false;
            return;
        }
        auto &cache_config = cache_config_it->second;

        // Locate the cache's parent_.
        cache_t *parent_ = NULL;
        if (cache_config.parent != CACHE_PARENT_MEMORY) {
            const auto &parent_it = all_caches_.find(cache_config.parent);
            if (parent_it == all_caches_.end()) {
                error_string_ = "Error locating the configuration of the parent cache: " +
                    cache_config.parent;
                success_ = false;
                return;
            }
            parent_ = parent_it->second;
        }

        // Locate the cache's children.
        std::vector<caching_device_t *> children;
        children.clear();
        for (std::string &child_name : cache_config.children) {
            const auto &child_it = all_caches_.find(child_name);
            if (child_it == all_caches_.end()) {
                error_string_ =
                    "Error locating the configuration of the cache: " + child_name;
                success_ = false;
                return;
            }
            children.push_back(child_it->second);
        }

        // Determine if this cache should be connected to the snoop filter.
        bool LL_snooped = num_LL > 1 && cache_config.parent == CACHE_PARENT_MEMORY;
        bool mid_snooped = total_snooped_caches > 1 &&
            cache_config.parent.compare(lowest_shared_cache) == 0;

        bool is_snooped = knobs_.model_coherence && (LL_snooped || mid_snooped);

        // If cache is below a snoop filter, it should be marked as coherent.
        bool is_coherent_ = knobs_.model_coherence &&
            (non_coherent_caches_.find(cache_name) == non_coherent_caches_.end());

        if (!cache->init(   
                    cache_settings_t( cache_config.assoc, 
                                      knobs_.line_size,
                                      cache_config.size, 
                                      knobs_.op_cache_line_utilization,
                                      cache_config.inclusive, 
                                      is_coherent_, 
                                      is_snooped ? snoop_id : -1
                                    ),
                parent_,
                new cache_stats_t( stat_dir_name,
                                   cache_name, 
                                   knobs_.op_cache_line_utilization,
                                   (int)knobs_.line_size, 
                                   cache_config.miss_file,
                                   warmup_enabled_, 
                                   is_coherent_),
                cache_config.prefetcher == PREFETCH_POLICY_NEXTLINE
                    ? new prefetcher_t((int)knobs_.line_size)
                    : nullptr,
                ,
                is_snooped ? snoop_filter_ : nullptr, 
                children) ) 
        {
            error_string_ = "Usage error: failed to initialize the cache " + cache_name;
            success_ = false;
            return;
        }

        // Next snooped cache should have a different ID.
        if (is_snooped) {
            snooped_caches_[snoop_id] = cache;
            snoop_id++;
        }

        bool is_l1_or_llc = false;

        // Assign the pointers to the L1 instruction and data caches.
        if (cache_config.core >= 0 && cache_config.core < (int)knobs_.num_cores) {
            is_l1_or_llc = true;
            if (cache_config.type == CACHE_TYPE_INSTRUCTION ||
                cache_config.type == CACHE_TYPE_UNIFIED) {
                l1_icaches_[cache_config.core] = cache;
            }
            if (cache_config.type == CACHE_TYPE_DATA ||
                cache_config.type == CACHE_TYPE_UNIFIED) {
                l1_dcaches_[cache_config.core] = cache;
            }
        }

        // Assign the pointer(s) to the LLC(s).
        if (cache_config.parent == CACHE_PARENT_MEMORY) {
            is_l1_or_llc = true;
            llcaches_[cache_name] = cache;
        }

        // Keep track of non-L1 and non-LLC caches.
        if (!is_l1_or_llc) {
            other_caches_[cache_name] = cache;
        }
    }
    if (knobs_.model_coherence && !snoop_filter_->init(snooped_caches_, snoop_id)) {
        ERRMSG("Usage error: failed to initialize snoop filter.\n");
        success_ = false;
        return;
    }
    // For larger hierarchies, especially with coherence, using hashtables
    // for faster lookups provides performance wins as high as 15%.
    // However, hashtables can slow down smaller hierarchies, so we only
    // enable if we anticipate a win.
    if (other_caches_.size() > 0 && (knobs_.model_coherence || knobs_.num_cores >= 32)) {
        for (auto &cache : all_caches_) {
            cache.second->set_hashtable_use(true);
        }
    }
#endif    
}

cache_simulator_t::~cache_simulator_t()
{
    
    auto *local_knobs = reinterpret_cast< knob_t* >( knobs_ );
    const auto stats_dir = local_knobs->stats_dir;
    //open streams, write stats for unfiltered data
    //write 4KiB
    std::ofstream stream_4( stats_dir + "/page_usage_unfiltered_4KiB.dat" );
    //write 64KiB
    std::ofstream stream_64( stats_dir + "/page_usage_unfiltered_64KiB.dat" );
    //write 1MiB
    std::ofstream stream_1M( stats_dir + "/page_usage_unfiltered_1MiB.dat" );
    page_stats_impl::write(  stream_4,
                             stream_64,
                             stream_1M );
    //these close stream so ignore that little detail, stream is unusable after call

    for (auto &caches_it : all_caches_) {
        cache_t *cache = caches_it.second;
        delete cache->get_stats();
        delete cache->get_prefetcher();
        delete cache;
    }

    if (l1_icaches_ != NULL) {
        delete[] l1_icaches_;
    }
    if (l1_dcaches_ != NULL) {
        delete[] l1_dcaches_;
    }
    if (snooped_caches_ != NULL) {
        delete[] snooped_caches_;
    }
    if (snoop_filter_ != NULL) {
        delete snoop_filter_;
    }
    page_stats_impl::destroy();
}

uint64_t
cache_simulator_t::remaining_sim_refs() const
{
    auto *local_knobs = reinterpret_cast< knob_t* >( knobs_ );
    return( local_knobs->sim_refs );
}

bool
cache_simulator_t::process_memref(const memref_t &memref)
{
    auto *local_knobs = reinterpret_cast< knob_t* >( knobs_ );
    if (local_knobs->skip_refs > 0) 
    {
        local_knobs->skip_refs--;
        return true;
    }
    // If no warmup is specified and we have simulated sim_refs then
    // we are done.
    if ((local_knobs->warmup_refs == 0 && local_knobs->warmup_fraction == 0.0) &&
        local_knobs->sim_refs == 0)
        return true;

    // The references after warmup and simulated ones are dropped.
    if (check_warmed_up() && local_knobs->sim_refs == 0)
        return true;

    // Both warmup and simulated references are simulated.

    if (!simulator_t::process_memref(memref))
        return false;

    if (memref.marker.type == TRACE_TYPE_MARKER) {
        // We ignore markers before we ask core_for_thread, to avoid asking
        // too early on a timestamp marker.
        if (local_knobs->verbose >= 3) {
            std::cerr << "::" << memref.data.pid << "." << memref.data.tid << ":: "
                      << "marker type " << memref.marker.marker_type << " value "
                      << memref.marker.marker_value << "\n";
        }
        return true;
    }

    // We use a static scheduling of threads to cores, as it is
    // not practical to measure which core each thread actually
    // ran on for each memref.
    int core;
    if (memref.data.tid == last_thread_)
    {
        core = last_core_;
    }
    else 
    {
        core = core_and_va_activation_for_thread( memref );
        last_thread_ = memref.data.tid;
        last_core_ = core;
        /** 
         * this should only be called once given right now we have only one attach
         * point, let's go ahead and update the knobs start address.
         */
        local_knobs->start_pc = 
            os_process_map_[ memref.data.pid ]->add_os_vm_offset_to_pc( 
                local_knobs->start_pc 
            );
        
        local_knobs->stop_pc = 
            os_process_map_[ memref.data.pid ]->add_os_vm_offset_to_pc( 
                local_knobs->stop_pc 
            );

        std::fprintf( stderr, "start pc: 0x%zx\n", local_knobs->start_pc );
        std::fprintf( stderr, "stop pc: 0x%zx\n", local_knobs->stop_pc );
    }

    if( record )
    {
        page_stats_impl::update( memref );
    }
    if( type_is_instr(memref.instr.type) || memref.instr.type == TRACE_TYPE_PREFETCH_INSTR ) 
    {
        if(! os_process_map_[ memref.instr.pid ]->is_address_in_code( memref.instr.addr ) )
        {
            std::fprintf( 
                         stderr, 
                        "0x%" PRIxPTR "\n", memref.instr.addr );
        }
        if (local_knobs->verbose >= 3) {
            std::cerr << "::" << memref.data.pid << "." << memref.data.tid << ":: "
                      << " @" << (void *)memref.instr.addr << " instr x"
                      << memref.instr.size << "\n";
        }
        if( memref.instr.addr == local_knobs->start_pc )
        {
            if( local_knobs->verbose >= 1 )
            {
                std::cerr << "starting dr_cachesim recording @" << (void *)memref.instr.addr << std::endl;
            }
            record = true;
        }
        if( memref.instr.addr == local_knobs->stop_pc )
        {
            if( local_knobs->verbose >= 1 )
            {
                std::cerr << "stoping dr_cachesim recording @" << (void *)memref.instr.addr << std::endl;
            }
            record = false;
        }

        l1_icaches_[core]->request(memref);
    } 
    else if (memref.data.type == TRACE_TYPE_READ ||
               memref.data.type == TRACE_TYPE_WRITE ||
               // We may potentially handle prefetches differently.
               // TRACE_TYPE_PREFETCH_INSTR is handled above.
               type_is_prefetch(memref.data.type)) {
        if (local_knobs->verbose >= 3) {
            std::cerr << "::" << memref.data.pid << "." << memref.data.tid << ":: "
                      << " @" << (void *)memref.data.pc << " "
                      << trace_type_names[memref.data.type] << " "
                      << (void *)memref.data.addr << " x" << memref.data.size << "\n";
        }
        l1_dcaches_[core]->request(memref);
    } else if (memref.flush.type == TRACE_TYPE_INSTR_FLUSH) {
        if (local_knobs->verbose >= 3) {
            std::cerr << "::" << memref.data.pid << "." << memref.data.tid << ":: "
                      << " @" << (void *)memref.data.pc << " iflush "
                      << (void *)memref.data.addr << " x" << memref.data.size << "\n";
        }
        l1_icaches_[core]->flush(memref);
    } else if (memref.flush.type == TRACE_TYPE_DATA_FLUSH) {
        if (local_knobs->verbose >= 3) {
            std::cerr << "::" << memref.data.pid << "." << memref.data.tid << ":: "
                      << " @" << (void *)memref.data.pc << " dflush "
                      << (void *)memref.data.addr << " x" << memref.data.size << "\n";
        }
        l1_dcaches_[core]->flush(memref);
    } else if (memref.exit.type == TRACE_TYPE_THREAD_EXIT) {
        handle_thread_exit(memref.exit.tid);
        last_thread_ = 0;
    } else if (memref.marker.type == TRACE_TYPE_INSTR_NO_FETCH) {
        // Just ignore.
        if (local_knobs->verbose >= 3) {
            std::cerr << "::" << memref.data.pid << "." << memref.data.tid << ":: "
                      << " @" << (void *)memref.instr.addr << " non-fetched instr x"
                      << memref.instr.size << "\n";
        }
    } else {
        error_string_ = "Unhandled memref type " + std::to_string(memref.data.type);
        return false;
    }

    // reset cache stats when warming up is completed
    if (!is_warmed_up_ && check_warmed_up()) {
        for (auto &cache_it : all_caches_) {
            cache_t *cache = cache_it.second;
            cache->get_stats()->reset();
        }
        if (local_knobs->verbose >= 1) {
            std::cerr << "Cache simulation warmed up\n";
        }
    } else {
        local_knobs->sim_refs--;
    }

    return true;
}

// Return true if the number of warmup references have been executed or if
// specified fraction of the llcaches_ has been loaded. Also return true if the
// cache has already been warmed up. When there are multiple last level caches
// this function only returns true when all of them have been warmed up.
bool
cache_simulator_t::check_warmed_up()
{
    auto *local_knobs = reinterpret_cast< knob_t* >( knobs_ );
    // If the cache has already been warmed up return true
    if (is_warmed_up_)
    {
        return true;
    }
    // If the warmup_fraction option is set then check if the last level has
    // loaded enough data to be warmed up.
    if (local_knobs->warmup_fraction > 0.0) {
        is_warmed_up_ = true;
        for (auto &cache : llcaches_) {
            if (cache.second->get_loaded_fraction() < local_knobs->warmup_fraction) {
                is_warmed_up_ = false;
                break;
            }
        }

        if (is_warmed_up_) {
            return true;
        }
    }

    // If warmup_refs is set then decrement and indicate warmup done when
    // counter hits zero.
    if (local_knobs->warmup_refs > 0) {
        local_knobs->warmup_refs--;
        if (local_knobs->warmup_refs == 0) {
            is_warmed_up_ = true;
            return true;
        }
    }

    // If we reach here then warmup is not done.
    return false;
}

bool
cache_simulator_t::print_results()
{
    std::cerr << "Cache simulation results:\n";
    // Print core and associated L1 cache stats first.
    for (unsigned int i = 0; i < knobs_->num_cores; i++) 
    {
        if (thread_ever_counts_[i] > 0) 
        {
            if (l1_icaches_[i] != l1_dcaches_[i]) 
            {
                l1_icaches_[i]->get_stats()->print_stats("");
                l1_dcaches_[i]->get_stats()->print_stats("");
            } 
            else 
            {
                l1_icaches_[i]->get_stats()->print_stats("");
            }
        }
    }

    // Print non-L1, non-LLC cache stats.
    for (auto &caches_it : other_caches_) {
        caches_it.second->get_stats()->print_stats("");
    }

    auto *local_knobs = reinterpret_cast< knob_t* >( knobs_ );

    // Print LLC stats.
    for (auto &caches_it : llcaches_) {
        caches_it.second->get_stats()->print_stats("");
    }

    if( local_knobs->model_coherence ) 
    {
        snoop_filter_->print_stats();
    }
    
    /**
     * print overall configuration as well here 
     */
    const auto stats_dir = local_knobs->stats_dir;
    std::ofstream config_stream( stats_dir + "/cache_config.dat" );
    for( const auto &c_pair : all_caches_ )
    {
        config_stream << c_pair.first << ": " << *c_pair.second << "\n";
    }
    config_stream.flush();
    config_stream.close();
    return true;
}

// All valid metrics are returned as a positive number.
// Negative return value is an error and is of type stats_error_t.
int_least64_t
cache_simulator_t::get_cache_metric(    metric_name_t metric, 
                                        unsigned level, 
                                        unsigned core,
                                        cache_split_t split) const
{
    caching_device_t *curr_cache;

    if (core >= knobs_->num_cores) 
    {
        return STATS_ERROR_WRONG_CORE_NUMBER;
    }

    if (split == cache_split_t::DATA) {
        curr_cache = l1_dcaches_[core];
    } else {
        curr_cache = l1_icaches_[core];
    }

    for (size_t i = 1; i < level; i++) {
        caching_device_t *parent = curr_cache->get_parent();

        if (parent == NULL) {
            return STATS_ERROR_WRONG_CACHE_LEVEL;
        }
        curr_cache = parent;
    }
    caching_device_stats_t *stats = curr_cache->get_stats();

    if (stats == NULL) {
        return STATS_ERROR_NO_CACHE_STATS;
    }

    return stats->get_metric(metric);
}

const cache_simulator_knobs_t &
cache_simulator_t::get_knobs() const
{
    auto *local_knobs = reinterpret_cast< knob_t* >( knobs_ );
    return( *local_knobs );
}

cache_t *
cache_simulator_t::create_cache(const std::string &policy)
{
    if (policy == REPLACE_POLICY_NON_SPECIFIED || // default LRU
        policy == REPLACE_POLICY_LRU)             // set to LRU
        return new cache_lru_t;
    if (policy == REPLACE_POLICY_LFU) // set to LFU
        return new cache_t;
    if (policy == REPLACE_POLICY_FIFO) // set to FIFO
        return new cache_fifo_t;

    // undefined replacement policy
    ERRMSG("Usage error: undefined replacement policy. "
           "Please choose " REPLACE_POLICY_LRU " or " REPLACE_POLICY_LFU ".\n");
    return NULL;
}
