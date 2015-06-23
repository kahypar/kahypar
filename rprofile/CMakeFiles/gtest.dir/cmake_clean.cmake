FILE(REMOVE_RECURSE
  "CMakeFiles/gtest"
  "CMakeFiles/gtest-complete"
  "gtest-prefix/src/gtest-stamp/gtest-install"
  "gtest-prefix/src/gtest-stamp/gtest-mkdir"
  "gtest-prefix/src/gtest-stamp/gtest-download"
  "gtest-prefix/src/gtest-stamp/gtest-update"
  "gtest-prefix/src/gtest-stamp/gtest-patch"
  "gtest-prefix/src/gtest-stamp/gtest-configure"
  "gtest-prefix/src/gtest-stamp/gtest-build"
)

# Per-language clean rules from dependency scanning.
FOREACH(lang)
  INCLUDE(CMakeFiles/gtest.dir/cmake_clean_${lang}.cmake OPTIONAL)
ENDFOREACH(lang)
