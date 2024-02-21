#!/bin/bash

run_Test_sleep() {
    # capture the start time
    start_time=$(date +%s)

    # run the program with the input redirected from the inp file
    $prg <$t/inp 2>&1 >$t/out

    # capture the end time
    end_time=$(date +%s)

    # calculate the difference in seconds
    seconds_passed=$((end_time - start_time))

    # extract sleep time from inp file
    sec=$(grep -oP '(?<=sleep )\d+' $t/inp | cut -f1 -d' ')

    # compare seconds_passed with the expected value
    # if not equal, redirect error message to out file
    if [ $seconds_passed -ne $sec ]; then
        echo "$t failed: Expected $sec seconds, but got $seconds_passed seconds" 2>&1 >>$t/out
    fi
}