/**
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef HSILIBS_PLUGINS_FAKEHSIEVENTGENERATOR_HPP_
#define HSILIBS_PLUGINS_FAKEHSIEVENTGENERATOR_HPP_

#include "hsilibs/HSIEventSender.hpp"

#include "utilities/TimestampEstimator.hpp"

#include "appfwk/DAQModule.hpp"
#include "daqdataformats/Types.hpp"
#include "appmodel/FakeHSIEventGeneratorConf.hpp"
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
 * @brief FakeHSIEventGeneratorModule generates fake HSIEvent messages
 * and pushes them to the configured output queue.
 */
class FakeHSIEventGeneratorModule : public hsilibs::HSIEventSender
{
public:
  /**
   * @brief FakeHSIEventGeneratorModule Constructor
   * @param name Instance name for this FakeHSIEventGeneratorModule instance
   */
  explicit FakeHSIEventGeneratorModule(const std::string& name);

  FakeHSIEventGeneratorModule(const FakeHSIEventGeneratorModule&) = delete; ///< FakeHSIEventGeneratorModule is not copy-constructible
  FakeHSIEventGeneratorModule& operator=(const FakeHSIEventGeneratorModule&) =
    delete;                                                ///< FakeHSIEventGeneratorModule is not copy-assignable
  FakeHSIEventGeneratorModule(FakeHSIEventGeneratorModule&&) = delete; ///< FakeHSIEventGeneratorModule is not move-constructible
  FakeHSIEventGeneratorModule& operator=(FakeHSIEventGeneratorModule&&) = delete; ///< FakeHSIEventGeneratorModule is not move-assignable

  void init(std::shared_ptr<appfwk::ModuleConfiguration> mcfg) override;
  //void get_info(opmonlib::InfoCollector& ci, int level) override;

private:
  // Commands
  void do_configure(const nlohmann::json& obj) override;
  void do_start(const nlohmann::json& obj) override;
  void do_stop(const nlohmann::json& obj) override;
  void do_scrap(const nlohmann::json& obj) override;
  void do_change_rate(const nlohmann::json& obj);

  std::shared_ptr<raw_sender_ct> m_raw_hsi_data_sender;
  
  void do_hsi_work(std::atomic<bool>&);
  dunedaq::utilities::WorkerThread m_thread;

  std::shared_ptr<iomanager::ReceiverConcept<dfmessages::TimeSync>> m_timesync_receiver;

  // Configuration
  std::atomic<daqdataformats::run_number_t> m_run_number;

  // Helper class for estimating DAQ time
  std::unique_ptr<utilities::TimestampEstimator> m_timestamp_estimator;

  // Random Generatior
  std::default_random_engine m_random_generator;
  std::uniform_int_distribution<uint32_t> m_uniform_distribution; // NOLINT(build/unsigned)
  std::poisson_distribution<uint64_t> m_poisson_distribution;     // NOLINT(build/unsigned)

  uint32_t generate_signal_map(); // NOLINT(build/unsigned)

  const appmodel::FakeHSIEventGeneratorConf* m_params;
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
};
} // namespace hsilibs
} // namespace dunedaq

#endif // HSILIBS_PLUGINS_FAKEHSIEVENTGENERATOR_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
