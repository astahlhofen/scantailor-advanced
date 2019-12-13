// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.


#include "DeskewTask.h"
#include <memory>
#include "core/BlackOnWhiteEstimator.h"
#include "core/DebugImagesImpl.h"
#include "core/filters/deskew/Params.h"
#include "core/logger/Logger.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/Connectivity.h"
#include "imageproc/Dpm.h"
#include "imageproc/GrayImage.h"
#include "imageproc/Grayscale.h"
#include "imageproc/Morphology.h"
#include "imageproc/OrthogonalRotation.h"
#include "imageproc/PolygonRasterizer.h"
#include "imageproc/RasterOp.h"
#include "imageproc/ReduceThreshold.h"
#include "imageproc/SeedFill.h"
#include "imageproc/SkewFinder.h"
#include "imageproc/UpscaleIntegerTimes.h"

namespace cli {
namespace deskew {

Task::Task(intrusive_ptr<::deskew::Settings> settings,
           intrusive_ptr<ImageSettings> imageSettings,
           intrusive_ptr<::cli::select_content::Task> nextTask,
           const PageId& pageId,
           bool batchProcessing,
           bool debug)
    : m_settings(std::move(settings)),
      m_imageSettings(std::move(imageSettings)),
      m_nextTask(std::move(nextTask)),
      m_pageId(pageId),
      m_batchProcessing(batchProcessing) {
  if (debug) {
    m_dbg = std::make_unique<DebugImagesImpl>();
  }
}

Task::~Task() = default;

bool Task::process(const TaskStatus& status, FilterData data) {
  status.throwIfCancelled();

  Logger::debug() << "Task::process(): Deskewing the text of image with id " << m_pageId.imageId().page() << " ("
                  << m_pageId.imageId().filePath().toStdString() << ")" << Logger::eol;

  const ::deskew::Dependencies deps(data.xform().preCropArea(), data.xform().preRotation());

  std::unique_ptr<::deskew::Params> params(m_settings->getPageParams(m_pageId));
  updateFilterData(status, data, (!params || !deps.matches(params->dependencies())));

  double skewAngle = 0;
  if (!params) {
    const QRectF imageArea(data.xform().transformBack().mapRect(data.xform().resultingRect()));
    const QRect boundedImageArea(imageArea.toRect().intersected(data.origImage().rect()));

    status.throwIfCancelled();

    if (boundedImageArea.isValid()) {
      imageproc::BinaryImage rotatedImage(orthogonalRotation(
          imageproc::BinaryImage(data.grayImageBlackOnWhite(), boundedImageArea, data.bwThresholdBlackOnWhite()),
          data.xform().preRotation().toDegrees()));
      if (m_dbg) {
        m_dbg->add(rotatedImage, "bw_rotated");
      }

      const QSize unrotatedDpm(Dpm(data.origImage()).toSize());
      const Dpm rotatedDpm(data.xform().preRotation().rotate(unrotatedDpm));
      cleanup(status, rotatedImage, Dpi(rotatedDpm));
      if (m_dbg) {
        m_dbg->add(rotatedImage, "after_cleanup");
      }

      status.throwIfCancelled();

      imageproc::SkewFinder skewFinder;
      skewFinder.setResolutionRatio((double) rotatedDpm.horizontal() / rotatedDpm.vertical());
      const imageproc::Skew skew(skewFinder.findSkew(rotatedImage));

      if (skew.confidence() >= imageproc::Skew::GOOD_CONFIDENCE) {
        skewAngle = -skew.angle();
      }

      ::deskew::Params newParams(skewAngle, deps, MODE_AUTO);
      m_settings->setPageParams(m_pageId, newParams);

      status.throwIfCancelled();
    }
  }

  ImageTransformation newXform(data.xform());
  newXform.setPostRotation(skewAngle);

  if (m_nextTask) {
    return m_nextTask->process(status, FilterData(data, newXform));
  }
  return false;
}

void Task::cleanup(const TaskStatus& status, imageproc::BinaryImage& image, const Dpi& dpi) {
  // We don't have to clean up every piece of garbage.
  // The only concern are the horizontal shadows, which we remove here.

  Dpi reducedDpi(dpi);
  imageproc::BinaryImage reducedImage;

  {
    imageproc::ReduceThreshold reductor(image);
    while (reducedDpi.horizontal() >= 200 && reducedDpi.vertical() >= 200) {
      reductor.reduce(2);
      reducedDpi = Dpi(reducedDpi.horizontal() / 2, reducedDpi.vertical() / 2);
    }
    reducedImage = reductor.image();
  }

  status.throwIfCancelled();

  const QSize brick(from150dpi(QSize(200, 14), reducedDpi));
  imageproc::BinaryImage opened(imageproc::openBrick(reducedImage, brick, imageproc::BLACK));
  reducedImage.release();

  status.throwIfCancelled();

  imageproc::BinaryImage seed(imageproc::upscaleIntegerTimes(opened, image.size(), imageproc::WHITE));
  opened.release();

  status.throwIfCancelled();

  imageproc::BinaryImage garbage(imageproc::seedFill(seed, image, imageproc::CONN8));
  seed.release();

  status.throwIfCancelled();

  imageproc::rasterOp<imageproc::RopSubtract<imageproc::RopDst, imageproc::RopSrc>>(image, garbage);
}

int Task::from150dpi(int size, int targetDpi) {
  const int newSize = (size * targetDpi + 75) / 150;
  if (newSize < 1) {
    return 1;
  }

  return newSize;
}

QSize Task::from150dpi(const QSize& size, const Dpi& targetDpi) {
  const int width = from150dpi(size.width(), targetDpi.horizontal());
  const int height = from150dpi(size.height(), targetDpi.vertical());

  return QSize(width, height);
}

void Task::updateFilterData(const TaskStatus& status, FilterData& data, bool needUpdate) {
  const std::unique_ptr<ImageSettings::PageParams> params = m_imageSettings->getPageParams(m_pageId);
  if (!needUpdate && params) {
    data.updateImageParams(*params);
  } else {
    const imageproc::GrayImage& img = data.grayImage();
    imageproc::BinaryImage mask(img.size(), imageproc::BLACK);
    imageproc::PolygonRasterizer::fillExcept(mask, imageproc::WHITE, data.xform().resultingPreCropArea(),
                                             Qt::WindingFill);
    bool isBlackOnWhite = true;
    //    if (ApplicationSettings::getInstance().isBlackOnWhiteDetectionEnabled()) {
    //      isBlackOnWhite = BlackOnWhiteEstimator::isBlackOnWhite(data.grayImage(), data.xform(), status, m_dbg.get());
    //    }
    isBlackOnWhite = BlackOnWhiteEstimator::isBlackOnWhite(data.grayImage(), data.xform(), status, m_dbg.get());
    ImageSettings::PageParams newParams(
        imageproc::BinaryThreshold::otsuThreshold(imageproc::GrayscaleHistogram(img, mask)), isBlackOnWhite);

    m_imageSettings->setPageParams(m_pageId, newParams);
    data.updateImageParams(newParams);
  }
}

QDomElement Task::saveSettings(const ::cli::ProjectWriter& writer, QDomDocument& doc) const {
  QDomElement filterEl = QDomElement(doc.createElement(getName()));
  writer.enumPages(
      [&](const PageId& pageId, const int numericId) { this->writeParams(doc, filterEl, pageId, numericId); });

  saveImageSettings(writer, doc, filterEl);
  return filterEl;
}

QString Task::getName() const {
  return "deskew";
}

void Task::saveImageSettings(const ProjectWriter& writer, QDomDocument& doc, QDomElement& filterEl) const {
  QDomElement imageSettingsEl(doc.createElement("image-settings"));
  writer.enumPages([&](const PageId& pageId, const int numericId) {
    this->writeImageParams(doc, imageSettingsEl, pageId, numericId);
  });

  filterEl.appendChild(imageSettingsEl);
}

void Task::writeParams(QDomDocument& doc, QDomElement& filterEl, const PageId& pageId, int numericId) const {
  const std::unique_ptr<::deskew::Params> params(m_settings->getPageParams(pageId));
  if (!params) {
    return;
  }

  QDomElement pageEl(doc.createElement("page"));
  pageEl.setAttribute("id", numericId);
  pageEl.appendChild(params->toXml(doc, "params"));

  filterEl.appendChild(pageEl);
}

void Task::writeImageParams(QDomDocument& doc, QDomElement& filterEl, const PageId& pageId, int numericId) const {
  const std::unique_ptr<ImageSettings::PageParams> params(m_imageSettings->getPageParams(pageId));
  if (!params) {
    return;
  }

  QDomElement pageEl(doc.createElement("page"));
  pageEl.setAttribute("id", numericId);
  pageEl.appendChild(params->toXml(doc, "image-params"));

  filterEl.appendChild(pageEl);
}

}  // namespace deskew
}  // namespace cli
