// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.


#ifndef SCANTAILOR_CLI_CMDRUNNER_H_
#define SCANTAILOR_CLI_CMDRUNNER_H_


class CmdRunner {
 public:
  CmdRunner();
  virtual ~CmdRunner();

 public:
  int run();
};


#endif  // SCANTAILOR_CLI_CMDRUNNER_H_
