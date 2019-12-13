// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "CLIRunner.h"
#include <algorithm>
#include <iostream>
#include <vector>
#include "Debug.h"
#include "cli/ProjectWriter.h"
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

namespace cli {

CLIRunner::CLIRunner() : m_parser{} {}

CLIRunner::~CLIRunner() {}

int CLIRunner::run(int argc, char** argv) {
  // Initialize the logger.
  Logger::instance()->addMessageWriter(new StandardMessageWriter());

  // Parse command line arguments.
  ::cli::ParserResult result = m_parser.process(argc, argv);
  if (result == ::cli::ParserResult::HelpRequested || result == ::cli::ParserResult::VersionRequested) {
    exit(0);
  } else if (result == ::cli::ParserResult::Error) {
    exit(1);
  }

  // Convert the parsed QFileInfos into ImageFileInfos.
  std::vector<ImageFileInfo> imageFileInfos;
  std::for_each(m_parser.inputFiles().begin(), m_parser.inputFiles().end(), [&](const QFileInfo& _fileInfo) {
    std::vector<ImageMetadata> perPageMetadata;
    const ImageMetadataLoader::Status st = ImageMetadataLoader::load(
        _fileInfo.absoluteFilePath(), [&](const ImageMetadata& _metadata) { perPageMetadata.push_back(_metadata); });

    if (st == ImageMetadataLoader::LOADED) {
      imageFileInfos.emplace_back(_fileInfo, perPageMetadata);
    } else {
      Logger::error() << "ERROR: Failed to load image file '" << _fileInfo.absoluteFilePath().toStdString()
                      << "'. Maybe the specified file is corrupt or no supported image type." << Logger::eol;
      std::exit(1);
    }
  });
  std::sort(imageFileInfos.begin(), imageFileInfos.end(), [](const ImageFileInfo& _lhs, const ImageFileInfo& _rhs) {
    return SmartFilenameOrdering()(_lhs.fileInfo(), _rhs.fileInfo());
  });
  ::cli::debug::logImageFileInfos("CLIRunner::run()", imageFileInfos);
  checkInputImages(imageFileInfos);
  ::cli::debug::logImageFileInfos("CLIRunner::run()", imageFileInfos);

  // Create the project pages.
  auto pages = make_intrusive<ProjectPages>(imageFileInfos, ProjectPages::ONE_PAGE, Qt::LeftToRight);
  PageSequence pageSequence = pages->toPageSequence(PageView::PAGE_VIEW);
  Logger::debug() << "CLIRunner::run(): Number of pages is " << pageSequence.numPages() << Logger::eol;

  using AbstractTaskPtr = intrusive_ptr<::cli::AbstractTask>;
  std::vector<AbstractTaskPtr> tasks{};

  auto imageSettings = make_intrusive<ImageSettings>();
  bool isBatch = true;
  bool isDebug = false;

  // Output tasks.
  auto outputTasks = getOutputTasks(pageSequence, isBatch, isDebug);
  tasks.insert(tasks.begin(), outputTasks.begin(), outputTasks.end());

  // Page layout tasks.
  auto pageLayoutTasks = getPageLayoutTasks(outputTasks, pageSequence, isBatch, isDebug);
  tasks.insert(tasks.begin(), pageLayoutTasks.begin(), pageLayoutTasks.end());

  // Select content tasks.
  auto selectContentTasks = getSelectContentTasks(pageLayoutTasks, pageSequence, isBatch, isDebug);
  tasks.insert(tasks.begin(), selectContentTasks.begin(), selectContentTasks.end());

  // Deskew tasks.
  auto deskewTasks = getDeskewTasks(selectContentTasks, imageSettings, pageSequence, isBatch, isDebug);
  tasks.insert(tasks.begin(), deskewTasks.begin(), deskewTasks.end());

  // Page split tasks.
  auto pageSplitTasks = getPageSplitTasks(deskewTasks, pages, pageSequence, isBatch, isDebug);
  tasks.insert(tasks.begin(), pageSplitTasks.begin(), pageSplitTasks.end());

  // Fix orientation tasks.
  auto fixOrientationTasks = getFixOrientationTasks(pageSplitTasks, imageSettings, pageSequence, isBatch, isDebug);
  tasks.insert(tasks.begin(), fixOrientationTasks.begin(), fixOrientationTasks.end());

  // Load file tasks.
  size_t idx = 0;
  for (auto it = pageSequence.begin(); it != pageSequence.end(); it++) {
    PageInfo& currentPage = *it;
    auto loadTask = make_intrusive<::cli::fix_orientation::LoadFileTask>(currentPage, pages, fixOrientationTasks[idx]);

    // Start processing.
    Logger::debug() << "CLIRunner::run(): Start processing pipeline for page with id " << currentPage.imageId().page()
                    << Logger::eol;
    loadTask->process();

    idx++;
  }

  if (m_parser.generateProjectFile()) {
    const QString outDir = m_parser.outputDir().absolutePath();
    const QString projectFilePath = QDir::cleanPath(outDir + "/project.ScanTailor");
    Logger::debug() << "CLIRunner::run(): Generate output project file '" << projectFilePath.toStdString() << "'"
                    << Logger::eol;
    OutputFileNameGenerator outFileNameGen{make_intrusive<FileNameDisambiguator>(), outDir,
                                           Qt::LayoutDirection::LeftToRight};
    ::cli::ProjectWriter writer(pages, outFileNameGen);
    writer.write(projectFilePath, tasks);
  }

  return 0;
}

void CLIRunner::checkInputImages(std::vector<ImageFileInfo>& inputImages) {
  Logger::debug() << "CLIRunner::checkInputImages(): Check the given DPI inside the metadata of the input images."
                  << Logger::eol;
  for (auto it = inputImages.begin(); it != inputImages.end(); it++) {
    ImageFileInfo& info = *it;
    std::for_each(info.imageInfo().begin(), info.imageInfo().end(), [&info, this](ImageMetadata& _metadata) {
      if (!_metadata.isDpiOK() && !m_parser.customFixDpi()) {
        Dpi dpi = _metadata.dpi();
        Logger::warning() << "CLIRunner::checkInputImages(): The dpi (" << dpi.horizontal() << " x " << dpi.vertical()
                          << ") of the image '" << info.fileInfo().absoluteFilePath().toStdString()
                          << "' seems to be not okay and no input dpi is specified." << Logger::eol;
      } else if (!_metadata.isDpiOK()) {
        _metadata.setDpi(m_parser.customDpi());
        Dpi dpi = m_parser.customDpi();
        Logger::info() << "CLIRunner::checkInputImages(): Fix dpi of image '"
                       << info.fileInfo().absoluteFilePath().toStdString() << "' to (" << dpi.horizontal() << " x "
                       << dpi.vertical() << ")" << Logger::eol;
      }
    });
  }
}

CLIRunner::TaskVector<::cli::output::Task> CLIRunner::getOutputTasks(const PageSequence& pages,
                                                                     bool isBatch,
                                                                     bool isDebug) {
  auto outputSettings = make_intrusive<::output::Settings>();

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
  outputSplittingOptions.setSplittingMode(::output::SplittingMode::BLACK_AND_WHITE_FOREGROUND);
  outputSplittingOptions.setOriginalBackgroundEnabled(true);

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

  ::output::OutputProcessingParams outputProcessingParams{};
  outputProcessingParams.setBlackOnWhiteSetManually(true);

  const QString outDir = m_parser.outputDir().absolutePath();
  OutputFileNameGenerator outFileNameGen{make_intrusive<FileNameDisambiguator>(), outDir,
                                         Qt::LayoutDirection::LeftToRight};

  TaskVector<::cli::output::Task> outputTasks;
  for (auto it = pages.begin(); it != pages.end(); it++) {
    const PageId& pageId = (*it).id();
    outputSettings->setParams(pageId, outputPageParams);
    outputSettings->setOutputProcessingParams(pageId, outputProcessingParams);
    outputTasks.push_back(
        make_intrusive<::cli::output::Task>(outputSettings, pageId, outFileNameGen, isBatch, isDebug));
  }
  return outputTasks;
}

CLIRunner::TaskVector<::cli::page_layout::Task> CLIRunner::getPageLayoutTasks(
    const TaskVector<::cli::output::Task>& outputTasks,
    const PageSequence& pages,
    bool isBatch,
    bool isDebug) {
  auto pageLayoutSettings = make_intrusive<::page_layout::Settings>();
  ::page_layout::Params pageLayoutPageParams{
      {0, 0, 0, 0},                                                        // margins
      {0, 0, 0, 0},                                                        // pageRect
      {0, 0, 0, 0},                                                        // contentRect
      {0, 0},                                                              // contentSizeMM
      {::page_layout::Alignment::VAUTO, ::page_layout::Alignment::HAUTO},  // Alignment
      true                                                                 // autoMargins
  };
  TaskVector<::cli::page_layout::Task> pageLayoutTasks;
  size_t idx = 0;
  for (auto it = pages.begin(); it != pages.end(); it++) {
    const PageId& pageId = (*it).id();
    pageLayoutSettings->setPageParams(pageId, pageLayoutPageParams);
    pageLayoutTasks.push_back(
        make_intrusive<::cli::page_layout::Task>(pageLayoutSettings, pageId, outputTasks[idx], isBatch, isDebug));
    idx++;
  }
  return pageLayoutTasks;
}

CLIRunner::TaskVector<::cli::select_content::Task> CLIRunner::getSelectContentTasks(
    const TaskVector<::cli::page_layout::Task>& pageLayoutTasks,
    const PageSequence& pages,
    bool isBatch,
    bool isDebug) {
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
  TaskVector<::cli::select_content::Task> selectContentTasks;
  size_t idx = 0;
  for (auto it = pages.begin(); it != pages.end(); it++) {
    const PageId& pageId = (*it).id();
    selectContentSettings->setPageParams(pageId, selectContentPageParams);
    selectContentTasks.push_back(make_intrusive<::cli::select_content::Task>(selectContentSettings, pageId,
                                                                             pageLayoutTasks[idx], isBatch, isDebug));
    idx++;
  }
  return selectContentTasks;
}

CLIRunner::TaskVector<::cli::deskew::Task> CLIRunner::getDeskewTasks(
    const TaskVector<::cli::select_content::Task>& selectContentTasks,
    intrusive_ptr<ImageSettings> imageSettings,
    const PageSequence& pages,
    bool isBatch,
    bool isDebug) {
  auto deskewSettings = make_intrusive<::deskew::Settings>();

  TaskVector<::cli::deskew::Task> deskewTasks;
  size_t idx = 0;
  for (auto it = pages.begin(); it != pages.end(); it++) {
    const PageId& pageId = (*it).id();
    deskewTasks.push_back(make_intrusive<::cli::deskew::Task>(deskewSettings, imageSettings, selectContentTasks[idx],
                                                              pageId, isBatch, isDebug));
    idx++;
  }
  return deskewTasks;
}

CLIRunner::TaskVector<::cli::page_split::Task> CLIRunner::getPageSplitTasks(
    const TaskVector<::cli::deskew::Task>& deskewTasks,
    intrusive_ptr<ProjectPages> projectPages,
    const PageSequence& pages,
    bool isBatch,
    bool isDebug) {
  auto pageSplitSettings = make_intrusive<::page_split::Settings>();
  TaskVector<::cli::page_split::Task> pageSplitTasks;
  size_t idx = 0;
  for (auto it = pages.begin(); it != pages.end(); it++) {
    const PageInfo& page = (*it);
    pageSplitTasks.push_back(make_intrusive<::cli::page_split::Task>(pageSplitSettings, projectPages, deskewTasks[idx],
                                                                     page, isBatch, isDebug));
    idx++;
  }
  return pageSplitTasks;
}

CLIRunner::TaskVector<::cli::fix_orientation::Task> CLIRunner::getFixOrientationTasks(
    const TaskVector<::cli::page_split::Task>& pageSplitTasks,
    intrusive_ptr<ImageSettings> imageSettings,
    const PageSequence& pages,
    bool isBatch,
    bool isDebug) {
  auto fixOrientationSettings = make_intrusive<::fix_orientation::Settings>();

  TaskVector<::cli::fix_orientation::Task> fixOrientationTasks;
  size_t idx = 0;
  for (auto it = pages.begin(); it != pages.end(); it++) {
    const PageId& pageId = (*it).id();
    fixOrientationTasks.push_back(make_intrusive<::cli::fix_orientation::Task>(
        pageId, fixOrientationSettings, imageSettings, pageSplitTasks[idx], isBatch));
    idx++;
  }
  return fixOrientationTasks;
}

}  // namespace cli
