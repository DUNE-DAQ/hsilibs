* 19-Jul-2023, KAB, ELF, and others: notes on existing integtests

"integtests" are intended to be automated integration and/or system tests that make use of the
"pytest" framework to validate the operation of the DAQ system in various scenarios.

Here is a sample command for invoking a test (feel free to keep or drop the options in brackets, as you prefer):

```
pytest -s iceberg_real_hsi_test.py [--nanorc-option partition-number 2] [--nanorc-option timeout 300]
```
For reference, here are the ideas behind the tests that currently exist in this repository:
* `iceberg_real_hsi_test.py` - tests the generation of pulser triggers by the real TLU/HSI electronics at the ICEBERG teststand
    * this test needs to be run on a computer at the ICEBERG teststand (iceberg03 or iceberg01)
    * the `daqsystemtest` repository needs to be cloned into the local software area along with the hsilibs repo
    * it needs the global timing partition to be started separately (hints provided in output of the test script)
    * it is useful to run this test with a couple of partition numbers to verify that it can talk to the global timing partition independent of its own partition number
