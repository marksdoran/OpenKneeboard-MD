ExternalProject_Add(
  geographiclibBuild
  URL "https://github.com/geographiclib/geographiclib/archive/refs/tags/v2.1.2.zip"
  URL_HASH "SHA256=b750b1eed42b280e2ce00c96b3a1f1fae95e04b28d95a5e854c320823d749d7c"
  CMAKE_ARGS
  "-DCMAKE_TOOLCHAIN_FILE=${THIRDPARTY_TOOLCHAIN_FILE}"
  -DBUILD_SHARED_LIBS=OFF
  BUILD_COMMAND
  "${CMAKE_COMMAND}"
  --build .
  --config "$<CONFIG>"
  --parallel
  --
  /p:CL_MPCount=
  /p:UseMultiToolTask=true
  /p:EnforceProcessCountAcrossBuilds=true
  INSTALL_COMMAND
  "${CMAKE_COMMAND}"
  --install .
  --prefix "<INSTALL_DIR>/$<CONFIG>"
  --config "$<CONFIG>"
  EXCLUDE_FROM_ALL
)

ExternalProject_Get_property(geographiclibBuild SOURCE_DIR)
ExternalProject_Get_property(geographiclibBuild INSTALL_DIR)
add_library(geographiclib INTERFACE)
add_dependencies(geographiclib INTERFACE geographiclibBuild)

target_link_libraries(
  geographiclib
  INTERFACE
  "${INSTALL_DIR}/$<CONFIG>/lib/geographiclib$<$<CONFIG:Debug>:_d>.lib"
)
target_include_directories(
  geographiclib
  INTERFACE
  "${INSTALL_DIR}/$<CONFIG>/include"
)
add_library(ThirdParty::GeographicLib ALIAS geographiclib)

install(
  FILES "${SOURCE_DIR}/LICENSE.txt"
  TYPE DOC
  RENAME "LICENSE-ThirdParty-geographiclib.txt"
)
