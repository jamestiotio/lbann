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
// lbann_layer .h .cpp - Parent class for all layer types
////////////////////////////////////////////////////////////////////////////////

#ifndef LBANN_LAYER_HPP_INCLUDED
#define LBANN_LAYER_HPP_INCLUDED

#include "lbann/lbann_base.hpp"
#include "lbann/lbann_comm.hpp"
#include "lbann/layers/lbann_layer_activations.hpp"
#include "lbann/utils/lbann_summary.hpp"
#include "lbann/optimizers/lbann_optimizer.hpp"
#include "lbann/optimizers/lbann_optimizer_sgd.hpp"
#include "lbann/optimizers/lbann_optimizer_adagrad.hpp"
#include "lbann/optimizers/lbann_optimizer_rmsprop.hpp"
#include "lbann/optimizers/lbann_optimizer_adam.hpp"
#include "lbann/utils/lbann_exception.hpp"
#include <string>
#include <vector>

namespace lbann
{

// Forward-declare this.
class regularizer;
class model;

  class Layer {
  public:
    Layer(const uint index, lbann_comm* comm, Optimizer *optimizer,
          uint mbsize, activation_type activation=activation_type::ID,
          std::vector<regularizer*> regs={});
    virtual ~Layer();
    virtual DataType forwardProp(DataType prev_WBL2NormSum);
    virtual void backProp();
    virtual bool update() { return false; };
    virtual void summarize(lbann_summary& summarizer, int64_t step);
    /**
     * Print information at the end of an epoch.
     * This is always called on the model masters and should synchronize
     * printing if needed.
     */
    virtual void epoch_print() const {}
    /**
     * Called on every layer at the end of each epoch to give it the chance to
     * reset/clean up.
     */
    virtual void epoch_reset() {}
    virtual DataType checkGradientMB(Layer& PrevLayer, const DataType Epsilon=1e-4) {
      return 0.0;
    };

    virtual void setup(int);

    /** Return the index of this layer. */
    inline uint get_index() const { return Index; }
    /** Return (a view of) the weights/biases matrix for this layer. */
    virtual ElMat& get_weights_biases() { return *m_weights; }
    /** Return (a view of) the weights/biases gradient matrix for this layer. */
    virtual ElMat& get_weights_biases_gradient() { return *m_weights_gradient; }
    /** Return (a view of) the activations matrix for this layer. */
    virtual ElMat& get_activations() { return *m_activations; }
    /** Return the layer's optimizer. */
    virtual Optimizer* get_optimizer() const { return optimizer; }
    /** Reset layer stat counters. */
    virtual void reset_counters() {
      fp_time = 0.0;
      bp_time = 0.0;
    }

    /** Return the size of mini-batch this layer uses. */
    virtual uint get_minibatch_size() const {
      return m_mini_batch_size;
    }
    /**
     * Get the "effective" size of a mini-batch.
     * This is for backward propagation, etc. when there are more updates being
     * contributed than the local mini-batch size implies (e.g. when doing
     * inter-model updates).
     */
    virtual uint get_effective_minibatch_size() const {
      return m_effective_mbsize;
    }
    /** Set the effective size of a mini-batch to size. */
    virtual void set_effective_minibatch_size(uint size) {
      m_effective_mbsize = size;
    }

    ElMat *fp_output();
    ElMat *bp_output();
    void setup_fp_input(ElMat *fp_input);
    void setup_bp_input(ElMat *bp_input);

    /* void updateMB(const float LearnRate); */
    //    virtual double computeCost(DistMat &deltas) = 0;
    //    { return 0.0;}

    bool saveToFile(int fd, const char* filename);
    bool loadFromFile(int fd, const char* filename);

    bool saveToCheckpoint(int fd, const char* filename, uint64_t* bytes);
    bool loadFromCheckpoint(int fd, const char* filename, uint64_t* bytes);

    bool saveToCheckpointShared(const char* dir, uint64_t* bytes);
    bool loadFromCheckpointShared(const char* dir, uint64_t* bytes);

  public:
    uint               Index;                  // Layer index (start with 0)
    uint 		NumNeurons; 	// # neurons
    execution_mode  m_execution_mode;
    activation_type m_activation_type;

    // TODO: move to lbann_layer_fully_connected.hpp
    ElMat *m_weights;            /// Weight-bias matrix Weight and Bias Set ((# neurons + 1) x (# previous layer's neurons + 1))
    ElMat *m_weights_gradient;   /// Gradient w.r.t. weight-bias matrix Weights and Bias Gradient ((# neurons + 1) x (# previous layer's neurons + 1))
    ElMat *m_preactivations;     /// Output of forward pass linear transformation Zs ((# neurons + 1) x mini-batch size)
    ElMat *m_prev_error_signal;  // Local copy of the error signal from "previous" layer ((# neurons + 1) x mini-batch size)

    ElMat *m_error_signal;       // Error signal to "next" layer Temporary deltas for computation ((# neurons + 1) x mini-batch size)
    ElMat *m_activations;        // Activations ((# neurons + 1) x mini-batch size)
    ElMat *m_prev_activations;   // Local copy of the activations from the "previous" layer ((# previous layer's neurons + 1) x mini-batch size)

    /// Create a view of each matrix so that it can accomodate partial mini-batches
    ElMat *m_weights_v;
    ElMat *m_weights_gradient_v;
    ElMat *m_preactivations_v;
    ElMat *m_prev_error_signal_v;
    ElMat *m_error_signal_v;
    ElMat *m_activations_v;
    ElMat *m_prev_activations_v;

    Optimizer *optimizer;

    ElMat *fp_input;            /// Pointer to input for the forward propagation - no local storage
    ElMat *bp_input;            /// Pointer to the input for the backward propagation - no local storage

    lbann_comm* comm;
    model* neural_network_model;
  protected:
    /** Setup views of the matrices for the layer's forward and backward propagation. */
    virtual void fp_set_std_matrix_view();
    /** Apply the layer's linear update in forward propagation. */
    virtual void fp_linearity() {}
    /** Handle the layer's linearity in backward propagation. */
    virtual void bp_linearity() {}
    /** Apply the layer's nonlinearity in forward propagation. */
    virtual void fp_nonlinearity();
    /** Handle the layer's nonlinearity in backward propagation. */
    virtual void bp_nonlinearity();

    /** Activation function */
    Activation* m_activation_fn;
    /** Regularizers being applied to the layer. */
    std::vector<regularizer*> regularizers;
    /** Size of the local mini-batch. */
    uint m_mini_batch_size;
    /** "Effective" mini-batch size for backward propagation, etc.. */
    uint m_effective_mbsize;

    /** Time spent in forward propagation. */
    double fp_time;
    /** Time spent in backward propagation. */
    double bp_time;
  };
}


#endif // LBANN_LAYER_HPP_INCLUDED
