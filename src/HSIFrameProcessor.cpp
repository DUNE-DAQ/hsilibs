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

namespace dunedaq {
namespace hsilibs {

void 
HSIFrameProcessor::conf(const nlohmann::json& args)
{
  // m_tasklist.push_back( std::bind(&HSIFrameProcessor::frame_error_check, this, std::placeholders::_1) );
  inherited::conf(args);
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
