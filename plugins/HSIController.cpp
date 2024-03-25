/**
 * @file HSIController.cpp HSIController class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "HSIController.hpp"
#include "hsilibs/hsicontroller/Nljs.hpp"
#include "hsilibs/hsicontroller/Structs.hpp"

#include "timinglibs/TimingIssues.hpp"
#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"

#include "timing/timingendpointinfo/InfoNljs.hpp"
#include "timing/timingendpointinfo/InfoStructs.hpp"
#include "timing/HSIDesignInterface.hpp"

#include "opmonlib/JSONTags.hpp"
#include "appfwk/cmd/Nljs.hpp"
#include "ers/Issue.hpp"
#include "logging/Logging.hpp"
#include "rcif/cmd/Nljs.hpp"

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace dunedaq {

namespace hsilibs {

HSIController::HSIController(const std::string& name)
  : dunedaq::timinglibs::TimingController(name, 9) // 2nd arg: how many hw commands can this module send?
  , m_endpoint_state(0)
  , m_control_hardware_io(false)
  , m_clock_frequency(62.5e6)
  , m_thread(std::bind(&HSIController::gather_monitor_data, this, std::placeholders::_1))
{
  register_command("conf", &HSIController::do_configure);
  register_command("start", &HSIController::do_start);
  register_command("stop_trigger_sources", &HSIController::do_stop);
  register_command("change_rate", &HSIController::do_change_rate);
  register_command("scrap", &HSIController::do_scrap);
  
  // timing endpoint hardware commands
  register_command("hsi_io_reset", &HSIController::do_hsi_io_reset);
  register_command("hsi_endpoint_enable", &HSIController::do_hsi_endpoint_enable);
  register_command("hsi_endpoint_disable", &HSIController::do_hsi_endpoint_disable);
  register_command("hsi_endpoint_reset", &HSIController::do_hsi_endpoint_reset);
  register_command("hsi_reset", &HSIController::do_hsi_reset);
  register_command("hsi_configure", &HSIController::do_hsi_configure);
  register_command("hsi_start", &HSIController::do_hsi_start);
  register_command("hsi_stop", &HSIController::do_hsi_stop);
  register_command("hsi_print_status", &HSIController::do_hsi_print_status);
}

void
HSIController::do_configure(const nlohmann::json& data)
{
  m_hsi_configuration = m_params->cast<timinglibs::dal::HSIController>();
  m_hardware_state_recovery_enabled = m_hsi_configuration->get_hardware_state_recovery_enabled();
  m_timing_device = m_hsi_configuration->get_device();
  m_control_hardware_io = m_hsi_configuration->get_control_hardware_io();

  configure_uhal(m_hsi_configuration->get_uhal_config()); // configure hw ipbus connection

  if (m_timing_device.empty())
  {
    throw timinglibs::UHALDeviceNameIssue(ERS_HERE, "Device name for HSIController should not be empty");
  }

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

  do_hsi_reset(data);

  auto start_params = data.get<rcif::cmd::StartParams>();
  if (start_params.trigger_rate > 0)
  {
    TLOG() << get_name() << " Changing rate: trigger_rate "
           << start_params.trigger_rate;  
    do_hsi_configure_trigger_rate_override(data, start_params.trigger_rate);
  }
  else
  {
    TLOG() << get_name() << " Changing rate: trigger_rate "
           << m_hsi_configuration->get_trigger_rate();  
    do_hsi_configure(data);
  }
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
HSIController::do_change_rate(const nlohmann::json& data)
{
  auto change_rate_params = data.get<rcif::cmd::ChangeRateParams>();
  TLOG() << get_name() << " Changing rate: trigger_rate "
         << change_rate_params.trigger_rate;

  do_hsi_configure_trigger_rate_override(data, change_rate_params.trigger_rate);
}

void
HSIController::send_configure_hardware_commands(const nlohmann::json& data)
{
  if (m_control_hardware_io)
  {
    do_hsi_io_reset(data);
  }
  do_hsi_reset(data);
  do_hsi_endpoint_reset(data);
  do_hsi_configure(data);
}

void
HSIController::do_hsi_io_reset(const nlohmann::json& )
{
  auto design = dynamic_cast<const timing::HSIDesignInterface*>(&m_hsi_device->getNode(""));

  design->reset_io();
  ++(m_sent_hw_command_counters.at(0).atomic);
}

void
HSIController::do_hsi_endpoint_enable(const nlohmann::json& data)
{
  timinglibs::timingcmd::TimingEndpointConfigureCmdPayload cmd_payload;
  cmd_payload.endpoint_id = 0;
  timinglibs::timingcmd::from_json(data, cmd_payload);

  TLOG_DEBUG(0) << "ept enable hw cmd; a: " << cmd_payload.address << ", p: " << cmd_payload.partition;

  auto design = dynamic_cast<const timing::HSIDesignInterface*>(&m_hsi_device->getNode(""));
  design->get_endpoint_node_plain(cmd_payload.endpoint_id)->enable(cmd_payload.address, cmd_payload.partition);

  ++(m_sent_hw_command_counters.at(1).atomic);
}

void
HSIController::do_hsi_endpoint_disable(const nlohmann::json& data)
{
  timinglibs::timingcmd::TimingEndpointConfigureCmdPayload cmd_payload;
  cmd_payload.endpoint_id = 0;
  timinglibs::timingcmd::from_json(data, cmd_payload);

  TLOG_DEBUG(0) << "ept disable hw cmd; a: " << cmd_payload.address << ", p: " << cmd_payload.partition;

  auto design = dynamic_cast<const timing::HSIDesignInterface*>(&m_hsi_device->getNode(""));
  design->get_endpoint_node_plain(cmd_payload.endpoint_id)->disable();
  ++(m_sent_hw_command_counters.at(2).atomic);
}

void
HSIController::do_hsi_endpoint_reset(const nlohmann::json& data)
{
  timinglibs::timingcmd::TimingEndpointConfigureCmdPayload cmd_payload;
  cmd_payload.endpoint_id = 0;
  timinglibs::timingcmd::from_json(data, cmd_payload);

  TLOG_DEBUG(0) << "ept reset hw cmd; a: " << cmd_payload.address << ", p: " << cmd_payload.partition;

  auto design = dynamic_cast<const timing::HSIDesignInterface*>(&m_hsi_device->getNode(""));
  design->get_endpoint_node_plain(cmd_payload.endpoint_id)->reset(cmd_payload.address, cmd_payload.partition);
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
HSIController::do_hsi_configure(const nlohmann::json& data)
{
  timinglibs::timingcmd::HSIConfigureCmdPayload cmd_payload;
  timinglibs::timingcmd::from_json(data, cmd_payload);

  if (!data.contains("random_rate"))
  {
    cmd_payload.random_rate = m_hsi_configuration->get_trigger_rate();
  }

  if (cmd_payload.random_rate <= 0) {
    ers::error(timinglibs::InvalidTriggerRateValue(ERS_HERE, cmd_payload.random_rate));
    return;
  }

  TLOG() << get_name() << " Setting emulated event rate [Hz] to: "
         << cmd_payload.random_rate;

  TLOG_DEBUG(0) << get_name() << ": " << m_timing_device << " hsi configure";

  auto design = dynamic_cast<const timing::HSIDesignInterface*>(&m_hsi_device->getNode(""));
  design->configure_hsi(
    cmd_payload.data_source, cmd_payload.rising_edge_mask, cmd_payload.falling_edge_mask, cmd_payload.invert_edge_mask, cmd_payload.random_rate);
  ++(m_sent_hw_command_counters.at(5).atomic);
}

void HSIController::do_hsi_configure_trigger_rate_override(nlohmann::json data, double trigger_rate_override)
{
  data["random_rate"] = trigger_rate_override;
  do_hsi_configure(data);
}

//void
//TimingHardwareManager::hsi_reset(const timinglibs::timingcmd::TimingHwCmd& hw_cmd)
//{
//}

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
  TLOG() << std::endl << design->get_hsi_node().get_status();
  ++(m_sent_hw_command_counters.at(8).atomic);
}

void
HSIController::get_info(opmonlib::InfoCollector& ci, int /*level*/)
{
  // send counters internal to the module
  hsicontrollerinfo::Info module_info;
  module_info.sent_hsi_io_reset_cmds = m_sent_hw_command_counters.at(0).atomic.load();
  module_info.sent_hsi_endpoint_enable_cmds = m_sent_hw_command_counters.at(1).atomic.load();
  module_info.sent_hsi_endpoint_disable_cmds = m_sent_hw_command_counters.at(2).atomic.load();
  module_info.sent_hsi_endpoint_reset_cmds = m_sent_hw_command_counters.at(3).atomic.load();
  module_info.sent_hsi_reset_cmds = m_sent_hw_command_counters.at(4).atomic.load();
  module_info.sent_hsi_configure_cmds = m_sent_hw_command_counters.at(5).atomic.load();
  module_info.sent_hsi_start_cmds = m_sent_hw_command_counters.at(6).atomic.load();
  module_info.sent_hsi_stop_cmds = m_sent_hw_command_counters.at(7).atomic.load();
  module_info.sent_hsi_print_status_cmds = m_sent_hw_command_counters.at(8).atomic.load();
  module_info.device_infos_received_count = m_device_infos_received_count;

  ci.add(module_info);
}

