/**
 * vm_smap_entry.hpp - 
 * @author: Jonathan Beard
 * @version: Thu Mar 31 06:16:00 2022
 * 
 * Copyright 2022 Jonathan Beard
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef VM_SMAP_ENTRY_HPP
#define VM_SMAP_ENTRY_HPP  1

#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <cinttypes>
#include <array>

#include "vm_map_entry.hpp"

struct vm_smap_entry_t : vm_maps_entry_t 
{
    using field_type_t        = std::int32_t;
#define PRINT_TYPE  PRIi32    
    using field_name_t  = const char[ 32 ];

    vm_smap_entry_t();
    virtual ~vm_smap_entry_t() = default;
    
    virtual void add_fields_to_entry( std::ifstream&, const std::string& ) override;

    /** rest of fields **/
    enum field_name_idx : int { 
        size = 0,
        kernel_page_size,
        mmu_page_size,
        rss,
        pss,
        shared_clean,
        shared_dirty,
        private_clean,
        private_dirty,
        referenced,
        anonymous,
        lazy_free,
        anon_huge_pages,
        shmem_pmd_mapped,
        file_pmd_mapped,
        shared_huge_tlb,
        private_huge_tlb,
        swap,
        swap_pss,
        locked,
        thp_eligible,
        n_fields
    };
    
    constexpr static std::array< field_name_t, 
                                 vm_smap_entry_t::n_fields > field_name = { 
        "size",
        "kernel_page_size",
        "mmu_page_size",
        "rss",
        "pss",
        "shared_clean",
        "shared_dirty",
        "private_clean",
        "private_dirty",
        "referenced",
        "anonymous",
        "lazy_free",
        "anon_huge_pages",
        "shmem_pmd_mapped",
        "file_pmd_mapped",
        "shared_huge_tlb",
        "private_huge_tlb",
        "swap",
        "swap_pss",
        "locked",
        "thp_eligible"
    };

    std::array< field_type_t, 
                vm_smap_entry_t::n_fields > field_data  = { 0 };
    std::vector< std::string >              vm_flags;
};


std::ostream& 
operator << ( std::ostream &stream, const vm_smap_entry_t &entry);


#endif /* END VM_SMAP_ENTRY_HPP */
