// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "CLIRunner.h"


int main(int argc, char** argv) {
  ::cli::CLIRunner runner;
  return runner.run(argc, argv);
}
