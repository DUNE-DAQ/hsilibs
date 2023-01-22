/**
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef HSILIBS_PLUGINS_FAKEHSIEVENTGENERATOR_HPP_
#define HSILIBS_PLUGINS_FAKEHSIEVENTGENERATOR_HPP_

#include "hsilibs/HSIEventSender.hpp"

#include "timinglibs/TimestampEstimator.hpp"
#include "hsilibs/fakehsieventgenerator/Nljs.hpp"
#include "hsilibs/fakehsieventgenerator/Structs.hpp"
#include "hsilibs/fakehsieventgeneratorinfo/InfoNljs.hpp"
#include "hsilibs/fakehsieventgeneratorinfo/InfoStructs.hpp"

#include "appfwk/DAQModule.hpp"
#include "daqdataformats/Types.hpp"
#include "dfmessages/TimeSync.hpp"
#include "ers/Issue.hpp"
#include "iomanager/Receiver.hpp"
#include "utilities/WorkerThread.hpp"

#include <bitset>
#include <chrono>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace dunedaq {
namespace hsilibs {

/**
 * @brief FakeHSIEventGenerator generates fake HSIEvent messages
 * and pushes them to the configured output queue.
 */
class FakeHSIEventGenerator : public hsilibs::HSIEventSender
{
public:
  /**
   * @brief FakeHSIEventGenerator Constructor
   * @param name Instance name for this FakeHSIEventGenerator instance
   */
  explicit FakeHSIEventGenerator(const std::string& name);

  FakeHSIEventGenerator(const FakeHSIEventGenerator&) = delete; ///< FakeHSIEventGenerator is not copy-constructible
  FakeHSIEventGenerator& operator=(const FakeHSIEventGenerator&) =
    delete;                                                ///< FakeHSIEventGenerator is not copy-assignable
  FakeHSIEventGenerator(FakeHSIEventGenerator&&) = delete; ///< FakeHSIEventGenerator is not move-constructible
  FakeHSIEventGenerator& operator=(FakeHSIEventGenerator&&) = delete; ///< FakeHSIEventGenerator is not move-assignable

  void init(const nlohmann::json& obj) override;
  void get_info(opmonlib::InfoCollector& ci, int level) override;

private:
  // Commands
  void do_configure(const nlohmann::json& obj) override;
  void do_start(const nlohmann::json& obj) override;
  void do_stop(const nlohmann::json& obj) override;
  void do_scrap(const nlohmann::json& obj) override;
  void do_change_rate(const nlohmann::json& obj);

  void dispatch_timesync(dfmessages::TimeSync& message);
  void convert_timesync_to_hsi_event(dfmessages::TimeSync& message);

  std::shared_ptr<iomanager::ReceiverConcept<dfmessages::TimeSync>> m_timesync_receiver;

  // Threading
  void do_hsievent_work(std::atomic<bool>&) override;

  // Configuration
  std::string m_timesync_topic;
  std::atomic<daqdataformats::run_number_t> m_run_number;

  // Helper class for estimating DAQ time
  std::unique_ptr<timinglibs::TimestampEstimator> m_timestamp_estimator;

  // Random Generatior
  std::default_random_engine m_random_generator;
  std::uniform_int_distribution<uint32_t> m_uniform_distribution; // NOLINT(build/unsigned)
  std::poisson_distribution<uint64_t> m_poisson_distribution;     // NOLINT(build/unsigned)

  uint32_t generate_signal_map(); // NOLINT(build/unsigned)

  uint64_t m_clock_frequency;                     // NOLINT(build/unsigned)
  std::atomic<float> m_trigger_rate;
  std::atomic<float> m_active_trigger_rate;
  std::atomic<uint64_t> m_event_period;           // NOLINT(build/unsigned)
  int64_t m_timestamp_offset;

  uint32_t m_hsi_device_id;            // NOLINT(build/unsigned)
  uint m_signal_emulation_mode;        // NOLINT(build/unsigned)
  uint64_t m_mean_signal_multiplicity; // NOLINT(build/unsigned)

  uint32_t m_enabled_signals;                       // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_generated_counter;        // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_last_generated_timestamp; // NOLINT(build/unsigned)

  std::atomic<uint64_t> m_received_timesync_count; // NOLINT(build/unsigned)
};
} // namespace hsilibs
} // namespace dunedaq

#endif // HSILIBS_PLUGINS_FAKEHSIEVENTGENERATOR_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
