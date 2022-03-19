cmake_minimum_required (VERSION 3.12)

# Add source to this project's executable.
add_executable(
  Cacus_EX
  "App.cpp"
)

target_link_libraries(
  Cacus_EX
    Cacus
)

# Move to Dir
install(
  TARGETS
    Cacus_EX
  DESTINATION ${_PLATFORM_PREFIX}
)