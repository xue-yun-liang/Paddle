// Copyright (c) 2024 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "paddle/cinn/frontend/group_pattern.h"
#include "paddle/cinn/hlir/dialect/operator/ir/manual_op.h"



namespace cinn::frontend {

struct ShardableAxis {
  int axis;
  std::string axis_name;

  bool operator==(const ShardableAxis& other) const {
    return this->axis == other.axis && this->axis_name == other.axis_name;
  }

  static int64_t UnqiueSeqNo() {
    static std::atomic<int64_t> cnt(0);
    return ++cnt;
  }
};

using ShardableAxes = std::vector<ShardableAxis>;

struct SoleOutputShardableAxes {
  ShardableAxes shardable_axes;
};

struct ShardableAxesSignature {
  SoleOutputShardableAxes sole_output_sa;
  std::unordered_map<OpAndOperandIndex, ShardableAxes> input_shardable_axes;
};

class ShardableAxesProvider {
 public:
  ~ShardableAxesProvider() = default;

  virtual ShardableAxesSignature MakeShardableAxesSignature4Op(
      const pir::Operation* op) = 0;

 protected:
  ShardableAxesProvider() = default;
};

std::shared_ptr<ShardableAxesProvider> MakeDefaultShardableAxesProvider(
    const pir::ShapeConstraintIRAnalysis* shape_analysis);

class ShardableAxesInferer {
 public:
  explicit ShardableAxesInferer(
      const std::shared_ptr<ShardableAxesProvider>& shardable_axes_provider)
      : shardable_axes_provider_(shardable_axes_provider) {}

  ShardableAxesInferer(const ShardableAxesInferer&) = default;
  ShardableAxesInferer(ShardableAxesInferer&&) = default;

  ShardableAxesSignature MakeShardableAxesSignature4Op(
      const pir::Operation* op) {
    return shardable_axes_provider_->MakeShardableAxesSignature4Op(op);
  }

  std::unordered_map<pir::Value, ShardableAxes> InferShardableAxesFromSink(
      const pir::Operation* sink, const OpTopo& op_topo);

  std::unordered_map<pir::Value, ShardableAxes> InferShardableAxes(
      const OpSetPtr& ops);

 private:
  template <typename InputIt>
  std::unordered_map<pir::Value, ShardableAxes> ReversedInferShardableAxes(
      const common::TopoWalker<const pir::Operation*>& reversed_walker,
      InputIt sink_and_init_begin,
      InputIt sink_and_init_end);

  std::unordered_map<pir::Value, ShardableAxes> ReversedInferShardableAxes(
      const common::TopoWalker<const pir::Operation*>& reversed_walker,
      const pir::Operation* sink,
      const ShardableAxes& init_sa);

  std::unordered_map<const pir::Operation*, ShardableAxesSignature>
  GetOp2ShardableAxesSignature(const OpSetPtr& ops);

  std::map<std::string, std::vector<std::string>> GetAxisName2BoundAxisName(
      const OpSetPtr& ops,
      const std::unordered_map<const pir::Operation*, ShardableAxesSignature>&
          op2shardable_axes_signature);

  std::unordered_map<std::string, std::string> GetAxisName2UnionFindSetRoot(
      const OpSetPtr& ops,
      const std::unordered_map<const pir::Operation*, ShardableAxesSignature>&
          op2shardable_axes_signature);

  std::unordered_map<pir::Value, ShardableAxes> GetSinkAndInitShardableAxes(
      const std::list<const pir::Operation*>& sinks,
      const std::unordered_map<const pir::Operation*, ShardableAxesSignature>&
          op2shardable_axes_signature,
      const std::unordered_map<std::string, std::string>&
          axis_name2union_find_set_root);

  void RenameDuplicatedAxisName(
      std::unordered_map<pir::Value, ShardableAxes>* sink2sa);

  std::unordered_map<pir::Value, ShardableAxes> GetSinkAndInitValues(
      const common::TopoWalker<const pir::Operation*>& reverse_walker,
      const OpSetPtr& ops,
      const std::list<const pir::Operation*>& sinks);

  std::shared_ptr<ShardableAxesProvider> shardable_axes_provider_;
};

}  // namespace cinn::frontend
