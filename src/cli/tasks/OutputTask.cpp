/*
 * OutputTask.cpp
 *
 *  Created on: 5 Dec 2019
 *  Copyright (C) 2019 Andreas Stahlhofen <astahlhofen@uni-koblenz.de>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "OutputTask.h"
#include <QDir>
#include "core/DebugImagesImpl.h"
#include "core/ImageLoader.h"
#include "core/TiffWriter.h"
#include "core/filters/output/FillZoneComparator.h"
#include "core/filters/output/OutputGenerator.h"
#include "core/filters/output/OutputImageBuilder.h"
#include "core/filters/output/OutputImageWithForeground.h"
#include "core/filters/output/OutputImageWithOriginalBackground.h"
#include "core/filters/output/PictureZoneComparator.h"
#include "core/filters/output/RenderParams.h"
#include "core/filters/output/Utils.h"
#include "core/logger/Logger.h"
#include "dewarping/DistortionModel.h"
#include "imageproc/BinaryImage.h"

namespace output {
namespace cli {

Task::Task(intrusive_ptr<Settings> settings,
           const PageId& pageId,
           const OutputFileNameGenerator& outFileNameGen,
           bool batch,
           bool debug)
    : m_settings(settings),
      m_pageId(pageId),
      m_outFileNameGen(outFileNameGen),
      m_batchProcessing(batch),
      m_debug(debug) {
  if (m_debug) {
    m_dbg = std::make_unique<DebugImagesImpl>();
  }
}

Task::~Task() {}

bool Task::process(const TaskStatus& status, const FilterData& data, const QPolygonF& contentRectPhys) {
  status.throwIfCancelled();

  Logger::debug() << "Task::process(): Generate the output file for image with id " << m_pageId.imageId().page() << " ("
                  << m_pageId.imageId().filePath().toStdString() << ")" << Logger::eol;

  Params params = m_settings->getParams(m_pageId);

  RenderParams renderParams(params.colorParams(), params.splittingOptions());
  const QString outFilePath(m_outFileNameGen.filePathFor(m_pageId));
  const QFileInfo outFileInfo(outFilePath);

  ImageTransformation newXform(data.xform());
  newXform.postScaleToDpi(params.outputDpi());

  const QString foregroundDir(Utils::foregroundDir(m_outFileNameGen.outDir()));
  const QString backgroundDir(Utils::backgroundDir(m_outFileNameGen.outDir()));
  const QString originalBackgroundDir(Utils::originalBackgroundDir(m_outFileNameGen.outDir()));
  const QString foregroundFilePath(QDir(foregroundDir).absoluteFilePath(outFileInfo.fileName()));
  const QString backgroundFilePath(QDir(backgroundDir).absoluteFilePath(outFileInfo.fileName()));
  const QString originalBackgroundFilePath(QDir(originalBackgroundDir).absoluteFilePath(outFileInfo.fileName()));
  const QFileInfo foregroundFileInfo(foregroundFilePath);
  const QFileInfo backgroundFileInfo(backgroundFilePath);
  const QFileInfo originalBackgroundFileInfo(originalBackgroundFilePath);

  const QString automaskDir(Utils::automaskDir(m_outFileNameGen.outDir()));
  const QString automaskFilePath(QDir(automaskDir).absoluteFilePath(outFileInfo.fileName()));
  QFileInfo automaskFileInfo(automaskFilePath);

  const QString specklesDir(Utils::specklesDir(m_outFileNameGen.outDir()));
  const QString specklesFilePath(QDir(specklesDir).absoluteFilePath(outFileInfo.fileName()));
  QFileInfo specklesFileInfo(specklesFilePath);

  const bool needPictureEditor = renderParams.mixedOutput() && !m_batchProcessing;
  const bool need_speckles_image
      = params.despeckleLevel() != DESPECKLE_OFF && renderParams.needBinarization() && !m_batchProcessing;

  {
    std::unique_ptr<OutputParams> storedOutputParams(m_settings->getOutputParams(m_pageId));
    if (storedOutputParams != nullptr) {
      if (storedOutputParams->outputImageParams().getPictureShapeOptions() != params.pictureShapeOptions()) {
        // if picture shape options changed, reset auto picture zones
        OutputProcessingParams outputProcessingParams = m_settings->getOutputProcessingParams(m_pageId);
        outputProcessingParams.setAutoZonesFound(false);
        m_settings->setOutputProcessingParams(m_pageId, outputProcessingParams);
      }
    }
  }

  const OutputGenerator generator(newXform, contentRectPhys);

  OutputImageParams newOutputImageParams(generator.outputImageSize(), generator.outputContentRect(), newXform,
                                         params.outputDpi(), params.colorParams(), params.splittingOptions(),
                                         params.dewarpingOptions(), params.distortionModel(), params.depthPerception(),
                                         params.despeckleLevel(), params.pictureShapeOptions(),
                                         m_settings->getOutputProcessingParams(m_pageId), params.isBlackOnWhite());

  ZoneSet newPictureZones(m_settings->pictureZonesForPage(m_pageId));
  const ZoneSet newFillZones(m_settings->fillZonesForPage(m_pageId));

  bool needReprocess = false;
  do {  // Just to be able to break from it.
    std::unique_ptr<OutputParams> storedOutputParams(m_settings->getOutputParams(m_pageId));

    if (!storedOutputParams) {
      needReprocess = true;
      break;
    }

    if (!storedOutputParams->outputImageParams().matches(newOutputImageParams)) {
      needReprocess = true;
      break;
    }

    if (!PictureZoneComparator::equal(storedOutputParams->pictureZones(), newPictureZones)) {
      needReprocess = true;
      break;
    }

    if (!FillZoneComparator::equal(storedOutputParams->fillZones(), newFillZones)) {
      needReprocess = true;
      break;
    }

    if (!renderParams.splitOutput()) {
      if (!outFileInfo.exists()) {
        needReprocess = true;
        break;
      }

      if (!storedOutputParams->outputFileParams().matches(OutputFileParams(outFileInfo))) {
        needReprocess = true;
        break;
      }
    } else {
      if (!foregroundFileInfo.exists() || !backgroundFileInfo.exists()) {
        needReprocess = true;
        break;
      }
      if (!(storedOutputParams->foregroundFileParams().matches(OutputFileParams(foregroundFileInfo)))
          || !(storedOutputParams->backgroundFileParams().matches(OutputFileParams(backgroundFileInfo)))) {
        needReprocess = true;
        break;
      }

      if (renderParams.originalBackground()) {
        if (!originalBackgroundFileInfo.exists()) {
          needReprocess = true;
          break;
        }
        if (!(storedOutputParams->originalBackgroundFileParams().matches(
                OutputFileParams(originalBackgroundFileInfo)))) {
          needReprocess = true;
          break;
        }
      }
    }

    if (needPictureEditor) {
      if (!automaskFileInfo.exists()) {
        needReprocess = true;
        break;
      }

      if (!storedOutputParams->automaskFileParams().matches(OutputFileParams(automaskFileInfo))) {
        needReprocess = true;
        break;
      }
    }

    if (need_speckles_image) {
      if (!specklesFileInfo.exists()) {
        needReprocess = true;
        break;
      }
      if (!storedOutputParams->specklesFileParams().matches(OutputFileParams(specklesFileInfo))) {
        needReprocess = true;
        break;
      }
    }
  } while (false);

  QImage outImg;
  imageproc::BinaryImage automaskImg;
  imageproc::BinaryImage specklesImg;

  if (!needReprocess) {
    QFile outFile(outFilePath);
    if (outFile.open(QIODevice::ReadOnly)) {
      outImg = ImageLoader::load(outFile, 0);
    }
    if (outImg.isNull() && renderParams.splitOutput()) {
      OutputImageBuilder imageBuilder;
      QFile foregroundFile(foregroundFilePath);
      if (foregroundFile.open(QIODevice::ReadOnly)) {
        imageBuilder.setForegroundImage(ImageLoader::load(foregroundFile, 0));
      }
      QFile backgroundFile(backgroundFilePath);
      if (backgroundFile.open(QIODevice::ReadOnly)) {
        imageBuilder.setBackgroundImage(ImageLoader::load(backgroundFile, 0));
      }
      if (renderParams.originalBackground()) {
        QFile originalBackgroundFile(originalBackgroundFilePath);
        if (originalBackgroundFile.open(QIODevice::ReadOnly)) {
          imageBuilder.setOriginalBackgroundImage(ImageLoader::load(originalBackgroundFile, 0));
        }
      }
      outImg = *imageBuilder.build();
    }
    needReprocess = outImg.isNull();

    if (needPictureEditor && !needReprocess) {
      QFile automaskFile(automaskFilePath);
      if (automaskFile.open(QIODevice::ReadOnly)) {
        automaskImg = imageproc::BinaryImage(ImageLoader::load(automaskFile, 0));
      }
      needReprocess = automaskImg.isNull() || automaskImg.size() != outImg.size();
    }

    if (need_speckles_image && !needReprocess) {
      QFile specklesFile(specklesFilePath);
      if (specklesFile.open(QIODevice::ReadOnly)) {
        specklesImg = imageproc::BinaryImage(ImageLoader::load(specklesFile, 0));
      }
      needReprocess = specklesImg.isNull();
    }
  }

  if (needReprocess) {
    // Even in batch processing mode we should still write automask, because it
    // will be needed when we view the results back in interactive mode.
    // The same applies even more to speckles file, as we need it not only
    // for visualization purposes, but also for re-doing despeckling at
    // different levels without going through the whole output generation process.
    const bool writeAutomask = renderParams.mixedOutput();
    const bool writeSpecklesFile = params.despeckleLevel() != DESPECKLE_OFF && renderParams.needBinarization();

    automaskImg = imageproc::BinaryImage();
    specklesImg = imageproc::BinaryImage();

    // OutputGenerator will write a new distortion model
    // there, if dewarping mode is AUTO.
    dewarping::DistortionModel distortionModel;
    if (params.dewarpingOptions().dewarpingMode() == MANUAL) {
      distortionModel = params.distortionModel();
    }

    bool invalidateParams = false;
    {
      std::unique_ptr<OutputImage> outputImage
          = generator.process(status, data, newPictureZones, newFillZones, distortionModel, params.depthPerception(),
                              writeAutomask ? &automaskImg : nullptr, writeSpecklesFile ? &specklesImg : nullptr,
                              m_dbg.get(), m_pageId, m_settings);

      params = m_settings->getParams(m_pageId);

      if (((params.dewarpingOptions().dewarpingMode() == AUTO)
           || (params.dewarpingOptions().dewarpingMode() == MARGINAL))
          && distortionModel.isValid()) {
        // A new distortion model was generated.
        // We need to save it to be able to modify it manually.
        params.setDistortionModel(distortionModel);
        m_settings->setParams(m_pageId, params);
        newOutputImageParams.setDistortionModel(distortionModel);
      }

      // Saving refreshed params and output processing params.
      newOutputImageParams.setBlackOnWhite(m_settings->getParams(m_pageId).isBlackOnWhite());
      newOutputImageParams.setOutputProcessingParams(m_settings->getOutputProcessingParams(m_pageId));

      if (renderParams.splitOutput()) {
        auto* outputImageWithForeground = dynamic_cast<OutputImageWithForeground*>(outputImage.get());

        QDir().mkdir(foregroundDir);
        QDir().mkdir(backgroundDir);
        if (!TiffWriter::writeImage(foregroundFilePath, outputImageWithForeground->getForegroundImage())
            || !TiffWriter::writeImage(backgroundFilePath, outputImageWithForeground->getBackgroundImage())) {
          invalidateParams = true;
        }

        if (renderParams.originalBackground()) {
          auto* outputImageWithOrigBg = dynamic_cast<OutputImageWithOriginalBackground*>(outputImage.get());

          QDir().mkdir(originalBackgroundDir);
          if (!TiffWriter::writeImage(originalBackgroundFilePath,
                                      outputImageWithOrigBg->getOriginalBackgroundImage())) {
            invalidateParams = true;
          }
        }
      }

      outImg = *outputImage;
    }

    if (!renderParams.originalBackground()) {
      QFile::remove(originalBackgroundFilePath);
    }
    if (!renderParams.splitOutput()) {
      QFile::remove(foregroundFilePath);
      QFile::remove(backgroundFilePath);
    }

    if (!TiffWriter::writeImage(outFilePath, outImg)) {
      invalidateParams = true;
    } else {
      deleteMutuallyExclusiveOutputFiles();
    }

    if (writeSpecklesFile && specklesImg.isNull()) {
      // Even if despeckling didn't actually take place, we still need
      // to write an empty speckles file.  Making it a special case
      // is simply not worth it.
      imageproc::BinaryImage(outImg.size(), imageproc::WHITE).swap(specklesImg);
    }

    if (writeAutomask) {
      // Note that QDir::mkdir() will fail if the parent directory,
      // that is $OUT/cache doesn't exist. We want that behaviour,
      // as otherwise when loading a project from a different machine,
      // a whole bunch of bogus directories would be created.
      QDir().mkdir(automaskDir);
      // Also note that QDir::mkdir() will fail if the directory already exists,
      // so we ignore its return value here.
      if (!TiffWriter::writeImage(automaskFilePath, automaskImg.toQImage())) {
        invalidateParams = true;
      }
    }
    if (writeSpecklesFile) {
      if (!QDir().mkpath(specklesDir)) {
        invalidateParams = true;
      } else if (!TiffWriter::writeImage(specklesFilePath, specklesImg.toQImage())) {
        invalidateParams = true;
      }
    }

    if (invalidateParams) {
      m_settings->removeOutputParams(m_pageId);
    } else {
      // Note that we can't reuse *_file_info objects
      // as we've just overwritten those files.
      const OutputParams outParams(
          newOutputImageParams, OutputFileParams(QFileInfo(outFilePath)),
          renderParams.splitOutput() ? OutputFileParams(QFileInfo(foregroundFilePath)) : OutputFileParams(),
          renderParams.splitOutput() ? OutputFileParams(QFileInfo(backgroundFilePath)) : OutputFileParams(),
          renderParams.originalBackground() ? OutputFileParams(QFileInfo(originalBackgroundFilePath))
                                            : OutputFileParams(),
          writeAutomask ? OutputFileParams(QFileInfo(automaskFilePath)) : OutputFileParams(),
          writeSpecklesFile ? OutputFileParams(QFileInfo(specklesFilePath)) : OutputFileParams(), newPictureZones,
          newFillZones);

      m_settings->setOutputParams(m_pageId, outParams);
    }
  }
  return true;
}

void Task::deleteMutuallyExclusiveOutputFiles() {
  switch (m_pageId.subPage()) {
    case PageId::SINGLE_PAGE:
      QFile::remove(m_outFileNameGen.filePathFor(PageId(m_pageId.imageId(), PageId::LEFT_PAGE)));
      QFile::remove(m_outFileNameGen.filePathFor(PageId(m_pageId.imageId(), PageId::RIGHT_PAGE)));
      break;
    case PageId::LEFT_PAGE:
    case PageId::RIGHT_PAGE:
      QFile::remove(m_outFileNameGen.filePathFor(PageId(m_pageId.imageId(), PageId::SINGLE_PAGE)));
      break;
  }
}

}  // namespace cli
}  // namespace output
