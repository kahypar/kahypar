#install latest version of cibuildwheel 
python3 -m pip install cibuildwheel

# make sure we use the packaged minimal boost implementation, as cibuildwheel doesn't seem to properly handle the boost dependency.
export KAHYPAR_USE_MINIMAL_BOOST=ON

# These builds don't seem to work with cibuildwheel anymore
export CIBW_SKIP=cp38-macosx_*

# create wheels
cibuildwheel --platform macos 

# upload
twine upload wheelhouse/*
