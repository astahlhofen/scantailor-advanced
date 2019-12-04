// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.
#include "Logger.h"
#include "writer/AbstractMessageWriter.h"

Logger* Logger::mInstance = 0;
int Logger::mLogLevel = INFO;

Logger* Logger::instance() {
  if (mInstance == 0) {
    mInstance = new Logger();
  }
  return mInstance;
}

void Logger::write(const std::string& _message, int _type) {
  /*
   * notify all message writers
   */
  if (_type > mLogLevel) {
    return;
  }

  for (std::vector<AbstractMessageWriter*>::iterator it = instance()->mMessageWriterVec.begin();
       it != instance()->mMessageWriterVec.end(); it++) {
    (*it)->write(_message, _type);
  }
}

void Logger::write(const char* _file, const char* _function, int _line, const char* _message, int _type) {
  /*
   * notify all message writers
   */
  if (_type > mLogLevel) {
    return;
  }

  for (std::vector<AbstractMessageWriter*>::iterator it = instance()->mMessageWriterVec.begin();
       it != instance()->mMessageWriterVec.end(); it++) {
    (*it)->write(_file, _function, _line, _message, _type);
  }
}

void Logger::setMessageWriter(AbstractMessageWriter* _messageWriter) {
  mMessageWriterVec.clear();
  mMessageWriterVec.push_back(_messageWriter);
}

void Logger::addMessageWriter(AbstractMessageWriter* _messageWriter) {
  mMessageWriterVec.push_back(_messageWriter);
}

void Logger::setLogLevel(LogLevel _logLevel) {
  mLogLevel = _logLevel;
}

size_t Logger::messageWritersCount() const {
  return mMessageWriterVec.size();
}

AbstractMessageWriter* Logger::messageWriter(size_t _index) const {
  if (_index >= mMessageWriterVec.size())
    return 0;
  return mMessageWriterVec[_index];
}

void Logger::deleteMessageWriter(size_t _index) {
  if (_index >= mMessageWriterVec.size())
    return;
  mMessageWriterVec.erase(mMessageWriterVec.begin() + _index);
}

Logger::Logger() {}

Logger::~Logger() {
  // if ( mInstance ) delete mInstance;
}

LogInterface& LogInterface::operator<<(EolType) {
  if (!mInstance) {
    throw std::runtime_error("Logger::operator<<(): The logger instance is not initialized.");
  }
  mInstance->write(mStream.str(), mType);
  return *this;
}
