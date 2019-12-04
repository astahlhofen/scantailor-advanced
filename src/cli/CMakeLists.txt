set(appname scantailor-cli)
set(sources
  main.cpp)
  
add_executable(${appname} ${sources})
target_include_directories(${appname} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
install(TARGETS ${appname} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")