/**
 * @file Types.hpp
 *
 *  Contains declaration of HSI_FRAME_STRUCT.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef HSILIBS_INCLUDE_HSILIBS_TYPES_HPP_
#define HSILIBS_INCLUDE_HSILIBS_TYPES_HPP_

#include "detdataformats/HSIFrame.hpp"
#include "daqdataformats/FragmentHeader.hpp"
#include "daqdataformats/SourceID.hpp"
#include "serialization/Serialization.hpp"

#include <algorithm> // For std::min
#include <cassert>   // For assert()
#include <cstdio>
#include <cstdlib>
#include <stdexcept> // For std::out_of_range
#include <stdint.h>  // For uint32_t etc

namespace dunedaq {
namespace hsilibs {

/**
 * @brief For HSI the numbers are different.
 * 1[HSI frames] x 24[Bytes] = 24[Bytes]
 * */
const constexpr std::size_t HSI_FRAME_STRUCT_SIZE = 28;

class HSI_FRAME_STRUCT
{
public:
  using FrameType = HSI_FRAME_STRUCT;
  
  dunedaq::detdataformats::HSIFrame frame;

  // comparable based on start timestamp
  bool operator<(const FrameType& other) const
  {
    return this->get_first_timestamp() < other.get_first_timestamp() ? true : false;
  }
  
  uint64_t get_timestamp() const // NOLINT(build/unsigned)
  {
    return frame.get_timestamp(); // NOLINT
  }

  uint64_t get_first_timestamp() const // NOLINT(build/unsigned)
  {
    return frame.get_timestamp(); // NOLINT
  }

  void set_first_timestamp(uint64_t ts) // NOLINT(build/unsigned)
  {
    frame.set_timestamp(ts);
  }

  FrameType* begin() { return this; }

  FrameType* end() { return (this + 1); } // NOLINT

  size_t get_payload_size() { return HSI_FRAME_STRUCT_SIZE; }

  size_t get_num_frames() { return 1; }

  size_t get_frame_size() { return HSI_FRAME_STRUCT_SIZE; }

  static const constexpr daqdataformats::SourceID::Subsystem subsystem =
    daqdataformats::SourceID::Subsystem::kHwSignalsInterface;
  static const constexpr daqdataformats::FragmentType fragment_type = daqdataformats::FragmentType::kHardwareSignal;
  static const constexpr uint64_t expected_tick_difference = 0; // NOLINT(build/unsigned)
};

static_assert(sizeof(struct HSI_FRAME_STRUCT) == HSI_FRAME_STRUCT_SIZE,
              "Check your assumptions on HSI_FRAME_STRUCT");

} // namespace hsilibs

DUNE_DAQ_TYPESTRING(hsilibs::HSI_FRAME_STRUCT, "HSIFrame")

} // namespace dunedaq

#endif // HSILIBS_INCLUDE_HSILIBS_TYPES_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
