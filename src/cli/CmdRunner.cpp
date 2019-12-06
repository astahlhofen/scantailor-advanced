// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "CmdRunner.h"
#include <algorithm>
#include <iostream>
#include <vector>
#include "CmdParser.h"
#include "Debug.h"
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

  ::cli::debug::logImageFileInfos("CmdRunner:run()", imageFileInfos);

  // Create the project pages.
  auto pages = make_intrusive<ProjectPages>(imageFileInfos, ProjectPages::ONE_PAGE, Qt::LeftToRight);
  PageSequence pageSequence = pages->toPageSequence(PageView::PAGE_VIEW);
  Logger::debug() << "CmdRunner::run(): Number of pages is " << pageSequence.numPages() << Logger::eol;

  auto begin{pageSequence.begin()};
  auto end{pageSequence.end()};

  using AbstractTaskPtr = intrusive_ptr<::cli::AbstractTask>;
  std::vector<AbstractTaskPtr> tasks{};

  for (auto pageIt = begin; pageIt != end; pageIt++) {
    auto imageSettings = make_intrusive<ImageSettings>();
    bool isBatch = true;
    bool isDebug = false;

    const PageInfo& currentPage = *pageIt;

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
    outputSettings->setParams(currentPage.id(), outputPageParams);

    ::output::OutputProcessingParams outputProcessingParams{};
    outputProcessingParams.setBlackOnWhiteSetManually(true);
    outputSettings->setOutputProcessingParams(currentPage.id(), outputProcessingParams);

    const QString outDir = options.outputDirectory.absolutePath();
    OutputFileNameGenerator outFileNameGen{make_intrusive<FileNameDisambiguator>(), outDir,
                                           Qt::LayoutDirection::LeftToRight};
    auto outputTask
        = make_intrusive<::cli::output::Task>(outputSettings, currentPage.id(), outFileNameGen, isBatch, isDebug);
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
    pageLayoutSettings->setPageParams(currentPage.id(), pageLayoutPageParams);
    auto pageLayoutTask
        = make_intrusive<::cli::page_layout::Task>(pageLayoutSettings, currentPage.id(), outputTask, isBatch, isDebug);
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
    selectContentSettings->setPageParams(currentPage.id(), selectContentPageParams);
    auto selectContentTask = make_intrusive<::cli::select_content::Task>(selectContentSettings, currentPage.id(),
                                                                         pageLayoutTask, isBatch, isDebug);
    tasks.insert(tasks.begin(), selectContentTask);

    // Deskew task.
    auto deskewSettings = make_intrusive<::deskew::Settings>();
    auto deskewTask = make_intrusive<::cli::deskew::Task>(deskewSettings, imageSettings, selectContentTask,
                                                          currentPage.id(), isBatch, isDebug);
    tasks.insert(tasks.begin(), deskewTask);

    // Page split task.
    auto pageSplitSettings = make_intrusive<::page_split::Settings>();
    auto pageSplitTask
        = make_intrusive<::cli::page_split::Task>(pageSplitSettings, pages, deskewTask, currentPage, isBatch, isDebug);
    tasks.insert(tasks.begin(), pageSplitTask);

    // Fix orientation task.
    auto fixOrientationSettings = make_intrusive<::fix_orientation::Settings>();
    auto fixOrientationTask = make_intrusive<::cli::fix_orientation::Task>(currentPage.id(), fixOrientationSettings,
                                                                           imageSettings, pageSplitTask, isBatch);
    tasks.insert(tasks.begin(), fixOrientationTask);

    // Load file task.
    auto loadTask = make_intrusive<::cli::fix_orientation::LoadFileTask>(currentPage, pages, fixOrientationTask);

    // Start processing.
    Logger::debug() << "CmdRunner::run(): Start processing pipeline for page with id " << currentPage.imageId().page()
                    << Logger::eol;
    loadTask->process();
  }

  if (options.generateOutputProject) {
    const QString outDir = options.outputDirectory.absolutePath();
    const QString projectFilePath = QDir::cleanPath(outDir + "/project.ScanTailor");
    Logger::debug() << "CmdRunner::run(): Generate output project file '" << projectFilePath.toStdString() << "'"
                    << Logger::eol;
    OutputFileNameGenerator outFileNameGen{make_intrusive<FileNameDisambiguator>(), outDir,
                                           Qt::LayoutDirection::LeftToRight};
    ::cli::ProjectWriter writer(pages, outFileNameGen);
    writer.write(projectFilePath, tasks);
  }

  return 0;
}
