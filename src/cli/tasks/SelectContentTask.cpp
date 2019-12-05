// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.


#include "SelectContentTask.h"
#include "core/DebugImagesImpl.h"
#include "core/filters/select_content/ContentBoxFinder.h"
#include "core/filters/select_content/PageFinder.h"
#include "core/filters/select_content/PhysSizeCalc.h"
#include "core/logger/Logger.h"


namespace select_content {
namespace cli {
Task::Task(intrusive_ptr<Settings> settings,
           const PageId& pageId,
           intrusive_ptr<page_layout::cli::Task> nextTask,
           bool batch,
           bool debug)
    : m_settings(std::move(settings)), m_pageId(pageId), m_nextTask(std::move(nextTask)), m_batchProcessing(batch) {
  if (debug) {
    m_dbg = std::make_unique<DebugImagesImpl>();
  }
}

Task::~Task() = default;

bool Task::process(const TaskStatus& status, const FilterData& data) {
  status.throwIfCancelled();

  Logger::debug() << "Task::process(): Search the content box of image with id " << m_pageId.imageId().page() << " ("
                  << m_pageId.imageId().filePath().toStdString() << ")" << Logger::eol;

  std::unique_ptr<Params> params(m_settings->getPageParams(m_pageId));
  const Dependencies deps = (params) ? Dependencies(data.xform().resultingPreCropArea(), params->contentDetectionMode(),
                                                    params->pageDetectionMode(), params->isFineTuningEnabled())
                                     : Dependencies(data.xform().resultingPreCropArea());

  Params newParams(deps);
  if (params) {
    newParams = *params;
    newParams.setDependencies(deps);
  }

  const PhysSizeCalc physSizeCalc(data.xform());

  bool needUpdateContentBox = false;
  bool needUpdatePageBox = false;

  if (!params || !deps.compatibleWith(params->dependencies(), &needUpdateContentBox, &needUpdatePageBox)) {
    QRectF pageRect(newParams.pageRect());
    QRectF contentRect(newParams.contentRect());

    if (needUpdatePageBox) {
      if (newParams.pageDetectionMode() == MODE_AUTO) {
        pageRect
            = PageFinder::findPageBox(status, data, newParams.isFineTuningEnabled(), m_settings->pageDetectionBox(),
                                      m_settings->pageDetectionTolerance(), m_dbg.get());
      } else if (newParams.pageDetectionMode() == MODE_DISABLED) {
        pageRect = data.xform().resultingRect();
      }

      if (!data.xform().resultingRect().intersected(pageRect).isValid()) {
        pageRect = data.xform().resultingRect();
      }

      // Force update the content box if it doesn't fit into the page box updated.
      if (contentRect.isValid() && (contentRect.intersected(pageRect) != contentRect)) {
        needUpdateContentBox = true;
      }

      newParams.setPageRect(pageRect);
    }

    if (needUpdateContentBox) {
      if (newParams.contentDetectionMode() == MODE_AUTO) {
        contentRect = ContentBoxFinder::findContentBox(status, data, pageRect, m_dbg.get());
      } else if (newParams.contentDetectionMode() == MODE_DISABLED) {
        contentRect = pageRect;
      }

      if (contentRect.isValid()) {
        contentRect &= pageRect;
      }

      newParams.setContentRect(contentRect);
      newParams.setContentSizeMM(physSizeCalc.sizeMM(contentRect));
    }
  }

  //  OptionsWidget::UiData uiData;
  //  uiData.setSizeCalc(physSizeCalc);
  //  uiData.setContentRect(newParams.contentRect());
  //  uiData.setPageRect(newParams.pageRect());
  //  uiData.setDependencies(deps);
  //  uiData.setContentDetectionMode(newParams.contentDetectionMode());
  //  uiData.setPageDetectionMode(newParams.pageDetectionMode());
  //  uiData.setFineTuneCornersEnabled(newParams.isFineTuningEnabled());

  m_settings->setPageParams(m_pageId, newParams);

  status.throwIfCancelled();

  if (m_nextTask) {
    return m_nextTask->process(status, FilterData(data, data.xform()), newParams.pageRect(), newParams.contentRect());
  }
  return false;
}

}  // namespace cli
}  // namespace select_content
