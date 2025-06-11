#!/bin/bash

me=$(basename "$0")

expc_script=$(pwd)/tests/bin/expc_$me

BIN=$TARGET $expc_script
