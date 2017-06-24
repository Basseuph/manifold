#!/bin/bash

ls -l

./systemc_test

i=0

# dummy loop to keep the emulation running and to prevent it crashing
while true; do
  sleep 10
  date
done
