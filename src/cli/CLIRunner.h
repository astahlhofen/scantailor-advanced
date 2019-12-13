// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.


#ifndef SCANTAILOR_CLI_CLIRunner_H_
#define SCANTAILOR_CLI_CLIRunner_H_

#include "CLIParser.h"

namespace cli {

class CLIRunner {
 public:
  CLIRunner();
  virtual ~CLIRunner();

 public:
  int run(int argc, char** argv);

 private:
  ::cli::CLIParser m_parser;
};

}  // namespace cli

#endif  // SCANTAILOR_CLI_CLIRunner_H_