void
HSIController::process_device_info(nlohmann::json info)
{
  ++m_device_infos_received_count;

  timing::timingendpointinfo::TimingEndpointInfo endpoint_info;

  auto endpoint_data = info[opmonlib::JSONTags::children]["endpoint"][opmonlib::JSONTags::properties][endpoint_info.info_type][opmonlib::JSONTags::data];

  from_json(endpoint_data, endpoint_info);

  m_endpoint_state = endpoint_info.state;
  
  TLOG_DEBUG(3) << "HSI ept state: 0x" << std::hex << m_endpoint_state << std::dec << ", infos received: " << m_device_infos_received_count;

  if (m_endpoint_state == 0x8)
  {
    if (!m_device_ready)
    {
      m_device_ready = true;
      TLOG_DEBUG(2) << "HSI endpoint became ready";
    }
  }
  else
  {
    if (m_device_ready)
    {
      m_device_ready = false;
      TLOG_DEBUG(2) << "HSI endpoint no longer ready";
    }
  }
}

void
HSIController::gather_monitor_data(std::atomic<bool>& running_flag)
{
  while (running_flag.load()) {

    opmonlib::InfoCollector info_collector;

    // collect the data from the hardware
    try
    {
      auto design = dynamic_cast<const timing::HSIDesignInterface*>(&m_hsi_device->getNode(""));
      design->get_info(info_collector, 1);
    } catch (const std::exception& excpt) {
      ers::warning(timinglibs::FailedToCollectOpMonInfo(ERS_HERE, m_timing_device, excpt));
    }

    nlohmann::json info = info_collector.get_collected_infos();
    process_device_info(info);

    auto prev_gather_time = std::chrono::steady_clock::now();
    auto next_gather_time = prev_gather_time + std::chrono::microseconds(100000);

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
