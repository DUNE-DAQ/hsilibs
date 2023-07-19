* 19-Jul-2023, KAB, ELF, and others: notes on existing integtests

"integtests" are intended to be automated integration and/or system tests that make use of the
"pytest" framework to validate the operation of the DAQ system in various scenarios.

Here is a sample command for invoking a test (feel free to keep or drop the options in brackets, as you prefer):

```
pytest -s disabled_output_test.py [--nanorc-option partition-number 2] [--nanorc-option timeout 300]
```
For reference, here are the ideas behind the tests that currently exist in this repository:
* `iceberg_real_hsi_test.py` - tests the generation of pulser triggers by the real TLU/HSI electronics at the ICEBERG teststand
    * needs to be run on the the iceberg03 computer at the ICEBERG teststand
    * it needs the global timing partition to be started separately (hints provided in output of the test script)
    * it is useful to run this test with a couple of partition numbers to verify that it can talk to the global timing partition independent of its own partition number
