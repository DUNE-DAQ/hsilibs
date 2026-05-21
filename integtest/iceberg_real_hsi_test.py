# *** NB, 24-Sep-2025, KAB:  I have made some minimal changes to get this integtest
# *** running again, at least as far as getting it to complain that the conditions
# *** for the test are not right.  I haven't yet made changes that would get it to
# *** run correctly if the conditions were right (e.g. on an ICEBERG computer, etc.)
# *** Truthfully, I expect there to be work to understand how we will need to control
# *** the timing system electronics (TLU) and whether we need to manually start a
# *** ConnectivityService instance.

import pytest
import os
import re
import psutil
import copy
import urllib.request

import integrationtest.data_file_checks as data_file_checks
import integrationtest.log_file_checks as log_file_checks
import integrationtest.data_classes as data_classes
import integrationtest.utility_functions as utility_functions
from integrationtest.verbosity_helper import IntegtestVerbosityLevels

import functools
print = functools.partial(print, flush=True)  # always flush print() output

pytest_plugins = "integrationtest.integrationtest_drunc"

my_dir = os.path.dirname(os.path.abspath(__file__))

# Values that help determine the running conditions
number_of_data_producers=2
number_of_readout_apps=2
number_of_dataflow_apps=1
base_trigger_rate=1.0 # Hz
trigger_rate_factor=3.5
run_duration=20  # seconds
conn_svc_port=15879

# Default values for validation parameters
expected_number_of_data_files=3*number_of_dataflow_apps
check_for_logfile_errors=True
expected_event_count=run_duration*base_trigger_rate/number_of_dataflow_apps
expected_event_count_tolerance=expected_event_count/10
wibeth_frag_hsi_trig_params={"fragment_type_description": "WIBEth",
                             "fragment_type": "WIBEth",
                             "hdf5_source_subsystem": "Detector_Readout",
                             "expected_fragment_count": (number_of_data_producers*number_of_readout_apps),
                             "min_size_bytes": 7272, "max_size_bytes": 14472}
triggercandidate_frag_params={"fragment_type_description": "Trigger Candidate",
                              "fragment_type": "Trigger_Candidate",
                              "hdf5_source_subsystem": "Trigger",
                              "expected_fragment_count": 1,
                              "min_size_bytes": 120, "max_size_bytes": 150}

# Determine if the conditions are right for these tests
we_are_running_on_an_iceberg_computer=False
hostname=os.uname().nodename
timing_host=""
if "iceberg01" in hostname:
    timing_host="iceberg01-priv"
if "iceberg03" in hostname:
    timing_host="iceberg03"
if "iceberg" in timing_host:
    we_are_running_on_an_iceberg_computer=True
the_global_timing_session_is_running=False
global_timing_session_user="Unknown"
for proc in psutil.process_iter():
    if "nanotimingrc" in proc.name() and "iceberg-integtest-timing-session" in proc.cmdline():
        the_global_timing_session_is_running=True
        global_timing_session_user=proc.username()
try:
  urllib.request.urlopen(f'http://localhost:{conn_svc_port}').status
  the_connection_server_is_running=True
except:
  the_connection_server_is_running=False
if the_global_timing_session_is_running:
    print(f"DEBUG: hostname is {hostname}, iceberg-computer flag is {we_are_running_on_an_iceberg_computer}, global-timing-running flag is {the_global_timing_session_is_running} (as user {global_timing_session_user}), connection-server-running flag is {the_connection_server_is_running}.")
else:
    print(f"DEBUG: hostname is {hostname}, iceberg-computer flag is {we_are_running_on_an_iceberg_computer}, global-timing-running flag is {the_global_timing_session_is_running}, connection-server-running flag is {the_connection_server_is_running}.")


conf_dict = data_classes.integtest_params_for_generated_dunedaq_config()
conf_dict.object_databases = ["config/daqsystemtest/integrationtest-objects.data.xml"]
conf_dict.dro_map_config.n_streams = number_of_data_producers
conf_dict.dro_map_config.n_apps = number_of_readout_apps
conf_dict.op_env = "integtest"
conf_dict.session = "icebergrealhsi"
conf_dict.tpg_enabled = False
conf_dict.use_fakedataprod = True

