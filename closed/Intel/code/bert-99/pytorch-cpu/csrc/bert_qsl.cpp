#include <c10/core/MemoryFormat.h>
#include <c10/core/TensorOptions.h>
#include <c10/util/Exception.h>
#include <cassert>
#include "bert_qsl.hpp"

namespace qsl {
TensorList SquadQuerySampleLibrary::GetTensorListFrom(
    at::IValue value) {
  std::vector<at::Tensor> tensor_list;
  auto toTensor = [](at::IValue item) {return item.toTensor();};

  if (value.isList()) {
    auto value_list = value.toList();

    std::transform(value_list.begin(), value_list.end(),
        std::back_inserter(tensor_list), toTensor);
  } else if (value.isTensorList()) {
    auto c10_list = value.toTensorList();
    tensor_list.insert(tensor_list.begin(), c10_list.begin(), c10_list.end());
  }else if (value.isTuple()) {
    auto value_list = value.toTuple()->elements();
    std::transform(value_list.begin(), value_list.end(),
        std::back_inserter(tensor_list), toTensor);
  } else {
    TORCH_CHECK(false, "Can't get TensorList from IValue type: ", value.tagKind());
  }

  return tensor_list;
}

TensorList SquadQuerySampleLibrary::GetTensorListFrom(
    const std::string& filename) {
  caffe2::serialize::PyTorchStreamReader reader(filename);
  auto stack = torch::jit::readArchiveAndTensors("data",
      "","",
      c10::nullopt,
      c10::nullopt,
      c10::nullopt,
      reader);

  return GetTensorListFrom(stack);
}

TensorList SquadQuerySampleLibrary::GetTensorListFrom(
    c10::Dict<at::IValue, at::IValue>& dict,
    const char* name) {
  at::IValue ivname (name);

  auto tensor_list = dict.find(ivname);
  if ( tensor_list != dict.end() )
    return GetTensorListFrom(tensor_list->value());
  else
    return TensorList();
}

c10::Dict<at::IValue, at::IValue> SquadQuerySampleLibrary::GetDictFrom(
    const std::string& filename) {
  caffe2::serialize::PyTorchStreamReader reader(filename);
  auto stack = torch::jit::readArchiveAndTensors("data",
      "","",
      c10::nullopt,
      c10::nullopt,
      c10::nullopt,
      reader);

  // Exception management
  return stack.toGenericDict();
}

SquadQuerySampleLibrary::SquadQuerySampleLibrary(
    const std::string& filename,
    const char* input_ids_name,
    const char* input_mask_name,
    const char* segment_ids_name) {
  auto datasets = GetDictFrom(filename);
  input_ids_set_ = GetTensorListFrom(datasets, input_ids_name);
  input_mask_set_ = GetTensorListFrom(datasets, input_mask_name);
  segment_ids_set_ = GetTensorListFrom(datasets, segment_ids_name);
  CheckSampleCount();
}

SquadQuerySampleLibrary::SquadQuerySampleLibrary(
    const std::string& f_input_ids,
    const std::string& f_input_mask,
    const std::string& f_segment_ids) {
  input_ids_set_ = GetTensorListFrom(f_input_ids);
  input_mask_set_ = GetTensorListFrom(f_input_mask);
  segment_ids_set_ = GetTensorListFrom(f_segment_ids);
  CheckSampleCount();
}

SquadQuerySampleLibrary SquadQuerySampleLibrary::Create(
    const std::string& filename) {
  return SquadQuerySampleLibrary(filename);
}

void SquadQuerySampleLibrary::CheckSampleCount() {
  /* throw if three sets have different sizes */
}

//
// Parallel bucket sort (unstable) would be the most efficient choice
// For length 40 ~ 384, each with a bucket of std::list
//
Queue_t SquadQuerySampleLibrary::Sort(
    const std::vector<QuerySample>& samples, bool reverse,
    size_t minLength, size_t maxLength) const {
  const auto lengthOffset = minLength;
  const auto nBucket = maxLength - lengthOffset +1;

  std::vector<Queue_t> Buckets(nBucket);
  std::vector<std::mutex> lks (nBucket);

  // (Parallel) sort
  // TODO: support other parallel library
# pragma omp parallel for
  for (const auto &sample : samples) {
    auto length = GetFeatureLength(sample.index);

    auto idx = reverse ? maxLength - length : length - lengthOffset;
    auto& bucket = Buckets[idx];
    auto& l = lks[idx];

    {
      std::unique_lock<std::mutex> guard(l);
      bucket.emplace_back(sample);
    }
  }

  // Splice them togather
  Queue_t result;
  for (auto &q : Buckets)
    result.splice(result.end(), std::move(q));

  return result;
}

//
// Assemble samples into larger batch
//
Stack SquadQuerySampleLibrary::AssembleSamples(
    std::vector<QuerySampleIndex> indices) const {
  TensorList ids_list, mask_list, segid_list;

  ids_list.reserve(indices.size());
  mask_list.reserve(indices.size());
  segid_list.reserve(indices.size());

  int64_t maxLength = 0;

  for (auto index : indices) {
    auto input_ids = input_ids_set_[index];
    auto input_mask = input_mask_set_[index];
    auto segment_ids = segment_ids_set_[index];
 
    if (maxLength == 0)
      maxLength = input_ids.size(0);

    if (input_ids.size(0) < maxLength) {
      // Padding needed
      std::vector<int64_t> newShape {maxLength};

      auto len = input_ids.size(0);
      auto opts = at::TensorOptions().dtype<int>().memory_format(at::MemoryFormat::Contiguous);

      auto padded_ids = at::zeros(newShape, opts);
      auto padded_mask = at::zeros(newShape, opts);
      auto padded_segids = at::zeros(newShape, opts);

      padded_ids.narrow(0,0,len).copy_(input_ids);
      padded_mask.narrow(0,0,len).copy_(input_mask);
      padded_segids.narrow(0,0,len).copy_(segment_ids);

      ids_list.emplace_back(padded_ids);
      mask_list.emplace_back(padded_mask);
      segid_list.emplace_back(padded_segids);
    } else {
      ids_list.emplace_back(input_ids);
      mask_list.emplace_back(input_mask);
      segid_list.emplace_back(segment_ids);
    }
  }
  auto input_ids = at::vstack(ids_list);
  auto input_mask = at::vstack(mask_list);
  auto segment_ids = at::vstack(segid_list);
  return Stack { input_ids, input_mask, segment_ids };
}

}
