/**
 * @file HSIController.hpp
 *
 * HSIController is a DAQModule implementation that
 * provides that provides a control interface for an HSI endpoint.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef HSILIBS_PLUGINS_HSICONTROLLER_HPP_
#define HSILIBS_PLUGINS_HSICONTROLLER_HPP_

#include "hsilibs/hsicontroller/Nljs.hpp"
#include "hsilibs/hsicontroller/Structs.hpp"
#include "hsilibs/hsicontrollerinfo/InfoNljs.hpp"
#include "hsilibs/hsicontrollerinfo/InfoStructs.hpp"

#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"
#include "timinglibs/TimingController.hpp"

#include "appfwk/DAQModule.hpp"
#include "ers/Issue.hpp"
#include "logging/Logging.hpp"
#include "utilities/WorkerThread.hpp"

#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
namespace hsilibs {

/**
 * @brief HSIController is a DAQModule implementation that
 * provides that provides a control interface for a HSI endpoint.
 */
class HSIController : public dunedaq::timinglibs::TimingController
{
public:
  /**
   * @brief HSIController Constructor
   * @param name Instance name for this HSIController instance
   */
  explicit HSIController(const std::string& name);

  HSIController(const HSIController&) = delete;            ///< HSIController is not copy-constructible
  HSIController& operator=(const HSIController&) = delete; ///< HSIController is not copy-assignable
  HSIController(HSIController&&) = delete;                 ///< HSIController is not move-constructible
  HSIController& operator=(HSIController&&) = delete;      ///< HSIController is not move-assignable

private:
  hsicontroller::ConfParams m_hsi_configuration;

  // Commands
  void do_configure(const nlohmann::json& data) override;
  void do_start(const nlohmann::json& data) override;
  void do_stop(const nlohmann::json& data) override;
  void do_scrap(const nlohmann::json& data) override;
  void do_change_rate(const nlohmann::json& data);
  void send_configure_hardware_commands(const nlohmann::json& data) override;

  timinglibs::timingcmd::TimingHwCmd construct_hsi_hw_cmd(const std::string& cmd_id);

  // timinglibs hsi commands
  void do_hsi_io_reset(const nlohmann::json& data);
  void do_hsi_endpoint_enable(const nlohmann::json& data);
  void do_hsi_endpoint_disable(const nlohmann::json& data);
  void do_hsi_endpoint_reset(const nlohmann::json& data);

  void do_hsi_reset(const nlohmann::json&);
  void do_hsi_configure(const nlohmann::json& data);
  void do_hsi_configure_trigger_rate_override(nlohmann::json data, double trigger_rate_override);
  void do_hsi_start(const nlohmann::json&);
  void do_hsi_stop(const nlohmann::json&);

  void do_hsi_print_status(const nlohmann::json&);

  // pass op mon info
  void get_info(opmonlib::InfoCollector& ci, int level) override;
  void process_device_info(nlohmann::json info) override;

  std::atomic<uint> m_endpoint_state;
  uint64_t m_clock_frequency;                     // NOLINT(build/unsigned)

};
} // namespace hsilibs
} // namespace dunedaq

#endif // HSILIBS_PLUGINS_HSICONTROLLER_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
