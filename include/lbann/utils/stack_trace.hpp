////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014-2016, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory.
// Written by the LBANN Research Team (B. Van Essen, et al.) listed in
// the CONTRIBUTORS file. <lbann-dev@llnl.gov>
//
// LLNL-CODE-697807.
// All rights reserved.
//
// This file is part of LBANN: Livermore Big Artificial Neural Network
// Toolkit. For details, see http://software.llnl.gov/LBANN or
// https://github.com/LLNL/LBANN.
//
// Licensed under the Apache License, Version 2.0 (the "Licensee"); you
// may not use this file except in compliance with the License.  You may
// obtain a copy of the License at:
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the license.
////////////////////////////////////////////////////////////////////////////////
#ifndef LBANN_UTILS_STACK_TRACE_HPP_INCLUDED
#define LBANN_UTILS_STACK_TRACE_HPP_INCLUDED

#include <string>

namespace lbann {
namespace stack_trace {

/** Get human-readable stack trace.
 *  Ignores stack frames within the lbann::stack_trace and
 *  lbann::lbann_exception namespaces. Calls non-reentrant functions,
 *  so behaviour may be undefined if used within a signal handler.
 */
std::string get();

/** Register signal handler.
 *  Initializes a signal handler that prints an error message and
 *  stack trace to the standard error stream when a signal is
 *  detected. If desired, it also writes to the file
 *  "stack_trace_rank<MPI rank>.txt". 
 *  
 *  This functionality is somewhat risky since the handler calls
 *  non-reentrant functions, which can result in undefined behavior
 *  (see https://www.ibm.com/developerworks/library/l-reent/). That
 *  said, it is possible (likely?) that our handler will work
 *  correctly. And if there's a SIGSEV and it doesn't, nothing much is
 *  lost (IMO).
 */
void register_signal_handler(bool write_to_file = false);

} //namespace stack_trace 
} //namespace lbann

#endif // LBANN_UTILS_STACK_TRACE_HPP_INCLUDED
