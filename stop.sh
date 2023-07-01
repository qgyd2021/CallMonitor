#!/usr/bin/env bash

kill -9 `ps -aef | grep 'CallMonitor' | grep -v grep | awk '{print $2}'`

kill -9 `ps -aef | grep 'for_restart.sh' | grep -v grep | awk '{print $2}'`
