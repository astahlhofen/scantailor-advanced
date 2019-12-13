// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "FixOrientationTask.h"
#include "Debug.h"
#include "core/logger/Logger.h"
#include "foundation/XmlMarshaller.h"
#include "imageproc/BinaryThreshold.h"

namespace cli {
namespace fix_orientation {

Task::Task(const PageId& pageId,
           intrusive_ptr<::fix_orientation::Settings> settings,
           intrusive_ptr<ImageSettings> imageSettings,
           intrusive_ptr<::cli::page_split::Task> nextTask,
           const bool batchProcessing)
    : m_nextTask(std::move(nextTask)),
      m_settings(std::move(settings)),
      m_imageSettings(std::move(imageSettings)),
      m_pageId(pageId),
      m_imageId(m_pageId.imageId()),
      m_batchProcessing(batchProcessing) {}

Task::~Task() = default;

bool Task::process(const TaskStatus& status, FilterData data) {
  // This function is executed from the worker thread.
  status.throwIfCancelled();

  Logger::debug() << "FixOrientationTask::process(): Fix orientation of image with id " << m_imageId.page() << " ("
                  << m_imageId.filePath().toStdString() << ")" << Logger::eol;

  updateFilterData(data);


  ::cli::debug::logImageSettingsForPage(
      "FixOrientationTask::process(): Image settings after updateFilterData(): ", m_imageSettings, m_pageId);

  ImageTransformation xform(data.xform());
  xform.setPreRotation(m_settings->getRotationFor(m_imageId));

  if (m_nextTask) {
    return m_nextTask->process(status, data);
  }
  return false;
}

void Task::updateFilterData(FilterData& data) {
  if (const std::unique_ptr<ImageSettings::PageParams> params = m_imageSettings->getPageParams(m_pageId)) {
    data.updateImageParams(*params);
  } else {
    ImageSettings::PageParams newParams(imageproc::BinaryThreshold::otsuThreshold(data.grayImage()), true);

    m_imageSettings->setPageParams(m_pageId, newParams);
    data.updateImageParams(newParams);
  }
}

QDomElement Task::saveSettings(const ::cli::ProjectWriter& writer, QDomDocument& doc) const {
  QDomElement filterEl = QDomElement(doc.createElement(getName()));
  writer.enumImages(
      [&](const ImageId& imageId, const int numericId) { this->writeParams(doc, filterEl, imageId, numericId); });

  saveImageSettings(writer, doc, filterEl);

  return filterEl;
}

QString Task::getName() const {
  return "fix-orientation";
}

void Task::writeParams(QDomDocument& doc, QDomElement& filterEl, const ImageId& imageId, int numericId) const {
  const OrthogonalRotation rotation(m_settings->getRotationFor(imageId));
  if (rotation.toDegrees() == 0) {
    return;
  }

  XmlMarshaller marshaller(doc);

  QDomElement imageEl(doc.createElement("image"));
  imageEl.setAttribute("id", numericId);
  imageEl.appendChild(rotation.toXml(doc, "rotation"));
  filterEl.appendChild(imageEl);
}

void Task::saveImageSettings(const ProjectWriter& writer, QDomDocument& doc, QDomElement& filterEl) const {
  QDomElement imageSettingsEl(doc.createElement("image-settings"));
  writer.enumPages([&](const PageId& pageId, const int numericId) {
    this->writeImageParams(doc, imageSettingsEl, pageId, numericId);
  });

  filterEl.appendChild(imageSettingsEl);
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

}  // namespace fix_orientation
}  // namespace cli
