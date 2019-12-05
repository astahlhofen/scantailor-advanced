// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.


#include "FixOrientationTask.h"
#include "core/logger/Logger.h"
#include "imageproc/BinaryThreshold.h"

namespace fix_orientation {
namespace cli {

Task::Task(const PageId& pageId,
           intrusive_ptr<Settings> settings,
           intrusive_ptr<ImageSettings> imageSettings,
           intrusive_ptr<page_split::cli::Task> nextTask,
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

  Logger::debug() << "Task::process(): Fix orientation of image with id " << m_imageId.page() << " ("
                  << m_imageId.filePath().toStdString() << ")" << Logger::eol;

  updateFilterData(data);

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

}  // namespace cli
}  // namespace fix_orientation
