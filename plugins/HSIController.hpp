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

#include "hsilibs/dal/HSIControllerConf.hpp"
#include "hsilibs/dal/HSIController.hpp"

#include "timinglibs/TimingEndpointControllerBase.hpp"
#include "timinglibs/TimingHardwareInterface.hpp"

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
class HSIController : public dunedaq::timinglibs::TimingEndpointControllerBase, dunedaq::timinglibs::TimingHardwareInterface
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

  void init(std::shared_ptr<appfwk::ModuleConfiguration> mcfg) override;
protected:
  const hsilibs::dal::HSIControllerConf* m_hsi_configuration;

  std::unique_ptr<uhal::HwInterface> m_hsi_device;
  bool m_control_hardware_io;

  dunedaq::utilities::WorkerThread m_thread;
  void gather_monitor_data(std::atomic<bool>&);

  // Commands
  void do_configure(const nlohmann::json& data) override;
  void do_start(const nlohmann::json& data) override;
  void do_stop(const nlohmann::json& data) override;
  void do_scrap(const nlohmann::json& data) override;
  void send_configure_hardware_commands(const nlohmann::json& data) override;

  // overriding these to talk directly to hw
  void do_io_reset(const nlohmann::json& data) override;
  void do_endpoint_enable(const nlohmann::json& data) override;
  void do_endpoint_disable(const nlohmann::json& data) override;
  void do_endpoint_reset(const nlohmann::json& data) override;

  // hsi commands
  void do_hsi_reset(const nlohmann::json&);
  void do_hsi_configure(double random_rate);
  void do_hsi_configure();
  void do_hsi_start(const nlohmann::json&);
  void do_hsi_stop(const nlohmann::json&);

  void do_hsi_print_status(const nlohmann::json&);

  // op mon info
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
