// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.
#ifndef SCANTAILOR_CORE_LOGGER_WRITER_ABSTRACTMESSAGEWRITER_H_
#define SCANTAILOR_CORE_LOGGER_WRITER_ABSTRACTMESSAGEWRITER_H_

#include <string>
#include "../LogLevel.h"

/**
 * \brief Prototype for MessageWriters. Every MessageWriter must be derived from this class
 */
class AbstractMessageWriter {
 public:
  AbstractMessageWriter();
  virtual ~AbstractMessageWriter();

  /**
   * write message to log destinations.
   * \param _message the message
   * \param _messageType the message type (critical, verbose) etc.
   */
  virtual void write(const std::string& _message, int _messageType = INFO) = 0;

  virtual void write(const char* _file, const char* _function, int _line, const char* _message, int _type = INFO) = 0;
};

#endif  // SCANTAILOR_CORE_LOGGER_WRITER_ABSTRACTMESSAGEWRITER_H_
