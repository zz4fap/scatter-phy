#!/bin/bash

origin=$(pwd)
cd communicator
protoc --python_out=./python --cpp_out=./cpp interf.proto
cd ${origin}
protoc --cpp_out=. --python_out=. registration.proto
protoc --cpp_out=. cil.proto
cd ${origin}


