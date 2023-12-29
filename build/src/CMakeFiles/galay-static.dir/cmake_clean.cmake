file(REMOVE_RECURSE
  "libgalay-static.a"
  "libgalay-static.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang CXX)
  include(CMakeFiles/galay-static.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
