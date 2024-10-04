cmake_minimum_required(VERSION 3.12)
project(hsilibs VERSION 4.3.0)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(confmodel REQUIRED)
find_package(ers REQUIRED)     
find_package(logging REQUIRED)
find_package(pybind11 REQUIRED)
find_package(cmdlib REQUIRED)
find_package(rcif REQUIRED)
find_package(appfwk REQUIRED)
find_package(timing REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(dfmessages REQUIRED)
find_package(datahandlinglibs REQUIRED)
find_package(timinglibs REQUIRED)
find_package(opmonlib REQUIRED)
find_package(uhal REQUIRED)
find_package(pugixml REQUIRED)
find_package(iomanager REQUIRED)
find_package(daqdataformats REQUIRED)
find_package(detdataformats REQUIRED)
find_package(utilities REQUIRED)
find_package(Boost COMPONENTS unit_test_framework iostreams REQUIRED)
find_package(okssystem REQUIRED)
find_package(oks REQUIRED)
find_package(oksdalgen REQUIRED)
find_package(conffwk REQUIRED)

daq_oks_codegen(hsi.schema.xml NAMESPACE dunedaq::hsilibs::dal DALDIR dal DEP_PKGS timinglibs confmodel)

set(BOOST_LIBS Boost::iostreams ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_LIBRARIES})

##############################################################################
# Dependency sets
set(HSILIBS_DEPENDENCIES 
  appfwk::appfwk
  iomanager::iomanager
  ers::ers
  logging::logging
  opmonlib::opmonlib
  nlohmann_json::nlohmann_json
  dfmessages::dfmessages
  datahandlinglibs::datahandlinglibs
  daqdataformats::daqdataformats
  detdataformats::detdataformats
  timinglibs::timinglibs
)

##############################################################################
daq_add_library(HSIEventSender.cpp HSIFrameProcessor.cpp LINK_LIBRARIES ${HSILIBS_DEPENDENCIES})

##############################################################################
daq_add_plugin(HSIDataHandlerModule duneDAQModule LINK_LIBRARIES hsilibs datahandlinglibs::datahandlinglibs ${BOOST_LIBS})
daq_add_plugin(FakeHSIEventGeneratorModule duneDAQModule LINK_LIBRARIES hsilibs timing::timing utilities::utilities)
#daq_add_plugin(HSIReadout duneDAQModule LINK_LIBRARIES timing::timing timinglibs::timinglibs hsilibs)
daq_add_plugin(HSIController duneDAQModule LINK_LIBRARIES hsilibs timing::timing timinglibs::timinglibs)

##############################################################################

##############################################################################
daq_install()
