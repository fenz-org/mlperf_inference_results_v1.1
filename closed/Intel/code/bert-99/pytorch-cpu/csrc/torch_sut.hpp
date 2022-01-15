#pragma once

#include <cassert>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>

#include <query_sample.h>
#include <condition_variable>
#include <system_under_test.h>
#include <query_sample_library.h>

#include "kmp_launcher.hpp"
#include "bert_qsl.hpp"
#include "bert_model.hpp"

class BertServerSUT : public mlperf::SystemUnderTest {

  using Queue_t = std::list<mlperf::QuerySample>;
  // using Queue_t = std::forward_list<mlperf::QuerySample>
  // using Queue_t = std::deque<mlperf::QuerySample>;
public:
  // configure inter parallel and intra paralel
  // 4x10 core required for expected performance
  BertServerSUT (
      const std::string& model_file,
      const std::string& samples_file,
      int inter_parallel,
      int intra_parallel,
      int batch, bool ht = true
  );

  ~BertServerSUT ();

  void IssueQuery(const std::vector<mlperf::QuerySample>& samples) override;

  void FlushQueries() override {}
  void ReportLatencyResults(
      const std::vector<mlperf::QuerySampleLatency>& latencies_ns) override {}

  const std::string& Name() const override {
    static const std::string name("BERT Server");
    return name;
  }

  static void QuerySamplesComplete(
      const std::vector<mlperf::QuerySample>& samples,
      const at::Tensor& results
  );

  static void QuerySamplesComplete(
      const mlperf::QuerySample& sample,
      const at::Tensor& result
  );

  mlperf::QuerySampleLibrary* GetQSL() {
    return &qsl_;
  }

private:
  qsl::SquadQuerySampleLibrary qsl_;
  models::TorchModel model_;

  std::condition_variable ctrl_;
  std::mutex mtx_;

  Queue_t mQueue_;
  bool mStop_ {false};

  // Control over max samples a instance will peek
  size_t mThreshold_;
  size_t slength_ {384};

  std::vector<std::thread> mInstances_;
  int nProcsPerInstance_;
  int nInstances_;
  bool mHt_;

private:
  int rootProc(int index);
  void thInstance(int index);
};

class BertOfflineSUT : public mlperf::SystemUnderTest {

  using Queue_t = std::list<mlperf::QuerySample>;
  // using Queue_t = std::forward_list<mlperf::QuerySample>
  // using Queue_t = std::deque<mlperf::QuerySample>;
public:
  // configure inter parallel and intra paralel
  // 4x10 core required for expected performance
  BertOfflineSUT (
      const std::string& model_file,
      const std::string& samples_file,
      int inter_parallel,
      int intra_parallel,
      int batch, bool ht = true
  );

  ~BertOfflineSUT ();

  void IssueQuery(const std::vector<mlperf::QuerySample>& samples) override;

  void FlushQueries() override {}
  void ReportLatencyResults(
      const std::vector<mlperf::QuerySampleLatency>& latencies_ns) override {}

  const std::string& Name() const override {
    static const std::string name("BERT Offline");
    return name;
  }

  static void QuerySamplesComplete(
      const std::vector<mlperf::QuerySample>& samples,
      const at::Tensor& results
  );

  static void QuerySamplesComplete(
      const mlperf::QuerySample& sample,
      const at::Tensor& result
  );

  mlperf::QuerySampleLibrary* GetQSL() {
    return &qsl_;
  }

private:
  qsl::SquadQuerySampleLibrary qsl_;
  models::TorchModel model_;

  std::condition_variable ctrl_;
  std::mutex mtx_;

  Queue_t mQueue_;
  bool mStop_ {false};

  // Control over max samples a instance will peek
  size_t mThreshold_;
  size_t slength_ {385};

  std::vector<std::thread> mInstances_;
  int nProcsPerInstance_;
  int nInstances_;
  bool mHt_;

private:
  int rootProc(int index);
  void thInstance(int index);
};

class BertSUT : public mlperf::SystemUnderTest {
  class Instance : public kmp::KMPLauncher {
    std::condition_variable ctrl_;
    std::mutex mtx_;

    int index_;
    BertSUT &sut_;

    // How many units does the instance ocuppy
    uint64_t mUnits_;

    bool mStop_;

    // Must be the last member
    std::thread th_;

    // Launch root thread of this instance
    void root_thread();

    // Get root thread place
    int RootPlace() const;
  public:
    Instance(int index, BertSUT &sut);
    Instance(const Instance&) = delete;
    Instance(Instance&& r): sut_(r.sut_) { // TODO: deal with it later
    }

    using KMPLauncher::setAffinityPlaces;

    void setUnitsMask(uint64_t units) {
      mUnits_ = units;
    }
    uint64_t getUnitsMask() const {
      return mUnits_;
    }

    void inference(const std::vector<mlperf::QuerySample>& samples);
  };

public:
  // configure inter parallel and intra paralel
  // 4x10 core required for expected performance
  BertSUT(
      const std::string& model_file,
      const std::string& samples_file,
      int inter_parallel = 10,
      int intra_parallel = 4
  );

  void IssueQuery(const std::vector<mlperf::QuerySample>& samples) override;
  void FlushQueries() override {}

  void IssueQueryOffline(const std::vector<mlperf::QuerySample>& samples);

  Instance& getInstance(int root) {
    return mInstances_.at(root);
  }

  friend class BertSUT::Instance;

private:
  qsl::SquadQuerySampleLibrary qsl_;
  models::TorchModel model_;

  int nCoresPerInstance_;
  int nInstances_;
  std::vector<Instance> mInstances_;

  // Bitmaps for intra-cores (fixed currently) blocks on each socket,
  // XXX:
  //   Unit positions are backwards, first is furthest in core map
  //   Core positions are normally sequential
  //
  //   Example: first unit, third core in 28-core system would be core 26
  //
  std::atomic<uint64_t> aUnitsMap_;

  // TODO: seperate a class for these functionality
  // Try allocate computation units and block if can't
  uint64_t AllocUnits(int nUnit);

  // Try alloc computation units in a time frame
  uint64_t AllocUnits(int nUnits, uint64_t timeMs);

  // Unit to CPU cores
  uint64_t CoreMask(uint64_t unitsMask, int unitCores);
  std::vector<int> MaskToPlaces(uint64_t mask);
  uint64_t PlacesToMask(std::vector<int> places);

public:
  void FreeUnits(uint64_t mask);
};
