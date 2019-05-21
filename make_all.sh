#!/bin/bash -ex
make

source /opt/emscripten/emsdk_env.sh
make www
cd www
exec emrun --no_browser --hostname 0.0.0.0 --port 8080 index.html
