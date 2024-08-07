/**
 * @file HSIDataHandlerModule.hpp Module implementing 
 * DataLinkHandlerConcept for HSI.
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef HSILIBS_PLUGINS_HSIDATALINKHANDLER_HPP_
#define HSILIBS_PLUGINS_HSIDATALINKHANDLER_HPP_

#include "appfwk/DAQModule.hpp"
#include "daqdataformats/Types.hpp"
#include "datahandlinglibs/concepts/DataHandlingConcept.hpp"

#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <atomic>

namespace dunedaq {
namespace hsilibs {

class HSIDataHandlerModule : public dunedaq::appfwk::DAQModule
{
public:
  /**
   * @brief HSIDataHandlerModule Constructor
   * @param name Instance name for this HSIDataHandlerModule instance
   */
  explicit HSIDataHandlerModule(const std::string& name);

  HSIDataHandlerModule(const HSIDataHandlerModule&) = delete;            ///< HSIDataHandlerModule is not copy-constructible
  HSIDataHandlerModule& operator=(const HSIDataHandlerModule&) = delete; ///< HSIDataHandlerModule is not copy-assignable
  HSIDataHandlerModule(HSIDataHandlerModule&&) = delete;                 ///< HSIDataHandlerModule is not move-constructible
  HSIDataHandlerModule& operator=(HSIDataHandlerModule&&) = delete;      ///< HSIDataHandlerModule is not move-assignable

  void init(std::shared_ptr<appfwk::ModuleConfiguration> mcfg) override;
  //  void get_info(opmonlib::InfoCollector& ci, int level) override;

private:
  // Commands
  void do_conf(const data_t& /*args*/);
  void do_scrap(const data_t& /*args*/);
  void do_start(const data_t& /*args*/);
  void do_stop(const data_t& /*args*/);
  void do_record(const data_t& /*args*/);

  // Configuration
  bool m_configured;
  daqdataformats::run_number_t m_run_number;

  // Internal
  std::unique_ptr<datahandlinglibs::DataHandlingConcept> m_readout_impl;

  // Threading
  std::atomic<bool> m_run_marker;
};

} // namespace hsilibs
} // namespace dunedaq

#endif // HSILIBS_PLUGINS_HSIDATALINKHANDLER_HPP_
