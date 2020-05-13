#!/bin/bash -x

# run script inside a docker with
#
# docker pull quay.io/pypa/manylinux2014_x86_64
# docker run --rm -it -v /home/schlag/repo/kahypar:/kahypar quay.io/pypa/manylinux2014_x86_64:latest

# if used as script, exit on first error
set -e

yum update -y
yum install -y wget
yum install devtoolset-9-toolchain
scl enable devtoolset-9 bash

# build newer cmake
rm -rf /tmp/cmake
mkdir /tmp/cmake
cd /tmp/cmake

wget https://github.com/Kitware/CMake/releases/download/v3.17.0/cmake-3.17.0-Linux-x86_64.tar.gz
tar -zxf cmake-3.17.0-Linux-x86_64.tar.gz
cd cmake-3.17.0-Linux-x86_64
export PATH=$PWD/bin:$PATH

cd ..
wget http://downloads.sourceforge.net/project/boost/boost/1.69.0/boost_1_69_0.tar.bz2
tar -xjf boost_1_69_0.tar.bz2
cd boost_1_69_0
./bootstrap.sh --prefix=/usr/local
./b2 install --prefix=/usr/local --with-program_options

# build many wheels
rm -rf /kahypar/dist
mkdir -p /kahypar/dist
for p in /opt/python/*; do
    $p/bin/pip wheel --verbose /kahypar -w /kahypar/dist
done

# repair wheels
for w in /kahypar/dist/kahypar-*whl; do
    auditwheel repair $w -w /kahypar/dist/
    rm -f $w
done
