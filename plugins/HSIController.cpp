/**
 * @file HSIController.cpp HSIController class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "HSIController.hpp"

#include "timinglibs/TimingIssues.hpp"
#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"

#include "timing/HSIDesignInterface.hpp"

#include "timing/timingfirmwareinfo/Nljs.hpp"
#include "timing/timingfirmwareinfo/Structs.hpp"

#include "ers/Issue.hpp"
#include "logging/Logging.hpp"

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace dunedaq {

namespace hsilibs {

HSIController::HSIController(const std::string& name)
  : dunedaq::timinglibs::TimingEndpointControllerBase(name, 9) // 2nd arg: how many hw commands can this module send?
  , m_endpoint_state(0)
  , m_control_hardware_io(false)
  , m_clock_frequency(62.5e6)
  , m_thread(std::bind(&HSIController::gather_monitor_data, this, std::placeholders::_1))
{
  // this controller talks to the hw directly
  m_hw_command_out_connection = "";

  register_command("conf", &HSIController::do_configure);
  register_command("start", &HSIController::do_start);
  register_command("stop_trigger_sources", &HSIController::do_stop);
  register_command("scrap", &HSIController::do_scrap);
  
  // hsi hardware commands
  //register_command("hsi_reset", &HSIController::do_hsi_reset);
  //register_command("hsi_configure", &HSIController::do_hsi_configure);
  //register_command("hsi_start", &HSIController::do_hsi_start);
  //register_command("hsi_stop", &HSIController::do_hsi_stop);
  //register_command("hsi_print_status", &HSIController::do_hsi_print_status);
}

void
HSIController::init(std::shared_ptr<appfwk::ModuleConfiguration> mcfg)
{
  TimingController::init(mcfg);
  auto mod_config = mcfg->module<hsilibs::dal::HSIController>(get_name());
  m_hsi_configuration = mod_config->get_configuration()->cast<hsilibs::dal::HSIControllerConf>();
}

void
HSIController::do_configure(const nlohmann::json& data)
{
  TimingController::do_configure(data);

  m_control_hardware_io = m_hsi_configuration->get_control_hardware_io();

  configure_uhal(m_hsi_configuration); // configure hw ipbus connection

  try
  {
    m_hsi_device = std::make_unique<uhal::HwInterface>(m_connection_manager->getDevice(m_timing_device));
  } catch (const uhal::exception::ConnectionUIDDoesNotExist& exception) {
    std::stringstream message;
    message << "UHAL device name not " << m_timing_device << " in connections file";
    throw timinglibs::UHALDeviceNameIssue(ERS_HERE, message.str(), exception);
  }

  m_thread.start_working_thread("gather-hsi-info");

  configure_hardware_or_recover_state<timinglibs::TimingEndpointNotReady>(data, "HSI endpoint", m_endpoint_state.load());

  TLOG() << get_name() << " conf done for hsi endpoint, device: " << m_timing_device;
}

void
HSIController::do_start(const nlohmann::json& data)
{
  TimingController::do_start(data); // set sent cmd counters to 0
  do_hsi_start(data);
}

void
HSIController::do_stop(const nlohmann::json& data)
{
  do_hsi_stop(data);
}

void
HSIController::do_scrap(const nlohmann::json& data)
{
  m_thread.stop_working_thread();
  scrap_uhal();

  m_timing_device="";
  m_control_hardware_io=false;
  m_endpoint_state = 0x0;

  TimingController::do_scrap(data);
}

void
HSIController::send_configure_hardware_commands(const nlohmann::json& data)
{
  if (m_control_hardware_io)
  {
    m_thread.stop_working_thread();
    do_io_reset(data);
    m_thread.start_working_thread("gather-hsi-info");
  }
  do_hsi_reset(data);
  do_endpoint_reset(data);
  do_hsi_configure();
}

void
HSIController::do_io_reset(const nlohmann::json& )
{
  auto design = dynamic_cast<const timing::HSIDesignInterface*>(&m_hsi_device->getNode(""));

  auto clock_config = m_hsi_configuration->get_clock_config();
  if (m_hsi_configuration->get_soft()) {
    TLOG_DEBUG(0) << get_name() << ": " << m_timing_device << " soft io reset";
    design->soft_reset_io();
  } else if (!clock_config.empty()) {
    TLOG_DEBUG(0) << get_name() << ": " << m_timing_device
                  << " io reset, with supplied clk file: " << clock_config;
    design->reset_io(clock_config);
  }
  else
  {
    TLOG_DEBUG(0) << get_name() << ": " << m_timing_device
                  << " io reset, with supplied clk source: " << m_hsi_configuration->get_clock_source();
    design->reset_io(static_cast<timing::ClockSource>(m_hsi_configuration->get_clock_source()));
  }

  ++(m_sent_hw_command_counters.at(0).atomic);
}

void
HSIController::do_endpoint_enable(const nlohmann::json& data)
{
  auto ept_address = m_hsi_configuration->get_address();
  TLOG_DEBUG(0) << "ept enable hw cmd; a: " << ept_address;

  auto design = dynamic_cast<const timing::HSIDesignInterface*>(&m_hsi_device->getNode(""));
  design->get_endpoint_node_plain(m_managed_endpoint_id)->enable(ept_address, 0);

  ++(m_sent_hw_command_counters.at(1).atomic);
}

void
HSIController::do_endpoint_disable(const nlohmann::json& data)
{
  TLOG_DEBUG(0) << "ept disable hw cmd";

  auto design = dynamic_cast<const timing::HSIDesignInterface*>(&m_hsi_device->getNode(""));
  design->get_endpoint_node_plain(m_managed_endpoint_id)->disable();
  ++(m_sent_hw_command_counters.at(2).atomic);
}

void
HSIController::do_endpoint_reset(const nlohmann::json& data)
{
  auto ept_address = m_hsi_configuration->get_address();
  TLOG_DEBUG(0) << "ept reset hw cmd; a: " << ept_address;

  auto design = dynamic_cast<const timing::HSIDesignInterface*>(&m_hsi_device->getNode(""));
  design->get_endpoint_node_plain(m_managed_endpoint_id)->reset(ept_address, 0);
  ++(m_sent_hw_command_counters.at(3).atomic);
}

void
HSIController::do_hsi_reset(const nlohmann::json&)
{
  TLOG_DEBUG(0) << get_name() << ": " << m_timing_device << " hsi reset";

  auto design = dynamic_cast<const timing::HSIDesignInterface*>(&m_hsi_device->getNode(""));
  design->get_hsi_node().reset_hsi();
  ++(m_sent_hw_command_counters.at(4).atomic);
}

void
HSIController::do_hsi_configure()
{
  TLOG_DEBUG(0) << get_name() << ": " << m_timing_device << ", hsi configure";
  auto random_rate = m_hsi_configuration->get_trigger_rate();
  do_hsi_configure(random_rate);
}

void
HSIController::do_hsi_configure(double random_rate)
{
  TLOG_DEBUG(0) << get_name() << ": " << m_timing_device << " hsi configure";

  auto data_source = m_hsi_configuration->get_data_source();
  auto rising_edge_mask = m_hsi_configuration->get_rising_edge_mask();
  auto falling_edge_mask = m_hsi_configuration->get_falling_edge_mask();
  auto invert_edge_mask = m_hsi_configuration->get_invert_edge_mask();

  if (random_rate <= 0) {
    throw timinglibs::InvalidTriggerRateValue(ERS_HERE, random_rate);
  }

  TLOG_DEBUG(0) << get_name() << " Setting emulated event rate [Hz] to: "
         << random_rate;

  auto design = dynamic_cast<const timing::HSIDesignInterface*>(&m_hsi_device->getNode(""));
  design->configure_hsi(
    data_source, rising_edge_mask, falling_edge_mask, invert_edge_mask, random_rate);
  ++(m_sent_hw_command_counters.at(5).atomic);
}

void
HSIController::do_hsi_start(const nlohmann::json&)
{
   TLOG_DEBUG(0) << get_name() << ": " << m_timing_device << " hsi start";

  auto design = dynamic_cast<const timing::HSIDesignInterface*>(&m_hsi_device->getNode(""));
  design->get_hsi_node().start_hsi();
  ++(m_sent_hw_command_counters.at(6).atomic);
}

void
HSIController::do_hsi_stop(const nlohmann::json&)
{
  TLOG_DEBUG(0) << get_name() << ": " << m_timing_device << " hsi stop";

  auto design = dynamic_cast<const timing::HSIDesignInterface*>(&m_hsi_device->getNode(""));
  design->get_hsi_node().stop_hsi();
  ++(m_sent_hw_command_counters.at(7).atomic);
}

void
HSIController::do_hsi_print_status(const nlohmann::json&)
{
  TLOG_DEBUG(0) << get_name() << ": " << m_timing_device << " hsi print status";

  auto design = dynamic_cast<const timing::HSIDesignInterface*>(&m_hsi_device->getNode(""));
  TLOG_DEBUG(0) << std::endl << design->get_hsi_node().get_status();
  ++(m_sent_hw_command_counters.at(8).atomic);
}

//void
//HSIController::get_info(opmonlib::InfoCollector& ci, int /*level*/)
//{
//  // send counters internal to the module
//  hsicontrollerinfo::Info module_info;
//  module_info.sent_hsi_io_reset_cmds = m_sent_hw_command_counters.at(0).atomic.load();
//  module_info.sent_hsi_endpoint_enable_cmds = m_sent_hw_command_counters.at(1).atomic.load();
//  module_info.sent_hsi_endpoint_disable_cmds = m_sent_hw_command_counters.at(2).atomic.load();
//  module_info.sent_hsi_endpoint_reset_cmds = m_sent_hw_command_counters.at(3).atomic.load();
//  module_info.sent_hsi_reset_cmds = m_sent_hw_command_counters.at(4).atomic.load();
//  module_info.sent_hsi_configure_cmds = m_sent_hw_command_counters.at(5).atomic.load();
//  module_info.sent_hsi_start_cmds = m_sent_hw_command_counters.at(6).atomic.load();
//  module_info.sent_hsi_stop_cmds = m_sent_hw_command_counters.at(7).atomic.load();
//  module_info.sent_hsi_print_status_cmds = m_sent_hw_command_counters.at(8).atomic.load();
//  module_info.device_infos_received_count = m_device_infos_received_count;
//
//  ci.add(module_info);
//}

