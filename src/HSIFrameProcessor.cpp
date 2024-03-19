/**
 * @file HSIFrameProcessor.cpp HSI specific Task based raw processor
 * implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "hsilibs/Types.hpp"
#include "HSIFrameProcessor.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <string>

using dunedaq::readoutlibs::logging::TLVL_FRAME_RECEIVED;

namespace dunedaq {
namespace hsilibs {

HSIFrameProcessor::HSIFrameProcessor(std::unique_ptr<readoutlibs::FrameErrorRegistry>& error_registry)
  : TaskRawDataProcessorModel<hsilibs::HSI_FRAME_STRUCT>(error_registry)
  , m_previous_ts(0)
{}

void 
HSIFrameProcessor::conf(const nlohmann::json& args)
{
  inherited::add_preprocess_task(
    std::bind(&HSIFrameProcessor::timestamp_check, this, std::placeholders::_1));
  // m_tasklist.push_back( std::bind(&HSIFrameProcessor::frame_error_check, this, std::placeholders::_1) );
  inherited::conf(args);
}

void
HSIFrameProcessor::timestamp_check(frameptr fp)
{
  // Acquire timestamp
  timestamp_t current_ts = fp->get_timestamp();
  uint64_t k_clock_frequency = 62500000; // NOLINT(build/unsigned)
  TLOG_DEBUG(TLVL_FRAME_RECEIVED) << "Received HSI frame timestamp value of " << current_ts << " ticks (..."
    << std::fixed << std::setprecision(8) << (static_cast<double>(current_ts % (k_clock_frequency*1000)) / static_cast<double>(k_clock_frequency)) << " sec)"; // NOLINT

  if (current_ts < m_previous_ts) {
    TLOG() << "*** Data Integrity ERROR *** Current HSIFrame timestamp " << current_ts << " is before previous timestamp " << m_previous_ts;
  }

  if (current_ts == 0) {
    TLOG() << "*** Data Integrity ERROR *** Current HSIFrame timestamp " << current_ts << " is 0";
  }

  m_previous_ts = current_ts;
  m_last_processed_daq_ts = current_ts;
}

/**
 * Pipeline Stage 2.: Check for errors
 * */
void 
HSIFrameProcessor::frame_error_check(frameptr /*fp*/)
{
  // check error fields
}

} // namespace hsilibs
} // namespace dunedaq