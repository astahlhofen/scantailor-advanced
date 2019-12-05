// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.


#include "PageLayoutTask.h"
#include "core/DebugImagesImpl.h"
#include "core/filters/page_layout/Params.h"
#include "core/filters/page_layout/Utils.h"
#include "core/logger/Logger.h"

namespace page_layout {
namespace cli {

Task::Task(intrusive_ptr<Settings> settings,
           const PageId& pageId,
           intrusive_ptr<output::cli::Task> nextTask,
           bool batch,
           bool debug)
    : m_nextTask(std::move(nextTask)), m_settings(std::move(settings)), m_pageId(pageId), m_batchProcessing(batch) {}

Task::~Task() = default;

bool Task::process(const TaskStatus& status,
                   const FilterData& data,
                   const QRectF& pageRect,
                   const QRectF& contentRect) {
  status.throwIfCancelled();

  Logger::debug() << "Task::process(): Determine the page layout of image with id " << m_pageId.imageId().page() << " ("
                  << m_pageId.imageId().filePath().toStdString() << ")" << Logger::eol;

  const QSizeF contentSizeMm(Utils::calcRectSizeMM(data.xform(), contentRect));

  if (m_settings->isPageAutoMarginsEnabled(m_pageId)) {
    const Margins& marginsMm = Utils::calcMarginsMM(data.xform(), pageRect, contentRect);
    m_settings->setHardMarginsMM(m_pageId, marginsMm);
  }

  QSizeF aggHardSizeBefore;
  QSizeF aggHardSizeAfter;
  const Params params(m_settings->updateContentSizeAndGetParams(m_pageId, pageRect, contentRect, contentSizeMm,
                                                                &aggHardSizeBefore, &aggHardSizeAfter));

  const QRectF adaptedContentRect(Utils::adaptContentRect(data.xform(), contentRect));

  if (m_nextTask) {
    const QPolygonF contentRectPhys(data.xform().transformBack().map(adaptedContentRect));
    const QPolygonF pageRectPhys(Utils::calcPageRectPhys(data.xform(), contentRectPhys, params, aggHardSizeAfter));

    ImageTransformation newXform(data.xform());
    newXform.setPostCropArea(Utils::shiftToRoundedOrigin(newXform.transform().map(pageRectPhys)));

    return m_nextTask->process(status, FilterData(data, newXform), contentRectPhys);
  }
  return false;
}

}  // namespace cli
}  // namespace page_layout