void
HSIController::process_device_info(nlohmann::json info)
{
  ++m_device_infos_received_count;

  timing::timingfirmwareinfo::TimingDeviceInfo device_info;
  from_json(info, device_info);

  auto ept_info = device_info.endpoint_info;
  m_endpoint_state = device_info.endpoint_info.state;
  bool ready = ept_info.ready;

  bool ept_good = (m_endpoint_state == 0x8) && ready;

  auto hsi_info = device_info.hsi_info;
  auto buffer_enabled = hsi_info.buffer_enabled;
  auto buffer_error = hsi_info.buffer_error;
  auto buffer_warning = hsi_info.buffer_warning;
  auto re_mask = hsi_info.re_mask;
  auto fe_mask = hsi_info.fe_mask;
  auto inv_mask = hsi_info.inv_mask;
  auto data_source = hsi_info.source;

  bool hsi_good = buffer_enabled && !buffer_error && !buffer_warning
                  && re_mask == m_hsi_configuration->get_rising_edge_mask()
                  && fe_mask == m_hsi_configuration->get_falling_edge_mask()
                  && inv_mask == m_hsi_configuration->get_invert_edge_mask()
                  && data_source == m_hsi_configuration->get_data_source();

  TLOG_DEBUG(0) << "EPT good: " << ept_good << ", HSI good: " << hsi_good << ", infos received: " << m_device_infos_received_count;

  TLOG_DEBUG(0) << "device data: " << info.dump();

  if (ept_good && hsi_good)
  {
    if (!m_device_ready)
    {
      m_device_ready = true;
      TLOG_DEBUG(2) << "HSI device became ready";
    }
  }
  else
  {
    if (m_device_ready)
    {
      m_device_ready = false;
      TLOG_DEBUG(2) << "HSI device no longer ready";
    }
  }
}

