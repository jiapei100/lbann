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

#include "lbann/objective_functions/geom_negloglike.hpp"

namespace lbann {

namespace objective_functions {

void geom_negloglike::compute_value(const AbsDistMat& predictions,
                                       const AbsDistMat& ground_truth) {
  
  // Get local matrices and matrix parameters
  const Mat& predictions_local = predictions.LockedMatrix();
  const Mat& ground_truth_local = ground_truth.LockedMatrix();
  const El::Int width = predictions.Width();
  const El::Int local_height = predictions_local.Height();
  const El::Int local_width = predictions_local.Width();

  // Compute sum of Geometric negative log-likelihood with Kahan summation
  double sum = 0;
  double correction = 0;
  for(El::Int col = 0; col < local_width; ++col) {
    for(El::Int row = 0; row < local_height; ++row) {
      const double true_val = ground_truth_local(row, col);
      const double pred_val = predictions_local(row, col);
      double term = -(true_val*std::log(1-pred_val) + std::log(pred_val))
      term += correction;
      const double next_sum = sum + term;
      correction = term - (next_sum - sum);
      sum = next_sum;
    }
  }
  
  // Compute Geometric negative log-likelihood error
  double geom_nll = sum / height;
  geom_nll = El::mpi::AllReduce(geom_nll, predictions.DistComm());

  // Update objective function value
  add_to_value(geom_nll);

}

/// Compute derivative of Geometric negative log-likelihood objective function
void geom_negloglike::compute_gradient(const AbsDistMat& predictions,
                                          const AbsDistMat& ground_truth,
                                          AbsDistMat& gradient) {

  // Get local matrices and matrix parameters
  const Mat& predictions_local = predictions.LockedMatrix();
  const Mat& ground_truth_local = ground_truth.LockedMatrix();
  Mat& gradient_local = gradient.Matrix();

  // Compute gradient
  El::IndexDependentFill(gradient_local,
                         (std::function<DataType(El::Int,El::Int)>)
                         ([&predictions_local, &ground_truth_local]
                          (El::Int r, El::Int c) -> DataType {
                           const DataType pred_val = predictions_local(r,c);
                           const DataType true_val = ground_truth_local(r,c);
                           return true_val/(1 - pred_val) - 1/pred_val;
                         }));

}

}  // namespace objective_functions

}  // namespace lbann