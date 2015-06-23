FILE(REMOVE_RECURSE
  "CMakeFiles/gmock"
  "CMakeFiles/gmock-complete"
  "gmock-prefix/src/gmock-stamp/gmock-install"
  "gmock-prefix/src/gmock-stamp/gmock-mkdir"
  "gmock-prefix/src/gmock-stamp/gmock-download"
  "gmock-prefix/src/gmock-stamp/gmock-update"
  "gmock-prefix/src/gmock-stamp/gmock-patch"
  "gmock-prefix/src/gmock-stamp/gmock-configure"
  "gmock-prefix/src/gmock-stamp/gmock-build"
)

# Per-language clean rules from dependency scanning.
FOREACH(lang)
  INCLUDE(CMakeFiles/gmock.dir/cmake_clean_${lang}.cmake OPTIONAL)
ENDFOREACH(lang)