conf_dict.config_substitutions.append(
    data_classes.attribute_substitution(
        obj_class="LatencyBuffer", updates={"size": 200000}
    )
)

if we_are_running_on_an_iceberg_computer and the_global_timing_session_is_running and the_connection_server_is_running:
    # FIXME / To-do:  24-Sep-2025, KAB, the following config params
    # need to be converted to v5-style.
    #conf_dict["trigger"]["ttcm_s1"] = 128
    #conf_dict["trigger"]["hsi_trigger_type_passthrough"] = True
    #conf_dict["hsi"]["random_trigger_rate_hz"] = base_trigger_rate
    #conf_dict["hsi"]["control_hsi_hw"]= True
    #conf_dict["hsi"]["hsi_device_name"]= "BOREAS_TLU_ICEBERG"
    #conf_dict["hsi"]["hsi_source"] = 1
    #conf_dict["hsi"]["use_timing_hsi"] = True
    #conf_dict["hsi"]["use_fake_hsi"] = False
    #conf_dict["hsi"]["host_timing_hsi"] = timing_host
    #conf_dict["hsi"]["hsi_re_mask"] = 1
    #conf_dict["hsi"]["hsi_hw_connections_file"] = os.path.abspath(f"{my_dir}/../../daqsystemtest/config/timing_systems/connections.xml")
    #conf_dict["timing"]["timing_session_name"] = "iceberg-integtest-timing-session"

    trigger_factor_conf = copy.deepcopy(conf_dict)
    #trigger_factor_conf["hsi"]["random_trigger_rate_hz"] = base_trigger_rate*trigger_rate_factor
    confgen_arguments={"Base_Trigger_Rate": conf_dict,
                       "Trigger_Rate_with_Factor": trigger_factor_conf
                      }
else:
    confgen_arguments={"Invalid test conditions, cannot run test": conf_dict}

# The commands to run in dunerc, as a list
if we_are_running_on_an_iceberg_computer and the_global_timing_session_is_running and the_connection_server_is_running:
    dunerc_command_list="boot conf".split()
    dunerc_command_list+="start 101 enable_triggers wait ".split() + [str(run_duration)] + "stop_run wait 2".split()
    dunerc_command_list+="start 102 wait 1 enable_triggers wait ".split() + [str(run_duration)] + "disable_triggers wait 1 stop_run".split()
    dunerc_command_list+="start_run 103 wait ".split() + [str(run_duration)] + "disable_triggers wait 1 drain_dataflow wait 1 stop_trigger_sources wait 1 stop wait 2".split()
    dunerc_command_list+="scrap terminate".split()
else:
    dunerc_command_list=["wait", "1"]

# The tests themselves