void
HSIController::gather_monitor_data(std::atomic<bool>& running_flag)
{
  while (running_flag.load()) {

    timing::timingfirmwareinfo::TimingDeviceInfo device_info;
    // collect the data from the hardware
    try
    {
      auto design = dynamic_cast<const timing::HSIDesignInterface*>(&m_hsi_device->getNode(""));
      design->get_info(device_info);
    } catch (const std::exception& excpt) {
      ers::warning(timinglibs::FailedToCollectOpMonInfo(ERS_HERE, m_timing_device, excpt));
    }

    nlohmann::json info;
    to_json(info, device_info);
    process_device_info(info);

    auto prev_gather_time = std::chrono::steady_clock::now();
    auto next_gather_time = prev_gather_time + std::chrono::milliseconds(500);

    // check running_flag periodically
    auto slice_period = std::chrono::microseconds(10000);
    auto next_slice_gather_time = prev_gather_time + slice_period;

    bool break_flag = false;
    while (next_gather_time > next_slice_gather_time + slice_period) {
      if (!running_flag.load()) {
        TLOG_DEBUG(0) << "while waiting to gather data, negative run flag detected.";
        break_flag = true;
        break;
      }
      std::this_thread::sleep_until(next_slice_gather_time);
      next_slice_gather_time = next_slice_gather_time + slice_period;
    }
    if (break_flag == false) {
      std::this_thread::sleep_until(next_gather_time);
    }
  }
}

} // namespace hsilibs
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::hsilibs::HSIController)

// Local Variables:
// c-basic-offset: 2
// End:
