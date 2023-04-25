cmake_minimum_required(VERSION 3.12)
project(hsilibs VERSION 2.1.0)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(ers REQUIRED)     
find_package(logging REQUIRED)
find_package(pybind11 REQUIRED)
find_package(cmdlib REQUIRED)
find_package(rcif REQUIRED)
find_package(appfwk REQUIRED)
find_package(timing REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(dfmessages REQUIRED)
find_package(readoutlibs REQUIRED)
find_package(timinglibs REQUIRED)
find_package(opmonlib REQUIRED)
find_package(uhal REQUIRED)
find_package(pugixml REQUIRED)
find_package(iomanager REQUIRED)
find_package(daqdataformats REQUIRED)
find_package(detdataformats REQUIRED)
find_package(Boost COMPONENTS unit_test_framework iostreams REQUIRED)

set(BOOST_LIBS Boost::iostreams ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_LIBRARIES})

daq_codegen( 
			 fakehsieventgenerator.jsonnet
			 hsicontroller.jsonnet
			 hsireadout.jsonnet
			 DEP_PKGS appfwk rcif cmdlib iomanager TEMPLATES Structs.hpp.j2 Nljs.hpp.j2 )

daq_codegen( 
       fakehsieventgeneratorinfo.jsonnet 
			 hsicontrollerinfo.jsonnet 
			 hsireadoutinfo.jsonnet 
			 DEP_PKGS opmonlib TEMPLATES opmonlib/InfoStructs.hpp.j2 opmonlib/InfoNljs.hpp.j2 )

##############################################################################
# Dependency sets
set(HSILIBS_DEPENDENCIES 
  appfwk::appfwk
  iomanager::iomanager
  ers::ers
  logging::logging
  iomanager::iomanager
  opmonlib::opmonlib
  nlohmann_json::nlohmann_json
  dfmessages::dfmessages
  readoutlibs::readoutlibs
  daqdataformats::daqdataformats
  detdataformats::detdataformats
)

##############################################################################
daq_add_library(HSIEventSender.cpp HSIFrameProcessor.cpp LINK_LIBRARIES ${HSILIBS_DEPENDENCIES})

##############################################################################
daq_add_plugin(HSIDataLinkHandler duneDAQModule LINK_LIBRARIES hsilibs readoutlibs::readoutlibs ${BOOST_LIBS})
daq_add_plugin(FakeHSIEventGenerator duneDAQModule LINK_LIBRARIES hsilibs timinglibs::timinglibs timing::timing)
daq_add_plugin(HSIReadout duneDAQModule LINK_LIBRARIES timing::timing timinglibs::timinglibs uhal::uhal pugixml::pugixml hsilibs)
daq_add_plugin(HSIController duneDAQModule LINK_LIBRARIES hsilibs timing::timing timinglibs::timinglibs)

##############################################################################

##############################################################################
daq_install()
