#!/bin/bash

set -e

cat /opt/redpitaya/fpga/fpga_0.94.bit > /dev/xdevcfg

LD_LIBRARY_PATH=/opt/redpitaya/lib /opt/rp-trigger-recorder/bin/rp-trigger-recorder --config=/etc/rp-trigger-recorder.conf --silent
