// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.


#include "SelectContentTask.h"
#include "cli/ProjectWriter.h"
#include "core/DebugImagesImpl.h"
#include "core/filters/select_content/ContentBoxFinder.h"
#include "core/filters/select_content/PageFinder.h"
#include "core/filters/select_content/PhysSizeCalc.h"
#include "core/logger/Logger.h"
#include "foundation/Utils.h"
#include "foundation/XmlMarshaller.h"

namespace cli {
namespace select_content {

Task::Task(intrusive_ptr<::select_content::Settings> settings,
           const PageId& pageId,
           intrusive_ptr<::cli::page_layout::Task> nextTask,
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

  std::unique_ptr<::select_content::Params> params(m_settings->getPageParams(m_pageId));
  const ::select_content::Dependencies deps
      = (params) ? ::select_content::Dependencies(data.xform().resultingPreCropArea(), params->contentDetectionMode(),
                                                  params->pageDetectionMode(), params->isFineTuningEnabled())
                 : ::select_content::Dependencies(data.xform().resultingPreCropArea());

  ::select_content::Params newParams(deps);
  if (params) {
    newParams = *params;
    newParams.setDependencies(deps);
  }

  const ::select_content::PhysSizeCalc physSizeCalc(data.xform());

  bool needUpdateContentBox = false;
  bool needUpdatePageBox = false;

  if (!params || !deps.compatibleWith(params->dependencies(), &needUpdateContentBox, &needUpdatePageBox)) {
    QRectF pageRect(newParams.pageRect());
    QRectF contentRect(newParams.contentRect());

    if (needUpdatePageBox) {
      if (newParams.pageDetectionMode() == MODE_AUTO) {
        pageRect = ::select_content::PageFinder::findPageBox(status, data, newParams.isFineTuningEnabled(),
                                                             m_settings->pageDetectionBox(),
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
        contentRect = ::select_content::ContentBoxFinder::findContentBox(status, data, pageRect, m_dbg.get());
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

  m_settings->setPageParams(m_pageId, newParams);

  status.throwIfCancelled();

  if (m_nextTask) {
    return m_nextTask->process(status, FilterData(data, data.xform()), newParams.pageRect(), newParams.contentRect());
  }
  return false;
}

QDomElement Task::saveSettings(const ::cli::ProjectWriter& writer, QDomDocument& doc) const {
  QDomElement filterEl = QDomElement(doc.createElement(getName()));
  filterEl.appendChild(XmlMarshaller(doc).sizeF(m_settings->pageDetectionBox(), "page-detection-box"));
  filterEl.setAttribute("pageDetectionTolerance",
                        ::foundation::Utils::doubleToString(m_settings->pageDetectionTolerance()));

  writer.enumPages(
      [&](const PageId& pageId, int numericId) { this->writePageSettings(doc, filterEl, pageId, numericId); });

  return filterEl;
}

QString Task::getName() const {
  return "select-content";
}

void Task::writePageSettings(QDomDocument& doc, QDomElement& filterEl, const PageId& pageId, int numericId) const {
  const std::unique_ptr<::select_content::Params> params(m_settings->getPageParams(pageId));
  if (!params) {
    return;
  }

  QDomElement pageEl(doc.createElement("page"));
  pageEl.setAttribute("id", numericId);
  pageEl.appendChild(params->toXml(doc, "params"));

  filterEl.appendChild(pageEl);
}

}  // namespace select_content
}  // namespace cli
