// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "CmdRunner.h"
#include <algorithm>
#include <iostream>
#include <vector>
#include "CmdParser.h"
#include "cli/ProjectWriter.h"
#include "core/ImageFileInfo.h"
#include "core/ImageLoader.h"
#include "core/ImageMetadataLoader.h"
#include "core/Margins.h"
#include "core/PageSequence.h"
#include "core/ProjectPages.h"
#include "core/SmartFilenameOrdering.h"
#include "core/filters/page_layout/Alignment.h"
#include "core/filters/page_layout/Params.h"
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

  using AbstractTaskPtr = intrusive_ptr<::cli::AbstractTask>;
  std::vector<AbstractTaskPtr> tasks{};

  // Output task.
  auto outputSettings = make_intrusive<output::Settings>();

  ::output::ColorParams outputColorParams{};
  outputColorParams.setColorMode(::output::ColorMode::COLOR_GRAYSCALE);

  ::output::ColorCommonOptions outputColorCommonOptions{};
  outputColorCommonOptions.setNormalizeIllumination(false);
  outputColorCommonOptions.setFillMargins(true);
  outputColorCommonOptions.setFillingColor(::output::FillingColor::FILL_BACKGROUND);
  outputColorCommonOptions.setFillOffcut(true);

  ::output::ColorCommonOptions::PosterizationOptions outputPosterizationOptions{};
  outputPosterizationOptions.setEnabled(true);
  outputPosterizationOptions.setForceBlackAndWhite(true);
  outputPosterizationOptions.setLevel(6);
  outputPosterizationOptions.setNormalizationEnabled(true);
  outputColorCommonOptions.setPosterizationOptions(outputPosterizationOptions);
  outputColorParams.setColorCommonOptions(outputColorCommonOptions);

  ::output::BlackWhiteOptions outputBlackWhiteOptions{};
  outputBlackWhiteOptions.setMorphologicalSmoothingEnabled(true);
  outputBlackWhiteOptions.setBinarizationMethod(::output::BinarizationMethod::OTSU);
  outputBlackWhiteOptions.setNormalizeIllumination(false);
  outputBlackWhiteOptions.setWolfUpperBound(254);
  outputBlackWhiteOptions.setWolfLowerBound(1);
  outputBlackWhiteOptions.setWolfCoef(0.3);
  outputBlackWhiteOptions.setSavitzkyGolaySmoothingEnabled(true);
  outputBlackWhiteOptions.setThresholdAdjustment(0);
  outputBlackWhiteOptions.setSauvolaCoef(0.34);
  outputBlackWhiteOptions.setWindowSize(200);

  ::output::BlackWhiteOptions::ColorSegmenterOptions outputColorSegmenterOptions;
  outputColorSegmenterOptions.setGreenThresholdAdjustment(0);
  outputColorSegmenterOptions.setEnabled(true);
  outputColorSegmenterOptions.setBlueThresholdAdjustment(0);
  outputColorSegmenterOptions.setNoiseReduction(7);
  outputColorSegmenterOptions.setRedThresholdAdjustment(0);
  outputBlackWhiteOptions.setColorSegmenterOptions(outputColorSegmenterOptions);
  outputColorParams.setBlackWhiteOptions(outputBlackWhiteOptions);

  ::output::SplittingOptions outputSplittingOptions{};
  outputSplittingOptions.setSplitOutput(true);
  outputSplittingOptions.setSplittingMode(::output::SplittingMode::COLOR_FOREGROUND);
  outputSplittingOptions.setOriginalBackgroundEnabled(false);

  ::output::PictureShapeOptions outputPictureShapeOptions{};
  outputPictureShapeOptions.setSensitivity(100);
  outputPictureShapeOptions.setPictureShape(::output::PictureShape::OFF_SHAPE);
  outputPictureShapeOptions.setHigherSearchSensitivity(true);

  ::dewarping::DistortionModel outputDistortionModel{};

  ::output::DepthPerception outputDepthPerception{2.0};

  ::output::DewarpingOptions outputDewarpingOptions{
      ::output::DewarpingMode::OFF,  // dewarpingMode
      false                          // needPostDeskew
  };

  ::output::Params outputPageParams{
      {600, 600},                 // dpi
      outputColorParams,          // colorParams
      outputSplittingOptions,     // splittingOptions
      outputPictureShapeOptions,  // pictureShapeOptions
      outputDistortionModel,      // distortionModel
      outputDepthPerception,      // depthPerception
      outputDewarpingOptions,     // dewarpingOptions
      0.0                         // despeckLevel
  };
  outputSettings->setParams(firstPage.id(), outputPageParams);

  ::output::OutputProcessingParams outputProcessingParams{};
  outputProcessingParams.setBlackOnWhiteSetManually(true);
  outputSettings->setOutputProcessingParams(firstPage.id(), outputProcessingParams);

  const QString outDir = options.outputDirectory.absolutePath();
  OutputFileNameGenerator outFileNameGen{make_intrusive<FileNameDisambiguator>(), outDir,
                                         Qt::LayoutDirection::LeftToRight};
  auto outputTask
      = make_intrusive<::cli::output::Task>(outputSettings, firstPage.id(), outFileNameGen, isBatch, isDebug);
  tasks.insert(tasks.begin(), outputTask);

  // Page layout task.
  auto pageLayoutSettings = make_intrusive<::page_layout::Settings>();
  ::page_layout::Params pageLayoutPageParams{
      {0, 0, 0, 0},                                                        // margins
      {0, 0, 0, 0},                                                        // pageRect
      {0, 0, 0, 0},                                                        // contentRect
      {0, 0},                                                              // contentSizeMM
      {::page_layout::Alignment::VAUTO, ::page_layout::Alignment::HAUTO},  // Alignment
      true                                                                 // autoMargins
  };
  pageLayoutSettings->setPageParams(firstPage.id(), pageLayoutPageParams);
  auto pageLayoutTask
      = make_intrusive<::cli::page_layout::Task>(pageLayoutSettings, firstPage.id(), outputTask, isBatch, isDebug);
  tasks.insert(tasks.begin(), pageLayoutTask);

  // Select content task.
  auto selectContentSettings = make_intrusive<::select_content::Settings>();
  ::select_content::Params selectContentPageParams{
      {0, 0, 0, 0},               // contentRect
      {0, 0},                     // sizeMM
      {0, 0, 0, 0},               // pageRect
      {},                         // dependencies
      AutoManualMode::MODE_AUTO,  // contentDetectionMode
      AutoManualMode::MODE_AUTO,  // pageDetectionMode
      true                        // fineTuneCorners
  };
  selectContentSettings->setPageParams(firstPage.id(), selectContentPageParams);
  auto selectContentTask = make_intrusive<::cli::select_content::Task>(selectContentSettings, firstPage.id(),
                                                                       pageLayoutTask, isBatch, isDebug);
  tasks.insert(tasks.begin(), selectContentTask);

  // Deskew task.
  auto deskewSettings = make_intrusive<::deskew::Settings>();
  auto deskewTask = make_intrusive<::cli::deskew::Task>(deskewSettings, imageSettings, selectContentTask,
                                                        firstPage.id(), isBatch, isDebug);
  tasks.insert(tasks.begin(), deskewTask);

  // Page split task.
  auto pageSplitSettings = make_intrusive<::page_split::Settings>();
  auto pageSplitTask
      = make_intrusive<::cli::page_split::Task>(pageSplitSettings, pages, deskewTask, firstPage, isBatch, isDebug);
  tasks.insert(tasks.begin(), pageSplitTask);

  // Fix orientation task.
  auto fixOrientationSettings = make_intrusive<::fix_orientation::Settings>();
  auto fixOrientationTask = make_intrusive<::cli::fix_orientation::Task>(firstPage.id(), fixOrientationSettings,
                                                                         imageSettings, pageSplitTask, isBatch);
  tasks.insert(tasks.begin(), fixOrientationTask);

  // Load file task.
  auto loadTask = make_intrusive<::cli::fix_orientation::LoadFileTask>(firstPage, pages, fixOrientationTask);

  // Start processing.
  Logger::debug() << "CmdRunner::run(): Start processing pipeline for page with id " << firstPage.imageId().page()
                  << Logger::eol;
  loadTask->process();

  QString projectOutDir = "project/project.xml";
  OutputFileNameGenerator projectOutFileNameGen{make_intrusive<FileNameDisambiguator>(), projectOutDir,
                                                Qt::LayoutDirection::LeftToRight};
  ::cli::ProjectWriter writer(pages, projectOutFileNameGen);
  writer.write("project.xml", tasks);

  return 0;
}
