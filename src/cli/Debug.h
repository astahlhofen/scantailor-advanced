// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CLI_DEBUG_H_
#define SCANTAILOR_CLI_DEBUG_H_

#include <vector>
#include "core/ImageFileInfo.h"
#include "core/ImageSettings.h"
#include "core/PageId.h"
#include "core/logger/Logger.h"
#include "foundation/intrusive_ptr.h"


namespace cli {
namespace debug {

void logImageSettingsForPage(const std::string& message,
                             const intrusive_ptr<ImageSettings>& settings,
                             const PageId& pageId);

void logImageFileInfos(const std::string& msgPrefix, const std::vector<ImageFileInfo>& imageFileInfos);

}  // namespace debug
}  // namespace cli

#endif  // SCANTAILOR_CLI_DEBUG_H_
