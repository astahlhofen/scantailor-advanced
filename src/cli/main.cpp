// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <QCoreApplication>
#include <QFileInfo>
#include <QTimer>
#include "CmdRunner.h"
#include "version.h"


int main(int _argc, char** _argv) {
  // Initialize the Qt application.
  QCoreApplication app(_argc, _argv);
  QCoreApplication::setApplicationVersion(VERSION);
  QCoreApplication::setApplicationName("scantailor-cli");

  CmdRunner runner;
  return runner.run();
}
