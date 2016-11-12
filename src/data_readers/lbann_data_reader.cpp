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
//
// lbann_data_reader .hpp .cpp - Input data base class for training, testing
////////////////////////////////////////////////////////////////////////////////

#include "lbann/data_readers/lbann_data_reader.hpp"

using namespace std;

void lbann::DataReader::calculate_multi_model_data_distribution(lbann_comm *comm) {
  int max_mini_batch_size = BatchSize;
  int num_parallel_readers_per_model = (m_stride / comm->get_num_models()) / max_mini_batch_size;
  int min_stride_across_models = max_mini_batch_size * comm->get_num_models();  /// Given that each model has to have at least one reader, what is the minimum stride

  m_last_mini_batch_size = max_mini_batch_size; /// By default the last mini-batch is a full one

  int num_whole_mini_batches_per_model = floor(getNumData() / min_stride_across_models);
  int num_whole_mini_batches_per_reader = floor(num_whole_mini_batches_per_model / num_parallel_readers_per_model);
  int parallel_readers_with_extra_mini_batch = num_whole_mini_batches_per_model % num_parallel_readers_per_model;
  int per_model_partial_mini_batch_size = (getNumData() - (num_whole_mini_batches_per_model * min_stride_across_models))/(comm->get_num_models());
  int world_master_remainder_data = 0;

  // Compute how many full "parallel" mini-batches are available
  m_last_mini_batch_threshold = num_whole_mini_batches_per_model * min_stride_across_models;

  if(comm->get_rank_in_model() < parallel_readers_with_extra_mini_batch) {
    num_whole_mini_batches_per_reader += 1;
    m_last_mini_batch_size = max_mini_batch_size;
  }

  m_num_mini_batches_per_reader = num_whole_mini_batches_per_reader;

  int world_master_remainder_adjustment = getNumData() 
    - (num_whole_mini_batches_per_model * min_stride_across_models) 
    - (per_model_partial_mini_batch_size * comm->get_num_models());
  if(comm->am_world_master()) {
    world_master_remainder_data = world_master_remainder_adjustment;
    world_master_remainder_adjustment = 0;
  }
  per_model_partial_mini_batch_size += world_master_remainder_data;

  /// The first reader that doesn't have an extra mini batch gets the partial batch
  if(comm->get_rank_in_model() == parallel_readers_with_extra_mini_batch && per_model_partial_mini_batch_size > 0) {
    m_num_mini_batches_per_reader++;
    m_last_mini_batch_size = per_model_partial_mini_batch_size;
  }

  if(m_last_mini_batch_size > max_mini_batch_size) { throw new lbann_exception("Error in calculating the partial mini-batch size, exceeds the max mini-batch size"); }

  /// Note that comm->get_model_rank() + comm->get_rank_in_model() is not equivalent to comm->get_world_rank() from a parallel I/O perspective
  /// Given the data readers model rank, how many models have a higher rank

  /// By default the last stride of each reader is part of a regular (full) round
  m_last_mini_batch_stride = m_stride;

  int last_mini_batch_offset = max(0, num_whole_mini_batches_per_reader - 1) * m_stride;

  ///  The last mini-batch may be partial and thus may have a smaller stride
  if(comm->get_rank_in_model() == parallel_readers_with_extra_mini_batch && per_model_partial_mini_batch_size > 0) {
    m_last_mini_batch_stride = (m_last_mini_batch_threshold - m_base_offset - m_model_offset - last_mini_batch_offset) 
      + comm->get_model_rank() * per_model_partial_mini_batch_size + world_master_remainder_adjustment; /// BVE 10/18/16
  }

  //  cout << "[" << comm->get_rank_in_world() << "] " << comm->get_model_rank() << " model rank, "<< comm->get_rank_in_model() << " rank in model, num_whole_mini_batches_per_model " << num_whole_mini_batches_per_model << " num_whole_mini_batches_per_reader " << num_whole_mini_batches_per_reader << "(m_num_mini_batches_per_reader=" << m_num_mini_batches_per_reader << ") parallel_readers_with_extra_mini_batch " << parallel_readers_with_extra_mini_batch << " partial_mini_batch_size=" << per_model_partial_mini_batch_size << " last mini bath size=" << m_last_mini_batch_size << " world_master_remainder_data=" << world_master_remainder_data << " threshold " << m_last_mini_batch_threshold << " with a last stride of " << m_last_mini_batch_stride << " and stride of " << m_stride << " and there are " << num_parallel_readers_per_model << " parallel readers per model" << " last mini batch offset = " << last_mini_batch_offset <<  " parallel reader with extra minibatch = " << parallel_readers_with_extra_mini_batch << " model bracket = " << (parallel_readers_with_extra_mini_batch * max_mini_batch_size + per_model_partial_mini_batch_size + world_master_remainder_data) <<" base ofset "<< m_base_offset << " model offset " << m_model_offset <<endl;

  return;
}