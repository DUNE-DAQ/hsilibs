/**
 * @file HSITypeAdapters_test.cxx
 *
 * Unittest for testing lower bound and request handling on HSI_FRAME_STRUCT
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2022.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#define BOOST_TEST_MODULE HSITypeAdapters_test // NOLINT

#include "hsilibs/Types.hpp"

#include "datahandlinglibs/testutils/TestUtilities.hpp"
#include "datahandlinglibs/models/BinarySearchQueueModel.hpp"

#include "boost/test/unit_test.hpp"

BOOST_AUTO_TEST_SUITE(HSITypeAdapters_test)

BOOST_AUTO_TEST_CASE(TBinarySearchQueueModel_HSI_FRAME_STRUCT_TestQueue)
{
    dunedaq::datahandlinglibs::test::test_queue_model<
            dunedaq::datahandlinglibs::BinarySearchQueueModel,
            dunedaq::hsilibs::HSI_FRAME_STRUCT>();
}
BOOST_AUTO_TEST_CASE(TBinarySearchQueueModel_HSI_FRAME_STRUCT_TestRequest)
{
    dunedaq::datahandlinglibs::test::test_request_model<
            dunedaq::datahandlinglibs::BinarySearchQueueModel,
            dunedaq::hsilibs::HSI_FRAME_STRUCT>();
}
BOOST_AUTO_TEST_SUITE_END()


