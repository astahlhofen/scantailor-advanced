// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.
#include "Debug.h"

namespace cli {
namespace debug {

void logImageSettingsForPage(const std::string& message,
                             const intrusive_ptr<ImageSettings>& settings,
                             const PageId& pageId) {
  int bwThreshold = settings->getPageParams(pageId)->getBwThreshold();
  bool isBlackOnWhite = settings->getPageParams(pageId)->isBlackOnWhite();
  Logger::debug() << message << "bwThreshold=" << bwThreshold << ", blackOnWhite=" << isBlackOnWhite << Logger::eol;
}

void logImageFileInfos(const std::string& msgPrefix, const std::vector<ImageFileInfo>& imageFileInfos) {
  std::for_each(imageFileInfos.begin(), imageFileInfos.end(), [&](const ImageFileInfo& _imageFileInfo) {
    const std::vector<ImageMetadata>& perPageMetadata = _imageFileInfo.imageInfo();
    Logger::debug() << msgPrefix << " File " << _imageFileInfo.fileInfo().absoluteFilePath().toStdString()
                    << Logger::eol;
    int pageCounter = 0;
    std::for_each(perPageMetadata.cbegin(), perPageMetadata.cend(), [&](const ImageMetadata& _metadata) {
      Logger::debug() << msgPrefix << "   Page " << pageCounter << ":" << Logger::eol;
      Logger::debug() << msgPrefix << "     DPI  : " << _metadata.dpi().horizontal() << " x "
                      << _metadata.dpi().vertical() << Logger::eol;
      Logger::debug() << msgPrefix << "     Size : " << _metadata.size().width() << " x " << _metadata.size().height()
                      << Logger::eol;
    });
  });
}


}  // namespace debug
}  // namespace cli
