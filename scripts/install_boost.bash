wget http://downloads.sourceforge.net/project/boost/boost/1.69.0/boost_1_69_0.tar.bz2
tar -xjf boost_1_69_0.tar.bz2
cd boost_1_69_0
./bootstrap.sh --prefix=/usr/local
./b2 install --prefix=/usr/local --with-program_options
