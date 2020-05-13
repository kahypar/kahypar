# using cibuildwheel
python3 -m pip install cibuildwheel==1.4.1

# python2.7 does not work well with C++17
export CIBW_SKIP=cp27-macosx_x86_64

# create wheels
python3 -m cibuildwheel --output-dir wheelhouse --platform macos

# upload
twine upload wheelhouse/*
