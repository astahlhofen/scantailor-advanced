set(appname scantailor-cli)
set(sources
  CmdParser.h
  main.cpp)
  
add_executable(${appname} ${sources})
target_include_directories(${appname} PRIVATE 
  "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${appname} PRIVATE
  core
  logger
  Qt5::Core
)
install(TARGETS ${appname} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")