/**
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef HSILIBS_PLUGINS_HSIREADOUT_HPP_
#define HSILIBS_PLUGINS_HSIREADOUT_HPP_

#include "hsilibs/HSIEventSender.hpp"
#include "hsilibs/hsireadout/Nljs.hpp"
#include "hsilibs/hsireadout/Structs.hpp"
#include "hsilibs/hsireadoutinfo/InfoNljs.hpp"
#include "hsilibs/hsireadoutinfo/InfoStructs.hpp"

#include "appfwk/DAQModule.hpp"
#include "dfmessages/HSIEvent.hpp"
#include "timing/HSINode.hpp"
#include "uhal/ConnectionManager.hpp"
#include "uhal/ProtocolUDP.hpp"
#include "uhal/log/exception.hpp"
#include "uhal/utilities/files.hpp"
#include "utilities/WorkerThread.hpp"
#include <ers/Issue.hpp>

#include <bitset>
#include <chrono>
#include <deque>
#include <memory>
#include <random>
#include <shared_mutex>
#include <string>
#include <vector>

namespace dunedaq {
namespace hsilibs {

/**
 * @brief HSIReadout generates fake HSIEvent messages
 * and pushes them to the configured output queue.
 */
class HSIReadout : public hsilibs::HSIEventSender
{
public:
  /**
   * @brief HSIReadout Constructor
   * @param name Instance name for this HSIReadout instance
   */
  explicit HSIReadout(const std::string& name);

  HSIReadout(const HSIReadout&) = delete;            ///< HSIReadout is not copy-constructible
  HSIReadout& operator=(const HSIReadout&) = delete; ///< HSIReadout is not copy-assignable
  HSIReadout(HSIReadout&&) = delete;                 ///< HSIReadout is not move-constructible
  HSIReadout& operator=(HSIReadout&&) = delete;      ///< HSIReadout is not move-assignable

  void init(const nlohmann::json& obj) override;
  void get_info(opmonlib::InfoCollector& ci, int level) override;

private:
  // Commands
  hsireadout::ConfParams m_cfg;
  void do_configure(const nlohmann::json& obj) override;
  void do_start(const nlohmann::json& obj) override;
  void do_stop(const nlohmann::json& obj) override;
  void do_scrap(const nlohmann::json& obj) override;

  std::shared_ptr<raw_sender_ct> m_raw_hsi_data_sender;
  
  void do_hsi_work(std::atomic<bool>&);
  dunedaq::utilities::WorkerThread m_thread;

  // Configuration
  std::string m_hsi_device_name;
  uint m_readout_period; // NOLINT(build/unsigned)

  std::string m_connections_file;
  std::unique_ptr<uhal::ConnectionManager> m_connection_manager;
  std::unique_ptr<uhal::HwInterface> m_hsi_device;
  std::atomic<daqdataformats::run_number_t> m_run_number;

  std::atomic<uint64_t> m_readout_counter;        // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_last_readout_timestamp; // NOLINT(build/unsigned)

  std::deque<uint16_t> m_buffer_counts; // NOLINT(build/unsigned)
  std::shared_mutex m_buffer_counts_mutex;
  void update_buffer_counts(uint16_t new_count); // NOLINT(build/unsigned)
  double read_average_buffer_counts();
};
} // namespace hsilibs
} // namespace dunedaq

#endif // HSILIBS_PLUGINS_HSIREADOUT_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
