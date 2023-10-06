#ifndef _SIMULATOR_CREATE_H_
#define _SIMULATOR_CREATE_H_ 1

#include <cstdint>
#include <string>
#include <limits>

#include "analysis_tool.h"

/**
 * @file drmemtrace/simulator_create.h
 * @brief DrMemtrace cache simulator creation.
 */
struct simulator_knobs_t 
{
    
    simulator_knobs_t() = default;
    

    simulator_knobs_t( const std::uint64_t start_pc, 
                       const std::uint64_t stop_pc ) : start_pc( start_pc ),
                                                       stop_pc ( stop_pc  )
    {
    }


    unsigned int num_cores          = 4;
    uint64_t skip_refs              = 0;
    uint64_t warmup_refs            = 0;
    double warmup_fraction          = 0.0;
    uint64_t sim_refs               = std::numeric_limits< std::uint64_t >::max();
    bool cpu_scheduling             = false;
    std::string stats_dir           = "";
    unsigned int verbose            = 0;
    std::uint64_t start_pc          = 0;
    std::uint64_t stop_pc           = 0;
};

#endif /* _SIMULATOR_CREATE_H_ */
