#install latest version of cibuildwheel 
python3 -m pip install cibuildwheel

# make sure we use the packaged minimal boost implementation, as cibuildwheel doesn't seem to properly handle the boost dependency.
export KAHYPAR_USE_MINIMAL_BOOST=ON

# create wheels
cibuildwheel --platform macos 

# upload
twine upload wheelhouse/*
