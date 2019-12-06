set(appname scantailor-cli)

set(sources
  tasks/AbstractTask.h
  tasks/DeskewTask.cpp tasks/DeskewTask.h
  tasks/FixOrientationTask.cpp tasks/FixOrientationTask.h
  tasks/LoadFileTask.cpp tasks/LoadFileTask.h
  tasks/OutputTask.cpp tasks/OutputTask.h
  tasks/PageLayoutTask.cpp tasks/PageLayoutTask.h
  tasks/PageSplitTask.cpp tasks/PageSplitTask.h
  tasks/SelectContentTask.cpp tasks/SelectContentTask.h
  CmdParser.h
  CmdRunner.cpp CmdRunner.h
  Debug.h
  main.cpp
  ProjectWriter.cpp ProjectWriter.h)
  
add_executable(${appname} ${sources})
target_include_directories(${appname} PRIVATE 
  "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${appname} PRIVATE
  core
  dewarping
  zones
  logger
  Qt5::Core
)
install(TARGETS ${appname} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")