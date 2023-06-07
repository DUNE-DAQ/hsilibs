/**
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef HSILIBS_INCLUDE_HSILIBS_HSIEVENTSENDER_HPP_
#define HSILIBS_INCLUDE_HSILIBS_HSIEVENTSENDER_HPP_

#include "hsilibs/Issues.hpp"
#include "hsilibs/Types.hpp"

#include "appfwk/DAQModule.hpp"
#include "dfmessages/HSIEvent.hpp"
#include "utilities/WorkerThread.hpp"
#include "iomanager/IOManager.hpp"
#include <ers/Issue.hpp>

#include <bitset>
#include <chrono>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace dunedaq {
namespace hsilibs {

/**
 * @brief HSIEventSender provides an interface to process and
 * and send HSIEvents.
 */
class HSIEventSender : public dunedaq::appfwk::DAQModule
{
public:
  /**
   * @brief HSIEventSender Constructor
   * @param name Instance name for this HSIEventSender instance
   */
  explicit HSIEventSender(const std::string& name);

  HSIEventSender(const HSIEventSender&) = delete;            ///< HSIEventSender is not copy-constructible
  HSIEventSender& operator=(const HSIEventSender&) = delete; ///< HSIEventSender is not copy-assignable
  HSIEventSender(HSIEventSender&&) = delete;                 ///< HSIEventSender is not move-constructible
  HSIEventSender& operator=(HSIEventSender&&) = delete;      ///< HSIEventSender is not move-assignable

  void init(const nlohmann::json& init_data) override;
protected:
  // Commands
  virtual void do_configure(const nlohmann::json& obj) = 0;
  virtual void do_start(const nlohmann::json& obj) = 0;
  virtual void do_stop(const nlohmann::json& obj) = 0;
  virtual void do_scrap(const nlohmann::json& obj) = 0;

  // Configuration
  std::string m_hsievent_send_connection;
  std::chrono::milliseconds m_queue_timeout;

  using raw_sender_ct = iomanager::SenderConcept<HSI_FRAME_STRUCT>;

  using hsievent_sender_ct = iomanager::SenderConcept<dfmessages::HSIEvent>;
  std::shared_ptr<hsievent_sender_ct> m_hsievent_sender;

  // push events to HSIEvent output queue
  virtual void send_hsi_event(dfmessages::HSIEvent& event);
  virtual void send_raw_hsi_data(const std::array<uint32_t, 7>& raw_data, raw_sender_ct* sender);

  std::atomic<uint64_t> m_sent_counter;           // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_failed_to_send_counter; // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_last_sent_timestamp;    // NOLINT(build/unsigned)
};
} // namespace hsilibs
} // namespace dunedaq

#endif // TIMINGLIBS_SRC_HSIEVENTSENDER_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
