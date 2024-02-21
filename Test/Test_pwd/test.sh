#!/bin/bash

run_Test_pwd() {
    # redirect pwd to exp file
    pwd >$t/exp
    
    # run the program with the input redirected from the inp file
    $prg <$t/inp 2>&1 >$t/out
}