#!/usr/bin/env bash
#
#  Build the code in $1 and tag it with distribution $2 and version $3
#
set -e
set -vx

ls
pwd
base=$(cd $1; /bin/pwd)
dist=$2
ver=$3

cd $base
mkdir -p pkg

tag=$(git rev-parse --short HEAD)
tar=metrist-libcurl-agent-$dist-$ver-$tag.tar.gz

make clean all
cd build
tar cvfz ../pkg/$tar metrist-libcurl-agent.so*
echo $tar >$base/pkg/$dist-$ver

# Normally this matches the calling host's architecture, but
# this is cleaner
echo $(uname -m) >$base/pkg/$dist-$ver.arch
