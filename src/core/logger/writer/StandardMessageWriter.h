// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.
#ifndef SCANTAILOR_CORE_LOGGER_WRITER_STANDARDMESSAGEWRITER_H_
#define SCANTAILOR_CORE_LOGGER_WRITER_STANDARDMESSAGEWRITER_H_

#include <string>
#include "AbstractMessageWriter.h"

/**
 * \brief a simple std::out message writer.
 *
 * this message writer prints all messages to the standard output
 */
class StandardMessageWriter : public AbstractMessageWriter {
 public:
  StandardMessageWriter();
  virtual ~StandardMessageWriter();

  virtual void write(const std::string& _message, int _messageType = INFO);

  virtual void write(const char* _file, const char* _function, int _line, const char* _message, int _type = INFO);

 protected:
  int mRed, mGreen, mBlue, mYellow, mWhite;
};

#endif  // SCANTAILOR_CORE_LOGGER_WRITER_STANDARDMESSAGEWRITER_H_
