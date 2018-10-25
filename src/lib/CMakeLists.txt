file(GLOB_RECURSE libsource tempest/**/*.te)

add_custom_command(
  OUTPUT "corelib.bc"
  COMMAND tempestc
    -o corelib
    --source-root ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/tempest/core
  DEPENDS tempestc ${libsource}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMENT "building core libraries"
  VERBATIM
)

add_custom_target(
  corelib
  DEPENDS "corelib.bc"
)
