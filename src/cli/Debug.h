// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CLI_DEBUG_H_
#define SCANTAILOR_CLI_DEBUG_H_

#include "core/logger/Logger.h"

void logImageSettingsForPage(const std::string& message,
                             const intrusive_ptr<ImageSettings>& settings,
                             const PageId& pageId) {
  int bwThreshold = settings->getPageParams(pageId)->getBwThreshold();
  bool isBlackOnWhite = settings->getPageParams(pageId)->isBlackOnWhite();
  Logger::debug() << message << "bwThreshold=" << bwThreshold << ", blackOnWhite=" << isBlackOnWhite << Logger::eol;
}

#endif  // SCANTAILOR_CLI_DEBUG_H_
