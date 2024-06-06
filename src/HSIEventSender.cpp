/**
 * @file HSIEventSender.cpp HSIEventSender class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "hsilibs/HSIEventSender.hpp"

#include "appfwk/app/Nljs.hpp"
#include "iomanager/IOManager.hpp"
#include "logging/Logging.hpp"
#include "confmodel/DaqModule.hpp"
#include "confmodel/Connection.hpp"

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace dunedaq {
namespace hsilibs {

HSIEventSender::HSIEventSender(const std::string& name)
  : dunedaq::appfwk::DAQModule(name)
  , m_hsievent_send_connection("")
  , m_queue_timeout(1)
  , m_hsievent_sender(nullptr)
  , m_sent_counter(0)
  , m_failed_to_send_counter(0)
  , m_last_sent_timestamp(0)
{
}

void
HSIEventSender::init(std::shared_ptr<appfwk::ModuleConfiguration> mcfg)
{
  auto mdal = mcfg->module<confmodel::DaqModule>(get_name()); // Only need generic DaqModule for output

  if (!mdal) {
    throw appfwk::CommandFailed(ERS_HERE, "init", get_name(), "Unable to retrieve configuration object");
  }

  for (auto con : mdal->get_outputs()) {
    if (con->get_data_type() == datatype_to_string<dfmessages::HSIEvent>()) {
      m_hsievent_send_connection = con->UID();
    }
  }

  m_hsievent_sender = get_iom_sender<dfmessages::HSIEvent>(m_hsievent_send_connection);
}

bool
HSIEventSender::ready_to_send(std::chrono::milliseconds timeout)
{
  if (m_hsievent_sender == nullptr) {
    return false;
  }
  return m_hsievent_sender->is_ready_for_sending(timeout);
}

void
HSIEventSender::send_hsi_event(dfmessages::HSIEvent& event)
{
  TLOG_DEBUG(3) << get_name() << ": Sending HSIEvent to " << m_hsievent_send_connection << ". \n"
                << event.header << ", " << std::bitset<32>(event.signal_map) << ", " << event.timestamp << ", "
                << event.sequence_counter << "\n";

  bool was_successfully_sent = false;
  while (!was_successfully_sent) {
    try {
      dfmessages::HSIEvent event_copy(event);
      m_hsievent_sender->send(std::move(event_copy), m_queue_timeout);
      ++m_sent_counter;
      m_last_sent_timestamp.store(event.timestamp);
      was_successfully_sent = true;
    } catch (const dunedaq::iomanager::TimeoutExpired& excpt) {
      std::ostringstream oss_warn;
      oss_warn << "push to output connection \"" << m_hsievent_send_connection << "\"";
      ers::error(dunedaq::iomanager::TimeoutExpired(ERS_HERE, get_name(), oss_warn.str(), m_queue_timeout.count()));
      ++m_failed_to_send_counter;
    }
  }
  if (m_sent_counter > 0 && m_sent_counter % 200000 == 0)
    TLOG_DEBUG(3) << "Have sent out " << m_sent_counter << " HSI events";
}

void
HSIEventSender::send_raw_hsi_data(const std::array<uint32_t, 7>& raw_data, raw_sender_ct* sender)
{
  HSI_FRAME_STRUCT payload;
  ::memcpy(&payload, &raw_data[0], sizeof(HSI_FRAME_STRUCT));

  TLOG_DEBUG(3) << get_name() << ": Sending HSI_FRAME_STRUCT " << std::hex << "0x" << payload.frame.version << ", 0x"
                << payload.frame.detector_id

                << "; 0x" << payload.frame.timestamp_low << "; 0x" << payload.frame.timestamp_high << "; 0x"
                << payload.frame.input_low << "; 0x" << payload.frame.input_high << "; 0x" << payload.frame.trigger
                << "; 0x" << payload.frame.sequence << std::endl;

  try {
    // TODO deal with this
    if (!sender) {
      throw(QueueIsNullFatalError(ERS_HERE, get_name(), "HSIEventSender output"));
    }
    sender->send(std::move(payload), m_queue_timeout);
  } catch (const dunedaq::iomanager::TimeoutExpired& excpt) {
    std::ostringstream oss_warn;
    oss_warn << "push to output raw hsi data queue failed";
    ers::error(dunedaq::iomanager::TimeoutExpired(ERS_HERE, get_name(), oss_warn.str(), m_queue_timeout.count()));
    ++m_failed_to_send_counter;
  }
}

} // namespace hsilibs
} // namespace dunedaq

// Local Variables:
// c-basic-offset: 2
// End:
