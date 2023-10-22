include(FetchContent)
FetchContent_Populate(
  boost-src
  URL https://sourceforge.net/projects/boost/files/boost/1.83.0/boost_1_83_0.tar.bz2
  URL_HASH MD5=406f0b870182b4eb17a23a9d8fce967d
  SOURCE_DIR external_tools/boost
)