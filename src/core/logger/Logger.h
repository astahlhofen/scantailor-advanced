// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.
#ifndef SCANTAILOR_CORE_LOGGER_LOGGER_H_
#define SCANTAILOR_CORE_LOGGER_LOGGER_H_

#include <sstream>
#include <string>
#include <vector>
#include "LogLevel.h"

/*
 * forward
 */
class AbstractMessageWriter;

/*!
 * this preprocessor macro is provided for convenience:
 * it can be used to write LM("log-message..") instead of
 * the log-message, it will provide additional information
 * about the caller of the logger for more detailed messages
 */
#ifndef LM
#define LM(x...) __FILE__, __FUNCTION__, __LINE__, x
#endif

/*!
 * \brief The EolType struct end-of-line helper for logging
 */
struct EolType {};

class Logger;

/*!
 * \brief The LogInterface class enables stream based operation on log-messages
 */
class LogInterface {
 public:
  LogInterface(int _type, Logger* _instance) : mType(_type), mInstance(_instance) {}

  LogInterface& operator<<(EolType);

  template <typename T>
  LogInterface& operator<<(T _val);

  LogInterface(const LogInterface& _if) : mType(_if.mType), mInstance(_if.mInstance) {}

 protected:
  int mType;
  Logger* mInstance;
  std::ostringstream mStream;
};

/**
 * \brief a class with logging functionality, implemented as a singleton
 */
class Logger {
 public:
  static EolType eol;

  /*!
   * get pointer to single instance
   * \return instance of the logger, will create one if none exists
   */
  static Logger* instance();

  static LogInterface debug() { return LogInterface(DEBUG, instance()); }
  static LogInterface info() { return LogInterface(INFO, instance()); }
  static LogInterface warning() { return LogInterface(WARNING, instance()); }
  static LogInterface error() { return LogInterface(ERROR, instance()); }

  /*!
   * write string to all message writers
   * @param _message the message
   * @param _type type of the message, used for message filtering
   */
  static void write(const std::string& _message, int _type = INFO);

  /*!
   * write string and additional meta information to all message writers
   * @param _file the source file name, expanded from LM()
   * @param _function function name, expanded from LM()
   * @param _line source code line, expanded from LM()
   * @param _message the message
   * @param _type the message type
   */
  static void write(const char* _file, const char* _function, int _line, const char* _message, int _type = INFO);

  /*!
   * set the ONLY message writer, MessageWriters added before will be removed
   * @param _MessageWriter
   */
  virtual void setMessageWriter(AbstractMessageWriter* _MessageWriter);

  /*!
   * add a new message-writer
   * @param _MessageWriter
   */
  virtual void addMessageWriter(AbstractMessageWriter* _MessageWriter);

  /*!
   * set the global log level.
   * @param _logLevel the log level
   */
  virtual void setLogLevel(LogLevel _logLevel);

  /*!
   * return the number of currently assigned message-writers
   * @return number of writers
   */
  size_t messageWritersCount() const;

  /*!
   * return the message writer with the specified index
   * @param _index index of the message-writer, must be between \
   *  zero and messageWritersCount() - 1
   * @return pointer to the writer, zero iff index is out of range
   */
  AbstractMessageWriter* messageWriter(size_t _index) const;

  /*!
   * delete message writer with the specified index, do nothing \
   * if index out of range
   * @param _index index
   */
  void deleteMessageWriter(size_t _index);

 protected:
  Logger();                 // singleton
  Logger(const Logger&) {}  // singleton, so no copies
  virtual ~Logger();
  Logger& operator=(const Logger&) { return (*this); }

  static Logger* mInstance;
  static int mLogLevel;

  std::vector<AbstractMessageWriter*> mMessageWriterVec;
};

template <typename T>
LogInterface& LogInterface::operator<<(T _val) {
  mStream << _val;
  return *this;
}

#ifndef lprintf
#define lprintf(x...)                        \
  {                                          \
    char buf[255];                           \
    snprintf(buf, sizeof(buf), x);           \
    Logger::verbose() << buf << Logger::eol; \
  }
#endif

#ifndef lprint
#define lprint(t, x...)            \
  {                                \
    char buf[255];                 \
    snprintf(buf, sizeof(buf), x); \
    Logger::write(buf, t);         \
  }
#endif

#endif  // SCANTAILOR_CORE_LOGGER_LOGGER_H_