def test_dunerc_success(run_dunerc, caplog):
    if not we_are_running_on_an_iceberg_computer:
        print(f"\n\n\N{LARGE YELLOW CIRCLE} This computer ({hostname}) is not part of the ICEBERG DAQ cluster and therefore can not run this test.")
        pytest.skip(f"This computer ({hostname}) is not part of the ICEBERG DAQ cluster and therefore can not run this test.")
    if not the_global_timing_session_is_running:
        print(f"\n\n\N{LARGE YELLOW CIRCLE} The global timing session does not appear to be running on this computer ({hostname}).")
        print("\N{LARGE YELLOW CIRCLE} Please check whether it is, and start it, if needed.")
        #var1="Hints: echo '{\"boot\": { \"use_connectivity_service\": true, \"start_connectivity_service\": true, \"connectivity_service_port\": 13579 }, \"timing_hardware_interface\": { \"host_thi\": \"" + timing_host + "\", \"firmware_type\": \"pdii\", \"timing_hw_connections_file\": \""
        #var2=os.path.realpath(os.path.dirname(__file__) + "/../../daqsystemtest")
        #var3="/config/timing_systems/connections.xml\" }, \"timing_master_controller\": { \"host_tmc\": \"" + timing_host + "\", \"master_device_name\": \"BOREAS_TLU_ICEBERG\" } }' >> iceberg_integtest_timing_config_input.json"
        #print(f"{var1}{var2}{var3}")
        #print("       daqconf_timing_gen --config ./iceberg_integtest_timing_config_input.json iceberg_integtest_timing_session_config")
        #print("       nanotimingrc --partition-number 4 iceberg_integtest_timing_session_config iceberg-integtest-timing-session boot conf wait 1200 scrap terminate")
        pytest.skip("The global timing session is not running.")
    if not the_connection_server_is_running:
        print("\n\n\N{LARGE YELLOW CIRCLE} The connectivity service must be running for this test. Please confirm that it is being started as part of the timing session for this test.")
        pytest.skip(f"The connectivity service must be running for this test.")

    # check for run control success, problems during pytest setup, etc.
    utility_functions.basic_checks(run_dunerc, caplog, print_test_name=True)

def test_log_files(run_dunerc):
    if not we_are_running_on_an_iceberg_computer:
        pytest.skip(f"This computer ({hostname}) is not part of the ICEBERG DAQ cluster and therefore can not run this test.")
    if not the_global_timing_session_is_running:
        pytest.skip("The global timing session is not running.")
    if not the_connection_server_is_running:
        pytest.skip(f"The connectivity service must be running for this test.")

    if check_for_logfile_errors:
        # Check that there are no warnings or errors in the log files
        assert log_file_checks.logs_are_error_free(run_dunerc.log_files, True, True,
                                                   verbosity_helper=run_dunerc.verbosity_helper)

def test_data_files(run_dunerc):
    if not we_are_running_on_an_iceberg_computer:
        pytest.skip(f"This computer ({hostname}) is not part of the ICEBERG DAQ cluster and therefore can not run this test.")
    if not the_global_timing_session_is_running:
        pytest.skip("The global timing session is not running.")
    if not the_connection_server_is_running:
        pytest.skip(f"The connectivity service must be running for this test.")

    fragment_check_list=[]
    fragment_check_list.append(wibeth_frag_hsi_trig_params)
    fragment_check_list.append(triggercandidate_frag_params)

    local_expected_event_count=expected_event_count
    local_event_count_tolerance=expected_event_count_tolerance
    current_test=os.environ.get('PYTEST_CURRENT_TEST')
    match_obj = re.search(r"Factor", current_test)
    if match_obj:
        local_expected_event_count*=trigger_rate_factor
        local_event_count_tolerance*=trigger_rate_factor

    # Run some tests on the output data files
    assert len(run_dunerc.data_files)==expected_number_of_data_files

    for idx in range(len(run_dunerc.data_files)):
        data_file=data_file_checks.DataFile(run_dunerc.data_files[idx], run_dunerc.verbosity_helper)
        assert data_file_checks.sanity_check(data_file)
        assert data_file_checks.check_file_attributes(data_file)
        assert data_file_checks.check_event_count(data_file, local_expected_event_count, local_event_count_tolerance)
        for jdx in range(len(fragment_check_list)):
            assert data_file_checks.check_fragment_count(data_file, fragment_check_list[jdx])
            assert data_file_checks.check_fragment_sizes(data_file, fragment_check_list[jdx])

# ### also test the expected trigger bit ###


def test_cleanup(run_dunerc):
    if not we_are_running_on_an_iceberg_computer:
        pytest.skip(f"This computer ({hostname}) is not part of the ICEBERG DAQ cluster and therefore can not run this test.")
    if not the_global_timing_session_is_running:
        pytest.skip("The global timing session is not running.")
    if not the_connection_server_is_running:
        pytest.skip(f"The connectivity service must be running for this test.")

    utility_functions.remove_hdf5_files_if_requested(run_dunerc, this_test_requests_hdf5_file_removal=False)
