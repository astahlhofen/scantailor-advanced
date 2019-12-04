set(libname logger)
set(sources
  Logger.cpp Logger.h
  LogLevel.h
  writer/AbstractMessageWriter.cpp writer/AbstractMessageWriter.h
  writer/FileMessageWriter.cpp writer/FileMessageWriter.h
  writer/StandardMessageWriter.cpp writer/StandardMessageWriter.h)
  
add_library(${libname} STATIC ${sources})
target_include_directories(${libname} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")