#!/bin/sh
set -o errexit
set -o xtrace

eosiocpp -o ./rankblock.token/rankblock.token.wast ./rankblock.token/rankblock.token.cpp
eosiocpp -g ./rankblock.token/rankblock.token.abi ./rankblock.token/rankblock.token.cpp
eosiocpp -o ./rankblock.code/rankblock.code.wast ./rankblock.code/rankblock.code.cpp
eosiocpp -g ./rankblock.code/rankblock.code.abi ./rankblock.code/rankblock.code.cpp