/**
 * @file HSIReadout.cpp HSIReadout class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "HSIReadout.hpp"

#include "timing/TimingIssues.hpp"
#include "timing/HSIDesignInterface.hpp"

#include "logging/Logging.hpp"
#include "rcif/cmd/Nljs.hpp"
#include "confmodel/DaqModule.hpp"
#include "confmodel/Connection.hpp"
#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace dunedaq {
  ERS_DECLARE_ISSUE(hsilibs, InvalidHSIEventHeader, " Invalid hsi buffer event header: 0x" << std::hex << header, ((uint32_t)header)) // NOLINT(build/unsigned)
  ERS_DECLARE_ISSUE(hsilibs, InvalidHSIEventTimestamp, " Invalid hsi buffer event timestamp: 0x" << std::hex << timestamp, ((uint64_t)timestamp)) // NOLINT(build/unsigned)
  ERS_DECLARE_ISSUE(hsilibs, InvalidNumberReadoutHSIWords, " Invalid number of hsi words readout from buffer: 0x" << std::hex << n_words, ((uint16_t)n_words)) // NOLINT(build/unsigned)
namespace hsilibs {

HSIReadout::HSIReadout(const std::string& name)
  : HSIEventSender(name)
  , m_thread(std::bind(&HSIReadout::do_hsi_work, this, std::placeholders::_1))
  , m_readout_period(1000)
  , m_hsi_device(nullptr)
  , m_readout_counter(0)
  , m_last_readout_timestamp(0)

{
  register_command("conf", &HSIReadout::do_configure);
  register_command("start", &HSIReadout::do_start);
  register_command("stop", &HSIReadout::do_stop);
  register_command("scrap", &HSIReadout::do_scrap);
}

void
HSIReadout::init(std::shared_ptr<appfwk::ModuleConfiguration> mcfg)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";
  HSIEventSender::init(mcfg);
  auto mdal = mcfg->module<appmodel::HSIReadout>(get_name());

  for (auto con : mdal->get_outputs()) {
    if (con->get_data_type() == datatype_to_string<HSI_FRAME_STRUCT>()) {

      m_raw_hsi_data_sender = get_iom_sender<HSI_FRAME_STRUCT>(con->UID());
  }
  }
  m_params = mdal->get_configuration();

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
}

void
HSIReadout::do_configure(const nlohmann::json& /*data*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_configure() method";

  m_readout_period = m_params->get_readout_period();

  configure_uhal(m_params->get_uhal_log_level(), m_params->get_connections_file()); // configure hw ipbus connection

  if (m_params->get_hsi_device_name().empty())
  {
    throw UHALDeviceNameIssue(ERS_HERE, "Device name for HSIReadout should not be empty");
  }
  m_hsi_device_name = m_params->get_hsi_device_name();

  try {
    m_hsi_device = std::make_unique<uhal::HwInterface>(m_connection_manager->getDevice(m_hsi_device_name));
  } catch (const uhal::exception::ConnectionUIDDoesNotExist& exception) {
    std::stringstream message;
    message << "UHAL device name not " << m_hsi_device_name << " in connections file";
    throw UHALDeviceNameIssue(ERS_HERE, message.str(), exception);
  }

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_configure() method";
}

void
HSIReadout::do_start(const nlohmann::json& data)
{
  TLOG() << get_name() << ": Entering do_start() method";
  auto start_params = data.get<rcif::cmd::StartParams>();
  m_run_number.store(start_params.run);
  m_thread.start_working_thread("read-hsi-events");
  TLOG() << get_name() << " successfully started";
  TLOG() << get_name() << ": Exiting do_start() method";
}

void
HSIReadout::do_stop(const nlohmann::json& /*data*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_stop() method";
  m_thread.stop_working_thread();
  TLOG() << get_name() << " successfully stopped";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_stop() method";
}

void
HSIReadout::do_scrap(const nlohmann::json& /*data*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_scrap() method";
  scrap_uhal();
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_scrap() method";
}

void
HSIReadout::do_hsi_work(std::atomic<bool>& running_flag)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_hsievent_work() method";

  m_readout_counter = 0;
  m_sent_counter = 0;
  m_failed_to_send_counter = 0;

  m_last_readout_timestamp = 0;
  m_last_sent_timestamp = 0;

  auto hsi_design = dynamic_cast<const timing::HSIDesignInterface*> (&m_hsi_device->getNode(""));
  auto hsi_node = hsi_design->get_hsi_node();
  auto ept_node = hsi_design->get_endpoint_node_plain(0);

  while (running_flag.load()) {
        
    // endpoint should be ready if already running
    auto hsi_endpoint_ready = ept_node->endpoint_ready();
    if (!hsi_endpoint_ready)
    {
      auto hsi_endpoint_state = ept_node->read_endpoint_state();
      ers::error(timing::EndpointNotReady(ERS_HERE, "HSI", hsi_endpoint_state));
    }
    
    auto hsi_emulation_mode = hsi_node.read_signal_source_mode();

    uhal::ValVector<uint32_t> hsi_words;
    try
    {
      uint16_t n_words_in_buffer; // NOLINT(build/unsigned)

      hsi_words = hsi_node.read_data_buffer(n_words_in_buffer, false, true);
      update_buffer_counts(n_words_in_buffer);
      TLOG_DEBUG(5) << get_name() << ": Number of words in HSI buffer: " << n_words_in_buffer;
    }
    catch (const uhal::exception::UdpTimeout& excpt)
    {
      ers::error(HSIReadoutNetworkIssue(ERS_HERE, excpt));
      std::this_thread::sleep_for(std::chrono::microseconds(m_readout_period));
      continue;
    }
    
    constexpr size_t n_words_per_hsi_buffer_event = timing::HSINode::hsi_buffer_event_words_number;
    // one or more complete events
    if (hsi_words.size() % n_words_per_hsi_buffer_event == 0 && hsi_words.size() > 0)
    { 
      uint n_hsi_events = hsi_words.size() / n_words_per_hsi_buffer_event;

      TLOG_DEBUG(4) << get_name() << ": Have readout " << n_hsi_events << " HSIEvent(s) ";

      m_readout_counter.store(m_readout_counter.load() + n_hsi_events);
      for (uint i = 0; i < n_hsi_events; ++i)
      {
        std::array<uint32_t, n_words_per_hsi_buffer_event> raw_event;

        auto event_start = hsi_words.begin() + (i * n_words_per_hsi_buffer_event);
        std::copy_n(event_start, n_words_per_hsi_buffer_event, raw_event.begin());

        uint32_t header = raw_event.at(0);  // NOLINT(build/unsigned)
        uint32_t ts_low = raw_event.at(1);  // NOLINT(build/unsigned)
        uint32_t ts_high = raw_event.at(2); // NOLINT(build/unsigned)
        uint32_t data = raw_event.at(3);    // NOLINT(build/unsigned)
        uint32_t trigger = raw_event.at(4); // NOLINT(build/unsigned)

        // put together the timestamp
        uint64_t ts = ts_low | (static_cast<uint64_t>(ts_high) << 32); // NOLINT(build/unsigned)

        // bits 31-16 contain the HSI device ID
        uint32_t hsi_device_id = header >> 16; // NOLINT(build/unsigned)
        // bits 15-0 contain the sequence counter
        uint32_t counter = header & 0x0000ffff; // NOLINT(build/unsigned)

        if ((header >> 16) != 0xaa00) {
          ers::error(InvalidHSIEventHeader(ERS_HERE,header));
          continue;
        }

        if (!ts)
        {
          ers::warning(InvalidHSIEventTimestamp(ERS_HERE,ts));
          continue;
        }

        if (counter > 0 && counter % 60000 == 0)
        {
          TLOG_DEBUG(3) << "Sequence counter from firmware: " << counter;
        }

        TLOG_DEBUG(3) << get_name() << ": read out data: " << std::showbase << std::hex << header << ", " << ts
                      << ", " << data << ", " << std::bitset<32>(trigger) << ", "
                      << "ts: " << ts << "\n";
                  
        // In lieu of propper HSI channel to signal mapping, fake signal map when HSI firmware+hardware is in emulation mode.
        // TODO DAQ/HSI team 24/03/22 Put in place HSI channel to signal mapping.

        if (hsi_emulation_mode)
        {
          TLOG_DEBUG(3) << " HSI hardware is in emulation mode, faking (overwriting) signal map from firmware+hardware to have (only) bit 7 high.";
          trigger = 1UL << 7;
        }
        
        dfmessages::HSIEvent event = dfmessages::HSIEvent(hsi_device_id, trigger, ts, counter, m_run_number);
          
        m_last_readout_timestamp.store(ts);

        send_hsi_event(event);

        // Send raw HSI data to a DLH 
        std::array<uint32_t, 7> hsi_struct;
        hsi_struct[0] = (0x1 << 6) | 0x1; // DAQHeader, frame version: 1, det id: 1
        hsi_struct[1] = ts_low;
        hsi_struct[2] = ts_high;
        hsi_struct[3] = data;
        hsi_struct[4] = 0x0;
        hsi_struct[5] = trigger;
        hsi_struct[6] = counter;

        TLOG_DEBUG(3) << get_name() << ": Formed HSI_FRAME_STRUCT "
              << std::hex 
              << "0x"   << hsi_struct[0]
              << ", 0x" << hsi_struct[1]
              << ", 0x" << hsi_struct[2]
              << ", 0x" << hsi_struct[3]
              << ", 0x" << hsi_struct[4]
              << ", 0x" << hsi_struct[5]
              << ", 0x" << hsi_struct[6]
              << "\n";

        send_raw_hsi_data(hsi_struct, m_raw_hsi_data_sender.get());
      }
    }
    // empty buffer is ok
    else if (hsi_words.size() == 0)
    {
      TLOG_DEBUG(20) << "Empty HSI buffter";
    }
    // anything else is unexpected
    else
    {
      ers::error(InvalidNumberReadoutHSIWords(ERS_HERE, hsi_words.size()));
    }
    std::this_thread::sleep_for(std::chrono::microseconds(m_readout_period));
  }
  std::ostringstream oss_summ;
  oss_summ << ": Exiting the read_hsievents() method, read out " << m_readout_counter.load()
           << " HSIEvent messages and successfully sent " << m_sent_counter.load() << " copies. ";
  ers::info(hsilibs::ProgressUpdate(ERS_HERE, get_name(), oss_summ.str()));
  TLOG_DEBUG(2) << get_name() << ": Exiting do_work() method";
}

void
HSIReadout::update_buffer_counts(uint16_t new_count) // NOLINT(build/unsigned)
{
  std::unique_lock mon_data_lock(m_buffer_counts_mutex);
  if (m_buffer_counts.size() > 1000)
    m_buffer_counts.pop_front();
  m_buffer_counts.push_back(new_count);
}

double
HSIReadout::read_average_buffer_counts()
{
  std::unique_lock mon_data_lock(m_buffer_counts_mutex);

  double total_counts;
  uint32_t number_of_counts; // NOLINT(build/unsigned)

  total_counts = 0;
  number_of_counts = m_buffer_counts.size();

  if (number_of_counts) {
    for (uint i = 0; i < number_of_counts; ++i) { // NOLINT(build/unsigned)
      total_counts = total_counts + m_buffer_counts.at(i);
    }
    return total_counts / number_of_counts;
  } else {
    return 0;
  }
}

// void
// HSIReadout::get_info(opmonlib::InfoCollector& ci, int /*level*/)
// {
//   // send counters internal to the module
//   hsireadoutinfo::Info module_info;

//   module_info.readout_hsi_events_counter = m_readout_counter.load();
//   module_info.sent_hsi_events_counter = m_sent_counter.load();
//   module_info.failed_to_send_hsi_events_counter = m_failed_to_send_counter.load();

//   module_info.last_readout_timestamp = m_last_readout_timestamp.load();
//   module_info.last_sent_timestamp = m_last_sent_timestamp.load();

//   module_info.average_buffer_occupancy = read_average_buffer_counts();

//   ci.add(module_info);
// }

} // namespace hsilibs
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::hsilibs::HSIReadout)

// Local Variables:
// c-basic-offset: 2
// End:
