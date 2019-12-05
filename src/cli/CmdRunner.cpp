// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "CmdRunner.h"
#include <algorithm>
#include <iostream>
#include <vector>
#include "CmdParser.h"
#include "core/ImageFileInfo.h"
#include "core/ImageLoader.h"
#include "core/ImageMetadataLoader.h"
#include "core/PageSequence.h"
#include "core/ProjectPages.h"
#include "core/SmartFilenameOrdering.h"
#include "core/logger/Logger.h"
#include "core/logger/writer/StandardMessageWriter.h"
#include "foundation/TaskStatus.h"
#include "tasks/FixOrientationTask.h"
#include "tasks/LoadFileTask.h"
#include "tasks/OutputTask.h"

CmdRunner::CmdRunner() {}

CmdRunner::~CmdRunner() {}

int CmdRunner::run() {
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
      parser.showHelp(1);
      break;
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

  // Print DEBUG INFO about the parsed image files.
  std::for_each(imageFileInfos.begin(), imageFileInfos.end(), [](ImageFileInfo& _fileItem) {
    const std::vector<ImageMetadata>& perPageMetadata = _fileItem.imageInfo();
    Logger::debug() << "CmdRunner::run(): File " << _fileItem.fileInfo().absoluteFilePath().toStdString()
                    << Logger::eol;
    int pageCounter = 0;
    std::for_each(perPageMetadata.cbegin(), perPageMetadata.cend(), [&](const ImageMetadata& _metadata) {
      Logger::debug() << "CmdRunner::run():   Page " << pageCounter << ":" << Logger::eol;
      Logger::debug() << "CmdRunner::run():     DPI  : " << _metadata.dpi().horizontal() << " x "
                      << _metadata.dpi().vertical() << Logger::eol;
      Logger::debug() << "CmdRunner::run():     Size : " << _metadata.size().width() << " x "
                      << _metadata.size().height() << Logger::eol;
    });
  });
  /////////////

  // Create the project pages.
  auto pages = make_intrusive<ProjectPages>(imageFileInfos, ProjectPages::ONE_PAGE, Qt::LeftToRight);
  PageSequence pageSequence = pages->toPageSequence(PageView::PAGE_VIEW);
  Logger::debug() << "CmdRunner::run(): Number of pages is " << pageSequence.numPages() << Logger::eol;

  const PageInfo& firstPage = pageSequence.pageAt(0);
  const ImageId& firstPageImageId = firstPage.imageId();

  auto imageSettings = make_intrusive<ImageSettings>();
  bool isBatch = false;
  bool isDebug = true;

  // Output task.
  auto outputSettings = make_intrusive<output::Settings>();
  const QString outDir = options.outputDirectory.absolutePath();
  OutputFileNameGenerator outFileNameGen{make_intrusive<FileNameDisambiguator>(), outDir,
                                         Qt::LayoutDirection::LeftToRight};
  auto outputTask = make_intrusive<output::cli::Task>(outputSettings, firstPage.id(), outFileNameGen, isBatch, isDebug);

  // Page layout task.
  auto pageLayoutSettings = make_intrusive<page_layout::Settings>();
  auto pageLayoutTask
      = make_intrusive<page_layout::cli::Task>(pageLayoutSettings, firstPage.id(), outputTask, isBatch, isDebug);

  // Select content task.
  auto selectContentSettings = make_intrusive<select_content::Settings>();
  auto selectContentTask = make_intrusive<select_content::cli::Task>(selectContentSettings, firstPage.id(),
                                                                     pageLayoutTask, isBatch, isDebug);

  // Deskew task.
  auto deskewSettings = make_intrusive<deskew::Settings>();
  auto deskewTask = make_intrusive<deskew::cli::Task>(deskewSettings, imageSettings, selectContentTask, firstPage.id(),
                                                      isBatch, isDebug);

  // Page split task.
  auto pageSplitSettings = make_intrusive<page_split::Settings>();
  auto pageSplitTask
      = make_intrusive<page_split::cli::Task>(pageSplitSettings, pages, deskewTask, firstPage, isBatch, isDebug);

  // Fix orientation task.
  auto fixOrientationSettings = make_intrusive<fix_orientation::Settings>();
  OrthogonalRotation rotation;
  rotation.nextClockwiseDirection();
  fixOrientationSettings->applyRotation(firstPageImageId, rotation);

  auto fixOrientationTask = make_intrusive<fix_orientation::cli::Task>(firstPage.id(), fixOrientationSettings,
                                                                       imageSettings, pageSplitTask, isBatch);

  // Load file task.
  auto loadTask = make_intrusive<fix_orientation::cli::LoadFileTask>(firstPage, pages, fixOrientationTask);

  // Start processing.
  Logger::debug() << "CmdRunner::run(): Start processing pipeline for page with id " << firstPage.imageId().page()
                  << Logger::eol;
  loadTask->process();

  return 0;
}
