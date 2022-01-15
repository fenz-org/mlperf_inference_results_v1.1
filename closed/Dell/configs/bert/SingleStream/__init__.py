# Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
import sys
sys.path.insert(0, os.getcwd())

from code.common.constants import Benchmark, Scenario
from code.common.system_list import System, Architecture, MIGConfiguration, MIGSlice
from configs.configuration import *


@ConfigRegistry.register(HarnessType.Custom, AccuracyTarget.k_99, PowerSetting.MaxP)
class XE2420_A10x1(BenchmarkConfiguration):
    system = System("XE2420_A10", Architecture.Ampere, 1)
    bert_opt_seqlen = 270
    coalesced_tensor = True
    gpu_batch_size = 1
    gpu_copy_streams = 1
    gpu_inference_streams = 1
    input_dtype = "int32"
    input_format = "linear"
    precision = "int8"
    tensor_path = "${PREPROCESSED_DATA_DIR}/squad_tokenized/input_ids.npy,${PREPROCESSED_DATA_DIR}/squad_tokenized/segment_ids.npy,${PREPROCESSED_DATA_DIR}/squad_tokenized/input_mask.npy"
    use_small_tile_gemm_plugin = False
    enable_interleaved = False
    gemm_plugin_fairshare_cache_size = 120
    single_stream_expected_latency_ns = 2800000
    use_graphs = True
    scenario = Scenario.SingleStream
    benchmark = Benchmark.BERT


@ConfigRegistry.register(HarnessType.Custom, AccuracyTarget.k_99, PowerSetting.MaxQ)
class XE2420_A10x1_MaxQ(XE2420_A10x1):
    pass


@ConfigRegistry.register(HarnessType.Custom, AccuracyTarget.k_99, PowerSetting.MaxP)
class XR12_A10x2(BenchmarkConfiguration):
    system = System("XR12_A10", Architecture.Ampere, 2)
    bert_opt_seqlen = 270
    coalesced_tensor = True
    gpu_batch_size = 1
    gpu_copy_streams = 1
    gpu_inference_streams = 1
    input_dtype = "int32"
    input_format = "linear"
    precision = "int8"
    tensor_path = "${PREPROCESSED_DATA_DIR}/squad_tokenized/input_ids.npy,${PREPROCESSED_DATA_DIR}/squad_tokenized/segment_ids.npy,${PREPROCESSED_DATA_DIR}/squad_tokenized/input_mask.npy"
    use_small_tile_gemm_plugin = False
    enable_interleaved = False
    gemm_plugin_fairshare_cache_size = 120
    single_stream_expected_latency_ns = 2800000
    use_graphs = True
    scenario = Scenario.SingleStream
    benchmark = Benchmark.BERT

