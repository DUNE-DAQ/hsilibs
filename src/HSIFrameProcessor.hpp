/**
 * @file HSIFrameProcessor.hpp HSI specific Task based raw processor
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef HSILIBS_SRC_HSI_HSIFRAMEPROCESSOR_HPP_
#define HSILIBS_SRC_HSI_HSIFRAMEPROCESSOR_HPP_

#include "readoutlibs/ReadoutIssues.hpp"
#include "readoutlibs/models/TaskRawDataProcessorModel.hpp"

#include "hsilibs/Types.hpp"
#include "logging/Logging.hpp"
#include "readoutlibs/FrameErrorRegistry.hpp"
#include "readoutlibs/ReadoutLogging.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <string>

namespace dunedaq {
namespace hsilibs {

class HSIFrameProcessor : public readoutlibs::TaskRawDataProcessorModel<hsilibs::HSI_FRAME_STRUCT>
{

public:
  using inherited = readoutlibs::TaskRawDataProcessorModel<hsilibs::HSI_FRAME_STRUCT>;
  using frameptr = hsilibs::HSI_FRAME_STRUCT*;
  using timestamp_t = std::uint64_t; // NOLINT(build/unsigned)

  // Constructor
  explicit HSIFrameProcessor(std::unique_ptr<readoutlibs::FrameErrorRegistry>& error_registry)
    : TaskRawDataProcessorModel<hsilibs::HSI_FRAME_STRUCT>(error_registry)
  {}

  // Override config for pipeline setup
  void conf(const nlohmann::json& args) override;
  
protected:
  /**
   * Pipeline Stage 2.: Check for error
   * */
  void frame_error_check(frameptr /*fp*/);

  // Internals
  bool m_problem_reported = false;
  std::atomic<int> m_ts_error_ctr{ 0 };

private:
};

} // namespace hsilibs
} // namespace dunedaq

#endif // HSILIBS_SRC_HSI_HSIFRAMEPROCESSOR_HPP_
