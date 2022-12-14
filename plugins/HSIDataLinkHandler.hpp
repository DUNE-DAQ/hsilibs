/**
 * @file HSIDataLinkHandler.hpp Module implementing 
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
#include "readoutlibs/concepts/ReadoutConcept.hpp"

#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <atomic>

namespace dunedaq {
namespace hsilibs {

class HSIDataLinkHandler : public dunedaq::appfwk::DAQModule
{
public:
  /**
   * @brief HSIDataLinkHandler Constructor
   * @param name Instance name for this HSIDataLinkHandler instance
   */
  explicit HSIDataLinkHandler(const std::string& name);

  HSIDataLinkHandler(const HSIDataLinkHandler&) = delete;            ///< HSIDataLinkHandler is not copy-constructible
  HSIDataLinkHandler& operator=(const HSIDataLinkHandler&) = delete; ///< HSIDataLinkHandler is not copy-assignable
  HSIDataLinkHandler(HSIDataLinkHandler&&) = delete;                 ///< HSIDataLinkHandler is not move-constructible
  HSIDataLinkHandler& operator=(HSIDataLinkHandler&&) = delete;      ///< HSIDataLinkHandler is not move-assignable

  void init(const data_t& args) override;
  void get_info(opmonlib::InfoCollector& ci, int level) override;

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
  std::unique_ptr<readoutlibs::ReadoutConcept> m_readout_impl;

  // Threading
  std::atomic<bool> m_run_marker;
};

} // namespace hsilibs
} // namespace dunedaq

#endif // HSILIBS_PLUGINS_HSIDATALINKHANDLER_HPP_
