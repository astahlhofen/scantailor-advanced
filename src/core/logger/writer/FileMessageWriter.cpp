// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.
#include "writer/FileMessageWriter.h"
#include <ctype.h>
#include <fstream>
#include <iostream>
#include <sstream>

FileMessageWriter::FileMessageWriter(const std::string& _filename) {
  mOfstream = new std::ofstream();
  mOfstream->open(_filename.c_str());
}

FileMessageWriter::~FileMessageWriter() {
  mOfstream->close();
  delete mOfstream;
}

void FileMessageWriter::write(const std::string& _message, int _messageType) {
  std::string logMessage;
  switch (_messageType) {
    case ERROR:
      logMessage = "Error         ";
      break;
    case WARNING:
      logMessage = "Warning       ";
      break;
    case DEBUG:
      logMessage = "Debug       ";
      break;
    default:
    case INFO:
      logMessage = "Information   ";
      break;
  }
  logMessage += " :: " + _message + "\n";

  mOfstream->write(logMessage.c_str(), logMessage.size());
  mOfstream->flush();
}

void FileMessageWriter::write(const char* _file, const char* _function, int _line, const char* _message, int _type) {
  std::string logMessage;
  switch (_type) {
    case ERROR:
      logMessage = "Error         ";
      break;
    case WARNING:
      logMessage = "Warning       ";
      break;
    case DEBUG:
      logMessage = "Debug       ";
      break;
    default:
    case INFO:
      logMessage = "Information   ";
      break;
  }
  logMessage += " :: " + std::string(_message) + "\n";
  logMessage += "[ in " + std::string(_file) + " @ " + std::string(_function) + " : " + std::to_string(_line);

  mOfstream->write(logMessage.c_str(), logMessage.size());
  mOfstream->flush();
}
