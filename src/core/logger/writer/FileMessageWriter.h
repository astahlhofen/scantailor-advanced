// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.
#ifndef SCANTAILOR_CORE_LOGGER_WRITER_FILEMESSAGEWRITER_H_
#define SCANTAILOR_CORE_LOGGER_WRITER_FILEMESSAGEWRITER_H_

#include <string>
#include "writer/AbstractMessageWriter.h"

/**
 * \brief write  log-messages to file.
 *
 * Add this message writer to the global logger to dump all messages to file
 */
class FileMessageWriter : public AbstractMessageWriter {
 public:
  explicit FileMessageWriter(const std::string& _filename);
  virtual ~FileMessageWriter();

  virtual void write(const std::string& _message, int _messageType = INFO);

  virtual void write(const char* _file, const char* _function, int _line, const char* _message, int _type = INFO);

 protected:
  std::string mFilename;
  std::ofstream* mOfstream;
};

#endif  // SCANTAILOR_CORE_LOGGER_WRITER_FILEMESSAGEWRITER_H_
