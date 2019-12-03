#!/bin/bash

function cleanup {
    rm -f out/*
    rm -f disks/*
    rm -rf program
    rm -f recover
}

cleanup
