#!/bin/bash

# cogapp requires python:
# apt install python3-pip
# pip3 install cogapp
# ./generate.sh

cog -d compression.h.preface > ../api_compression.h
cog -d compression.cc.preface > ../../src/api_compression.cc
