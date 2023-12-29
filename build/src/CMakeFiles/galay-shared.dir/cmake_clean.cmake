file(REMOVE_RECURSE
  "libgalay-shared.pdb"
  "libgalay-shared.so"
)

# Per-language clean rules from dependency scanning.
foreach(lang CXX)
  include(CMakeFiles/galay-shared.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
