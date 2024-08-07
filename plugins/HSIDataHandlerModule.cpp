/**
 * @file HSIDataHandlerModule.cpp HSIDataHandlerModule class implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "HSIDataHandlerModule.hpp"

#include "hsilibs/Types.hpp"
#include "HSIFrameProcessor.hpp"

#include "datahandlinglibs/concepts/DataHandlingConcept.hpp"
#include "datahandlinglibs/models/DataHandlingModel.hpp"
#include "datahandlinglibs/models/BinarySearchQueueModel.hpp"
#include "datahandlinglibs/models/DefaultRequestHandlerModel.hpp"
#include "datahandlinglibs/ReadoutLogging.hpp"

#include "appfwk/cmd/Nljs.hpp"
#include "logging/Logging.hpp"
#include "rcif/cmd/Nljs.hpp"

#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace dunedaq::datahandlinglibs::logging;

namespace dunedaq {
namespace hsilibs {

HSIDataHandlerModule::HSIDataHandlerModule(const std::string& name)
  : DAQModule(name)
  , m_configured(false)
  , m_readout_impl(nullptr)
  , m_run_marker{ false }
{
  register_command("conf", &HSIDataHandlerModule::do_conf);
  register_command("scrap", &HSIDataHandlerModule::do_scrap);
  register_command("start", &HSIDataHandlerModule::do_start);
  register_command("stop_trigger_sources", &HSIDataHandlerModule::do_stop);
  register_command("record", &HSIDataHandlerModule::do_record);
}

void
HSIDataHandlerModule::init(std::shared_ptr<appfwk::ModuleConfiguration> mcfg)
{

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";
  
  namespace rol = dunedaq::datahandlinglibs;

  m_readout_impl = std::make_unique<rol::DataHandlingModel<
                    hsilibs::HSI_FRAME_STRUCT,
                    rol::DefaultRequestHandlerModel<hsilibs::HSI_FRAME_STRUCT, rol::BinarySearchQueueModel<hsilibs::HSI_FRAME_STRUCT>>,
                    rol::BinarySearchQueueModel<hsilibs::HSI_FRAME_STRUCT>,
                    hsilibs::HSIFrameProcessor>>(m_run_marker);
  m_readout_impl->init(mcfg->module<appmodel::DataHandlerModule>(get_name()));
  if (m_readout_impl == nullptr)
  {
    TLOG() << get_name() << "Initialize HSIDataHandlerModule FAILED! ";
    throw datahandlinglibs::FailedReadoutInitialization(ERS_HERE, get_name(), "OKS Config"); // 4 json ident
  }
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
}

// void
// HSIDataHandlerModule::get_info(opmonlib::InfoCollector& ci, int level)
// {
//   m_readout_impl->get_info(ci, level);
// }

void
HSIDataHandlerModule::do_conf(const data_t& args)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_conf() method";
  m_readout_impl->conf(args);
  m_configured = true;
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_conf() method";
}

void
HSIDataHandlerModule::do_scrap(const data_t& args)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_scrap() method";
  m_readout_impl->scrap(args);
  m_configured = false;
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_scrap() method";
}
void
HSIDataHandlerModule::do_start(const data_t& args)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_start() method";
  m_run_marker.store(true);
  m_readout_impl->start(args);

  rcif::cmd::StartParams start_params = args.get<rcif::cmd::StartParams>();
  m_run_number = start_params.run;
  TLOG() << get_name() << " successfully started for run number " << m_run_number;

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_start() method";
}

void
HSIDataHandlerModule::do_stop(const data_t& args)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_stop() method";
  m_run_marker.store(false);
  m_readout_impl->stop(args);
  TLOG() << get_name() << " successfully stopped for run number " << m_run_number;
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_stop() method";
}

void
HSIDataHandlerModule::do_record(const data_t& args)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_issue_recording() method";
  m_readout_impl->record(args);
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_issue_recording() method";
}

} // namespace hsilibs
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::hsilibs::HSIDataHandlerModule)
