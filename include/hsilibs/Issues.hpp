/**
 * @file Issues.hpp
 *
 * This file contains the definitions of ERS Issues that are common
 * to two or more of the DAQModules in this package.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef HSILIBS_INCLUDE_HSILIBS_ISSUES_HPP_
#define HSILIBS_INCLUDE_HSILIBS_ISSUES_HPP_

#include "appfwk/DAQModule.hpp"
#include "ers/Issue.hpp"

#include <string>

// NOLINTNEXTLINE(build/define_used)
#define TLVL_ENTER_EXIT_METHODS 10

namespace dunedaq {

ERS_DECLARE_ISSUE_BASE(hsilibs,
                       ProgressUpdate,
                       appfwk::GeneralDAQModuleIssue,
                       message,
                       ((std::string)name),
                       ((std::string)message))

ERS_DECLARE_ISSUE_BASE(hsilibs,
                       InvalidQueueFatalError,
                       appfwk::GeneralDAQModuleIssue,
                       "The " << queueType << " queue was not successfully created.",
                       ((std::string)name),
                       ((std::string)queueType))

ERS_DECLARE_ISSUE_BASE(hsilibs,
                       SenderReadyTimeout,
                       appfwk::GeneralDAQModuleIssue,
                       "Timeout waiting for the Sender for connection " << conn << " to be ready.",
                       ((std::string)name),
                       ((std::string)conn))

ERS_DECLARE_ISSUE(hsilibs,                         ///< Namespace
                  UHALIssue,                          ///< Issue class name
                  " UHAL related issue: " << message, ///< Message
                  ((std::string)message)              ///< Message parameters
)

ERS_DECLARE_ISSUE_BASE(hsilibs,
                       UHALConnectionsFileIssue,
                       hsilibs::UHALIssue,
                       " UHAL connections file issue: " << message,
                       ((std::string)message),
                       ERS_EMPTY)

ERS_DECLARE_ISSUE_BASE(hsilibs,
                       InvalidUHALLogLevel,
                       hsilibs::UHALIssue,
                       " Invalid UHAL log level supplied: " << log_level,
                       ((std::string)log_level),
                       ERS_EMPTY)

ERS_DECLARE_ISSUE_BASE(hsilibs,
                       UHALDeviceNameIssue,
                       hsilibs::UHALIssue,
                       " UHAL device name issue: " << message,
                       ((std::string)message),
                       ERS_EMPTY)

ERS_DECLARE_ISSUE_BASE(hsilibs,
                       UHALDeviceNodeIssue,
                       hsilibs::UHALIssue,
                       " UHAL node issue: " << message,
                       ((std::string)message),
                       ERS_EMPTY)

ERS_DECLARE_ISSUE_BASE(hsilibs,
                       UHALDeviceClassIssue,
                       hsilibs::UHALDeviceNodeIssue,
                       " Failed to cast device " << device << " to type " << type << " where actual_type type is " << actual_type,
                       ((std::string)message),
                       ((std::string)device)((std::string)type)((std::string)actual_type))

ERS_DECLARE_ISSUE(hsilibs,
                  FailedToCollectOpMonInfo,
                  " Failed to collect op mon info from device: " << device_name,
                  ((std::string)device_name))

ERS_DECLARE_ISSUE(hsilibs, HardwareCommandIssue, " Issue wih hw cmd id: " << hw_cmd_id, ((std::string)hw_cmd_id))

ERS_DECLARE_ISSUE(hsilibs, HSIBufferIssue, "HSI buffer in state: " << buffer_state, ((std::string)buffer_state))

ERS_DECLARE_ISSUE(hsilibs, HSIReadoutIssue, "Failed to read HSI events.", ERS_EMPTY)

ERS_DECLARE_ISSUE_BASE(hsilibs,
                       HSIReadoutNetworkIssue,
                       hsilibs::HSIReadoutIssue,
                       "Failed to read HSI events due to network issue.",
                       ERS_EMPTY,
                       ERS_EMPTY)

ERS_DECLARE_ISSUE(hsilibs,
                  InvalidTriggerRateValue,
                  " Trigger rate value " << trigger_rate << " invalid!",
                  ((uint64_t)trigger_rate)) // NOLINT(build/unsigned)

ERS_DECLARE_ISSUE_BASE(hsilibs,
                       QueueIsNullFatalError,
                       appfwk::GeneralDAQModuleIssue,
                       " Queue " << queue << " is null! ",
                       ((std::string)name),
                       ((std::string)queue))
} // namespace dunedaq

#endif // HSILIBS_INCLUDE_HSILIBS_ISSUES_HPP_
