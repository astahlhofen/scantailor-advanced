// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <QFileInfo>
#include <algorithm>
#include <iostream>
#include <vector>
#include "CmdParser.h"
#include "core/ImageFileInfo.h"
#include "core/ImageMetadata.h"
#include "core/ImageMetadataLoader.h"
#include "core/SmartFilenameOrdering.h"
#include "core/logger/Logger.h"
#include "core/logger/writer/StandardMessageWriter.h"
#include "version.h"

int main(int _argc, char** _argv) {
  // Initialize the Qt application.
  QCoreApplication app(_argc, _argv);
  QCoreApplication::setApplicationVersion(VERSION);
  QCoreApplication::setApplicationName("scantailor-cli");

  // Initialize the command line parser.
  QCommandLineParser parser;
  parser.setApplicationDescription("The scantailor command line interface.");

  // Initialize the logger.
  Logger::instance()->addMessageWriter(new StandardMessageWriter());

  // Read command line options.
  CmdOptions options;
  QString errorMessage;
  switch (parseCommandLine(parser, &options, &errorMessage)) {
    case OPTIONS_OK:
      break;
    case OPTIONS_ERROR:
      std::cerr << "ERROR: Failed to parse command line options - " << errorMessage.toStdString() << std::endl;
      std::cerr << parser.helpText().toStdString() << std::endl;
      return 1;
    case OPTIONS_VERSION_REQUESTED:
      std::cerr << QCoreApplication::applicationName().toStdString() << " "
                << qPrintable(QCoreApplication::applicationVersion()) << std::endl;
      return 0;
    case OPTIONS_HELP_REQUESTED:
      parser.showHelp();
      return 0;
  }

  // Convert the parsed QFileInfos into ImageFileInfos.
  std::vector<ImageFileInfo> imageFileInfos;
  std::for_each(options.inputFiles.begin(), options.inputFiles.end(), [&](const QFileInfo& _fileInfo) {
    std::vector<ImageMetadata> perPageMetadata;
    const ImageMetadataLoader::Status st = ImageMetadataLoader::load(
        _fileInfo.absoluteFilePath(), [&](const ImageMetadata& _metadata) { perPageMetadata.push_back(_metadata); });

    if (st == ImageMetadataLoader::LOADED) {
      imageFileInfos.emplace_back(_fileInfo, perPageMetadata);
    } else {
      Logger::error() << "ERROR: Failed to load image file '" << _fileInfo.absoluteFilePath().toStdString()
                      << "'. Maybe the specified file is corrupt or no supported image type." << Logger::eol;
      QCoreApplication::exit(1);
    }
  });
  std::sort(imageFileInfos.begin(), imageFileInfos.end(), [](const ImageFileInfo& _lhs, const ImageFileInfo& _rhs) {
    return SmartFilenameOrdering()(_lhs.fileInfo(), _rhs.fileInfo());
  });

  // DEBUG INFO
  std::for_each(imageFileInfos.begin(), imageFileInfos.end(), [](ImageFileInfo& _fileItem) {
    const std::vector<ImageMetadata>& perPageMetadata = _fileItem.imageInfo();
    Logger::debug() << "main(): File " << _fileItem.fileInfo().absoluteFilePath().toStdString() << Logger::eol;
    int pageCounter = 0;
    std::for_each(perPageMetadata.cbegin(), perPageMetadata.cend(), [&](const ImageMetadata& _metadata) {
      Logger::debug() << "main():   Page " << pageCounter << ":" << Logger::eol;
      Logger::debug() << "main():     DPI  : " << _metadata.dpi().horizontal() << " x " << _metadata.dpi().vertical()
                      << Logger::eol;
      Logger::debug() << "main():     Size : " << _metadata.size().width() << " x " << _metadata.size().height()
                      << Logger::eol;
    });
  });
  /////////////

  return 0;
}
