#!/bin/bash -x

# run script inside a docker with
#
# docker pull quay.io/pypa/manylinux2010_x86_64
# docker run --rm -it -v /home/schlag/repo/kahypar:/kahypar quay.io/pypa/manylinux2010_x86_64

set -e

yum update -y
yum install -y wget

# build newer cmake
rm -rf /tmp/cmake
mkdir /tmp/cmake
cd /tmp/cmake

wget https://github.com/Kitware/CMake/releases/download/v3.15.5/cmake-3.15.5.tar.gz
tar -zxf cmake-3.15.5.tar.gz
cd cmake-3.15.5

./bootstrap --prefix=/usr/local
make -j4
make install

cd ..
wget https://phoenixnap.dl.sourceforge.net/project/boost/boost/1.58.0/boost_1_58_0.tar.gz
tar -zxf boost_1_58_0.tar.gz
cd boost_1_58_0
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
