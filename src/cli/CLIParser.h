// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef CLI_CLIParser_H_
#define CLI_CLIParser_H_

#include <QDir>
#include <sstream>
#include "core/logger/Logger.h"

namespace CLI {

std::istringstream& operator>>(std::istringstream& in, QDir& directory);

}  // namespace CLI

#include "vendor/CLI11.hpp"

namespace cli {

class CLIParser {
 public:
  CLIParser();
  virtual ~CLIParser();

  // ##############################################################################################
  // # API
  // ##############################################################################################

 public:
  int process(int argc, char** argv);

  const std::vector<QFileInfo>& inputFiles() const { return m_inputFiles; }
  const QDir& outputDir() const { return m_outputDir; }
  bool generateProjectFile() const { return m_generateProjectFile; }

 private:
  ::CLI::App m_app;

  QDir m_outputDir;
  LogLevel m_logLevel;
  std::vector<QFileInfo> m_inputFiles;
  bool m_generateProjectFile;
};

}  // namespace cli

#endif  // CLI_CLIParser_H_
