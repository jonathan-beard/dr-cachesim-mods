/**
 * vm_defs.hpp - 
 * @author: Jonathan Beard
 * @version: Tue Apr 12 09:14:09 2022
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
#ifndef VM_DEFS_HPP
#define VM_DEFS_HPP  1

#ifndef UNUSED 
#ifdef __clang__
#define UNUSED( x ) (void)(x)
#else
#define UNUSED( x )[&x]{}()
#endif
//FIXME need to double check to see IF THIS WORKS ON MSVC
#endif

#endif /* END VM_DEFS_HPP */
