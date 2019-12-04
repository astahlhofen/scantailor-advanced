// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.
#include "writer/StandardMessageWriter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>

StandardMessageWriter::StandardMessageWriter() : mRed(31), mGreen(32), mBlue(34), mYellow(33), mWhite(37) {}

StandardMessageWriter::~StandardMessageWriter() {}

void StandardMessageWriter::write(const std::string& _message, int _messageType) {
  std::string messageType;
  char color;

  switch (_messageType) {
    case ERROR:
      messageType = "Error     ";
      color = mRed;  // red
      break;
    case INFO:
      messageType = "Info      ";
      color = mYellow;  // yellow
      break;
    case DEBUG:
      messageType = "Debug   ";
      color = mBlue;  // blue
      break;
    default:
    case WARNING:
      messageType = "Warning   ";
      color = mGreen;  // green
      break;
  }

  printf("%c[%d;%dm%s", 0x1B, 1, color, messageType.c_str());
  printf("%c[%dm", 0x1B, 0);

  printf(":%s\n", _message.c_str());
}

void StandardMessageWriter::write(const char* _file,
                                  const char* _function,
                                  int _line,
                                  const char* _message,
                                  int _type) {
  std::string messageType;
  char color;

  switch (_type) {
    case ERROR:
      messageType = "Error     ";
      color = mRed;  // red
      break;
    case INFO:
      messageType = "Info      ";
      color = mGreen;  // yellow
      break;
    case DEBUG:
      messageType = "Debug   ";
      color = mBlue;  // blue
      break;
    default:
    case WARNING:
      messageType = "Warning   ";
      color = mGreen;  // green
      break;
  }

  printf("%c[%d;%dm%s", 0x1B, 1, color, messageType.c_str());
  printf("%c[%dm", 0x1B, 0);
  printf(":%s", _message);
  printf("[in %s @ %s, line %i] \n", _file, _function, _line);
}
