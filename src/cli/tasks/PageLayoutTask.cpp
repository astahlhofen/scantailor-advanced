// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.


#include "PageLayoutTask.h"
#include "cli/ProjectWriter.h"
#include "core/DebugImagesImpl.h"
#include "core/filters/page_layout/Guide.h"
#include "core/filters/page_layout/Params.h"
#include "core/filters/page_layout/Utils.h"
#include "core/logger/Logger.h"

namespace cli {
namespace page_layout {

Task::Task(intrusive_ptr<::page_layout::Settings> settings,
           const PageId& pageId,
           intrusive_ptr<::cli::output::Task> nextTask,
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

  const QSizeF contentSizeMm(::page_layout::Utils::calcRectSizeMM(data.xform(), contentRect));

  if (m_settings->isPageAutoMarginsEnabled(m_pageId)) {
    const Margins& marginsMm = ::page_layout::Utils::calcMarginsMM(data.xform(), pageRect, contentRect);
    m_settings->setHardMarginsMM(m_pageId, marginsMm);
  }

  QSizeF aggHardSizeBefore;
  QSizeF aggHardSizeAfter;
  const ::page_layout::Params params(m_settings->updateContentSizeAndGetParams(
      m_pageId, pageRect, contentRect, contentSizeMm, &aggHardSizeBefore, &aggHardSizeAfter));

  const QRectF adaptedContentRect(::page_layout::Utils::adaptContentRect(data.xform(), contentRect));

  if (m_nextTask) {
    const QPolygonF contentRectPhys(data.xform().transformBack().map(adaptedContentRect));
    const QPolygonF pageRectPhys(
        ::page_layout::Utils::calcPageRectPhys(data.xform(), contentRectPhys, params, aggHardSizeAfter));

    ImageTransformation newXform(data.xform());
    newXform.setPostCropArea(::page_layout::Utils::shiftToRoundedOrigin(newXform.transform().map(pageRectPhys)));

    return m_nextTask->process(status, FilterData(data, newXform), contentRectPhys);
  }
  return false;
}

QDomElement Task::saveSettings(const ::cli::ProjectWriter& writer, QDomDocument& doc) const {
  QDomElement filterEl(doc.createElement("page-layout"));

  filterEl.setAttribute("showMiddleRect", m_settings->isShowingMiddleRectEnabled() ? "1" : "0");

  if (!m_settings->guides().empty()) {
    QDomElement guidesEl(doc.createElement("guides"));
    for (const ::page_layout::Guide& guide : m_settings->guides()) {
      guidesEl.appendChild(guide.toXml(doc, "guide"));
    }
    filterEl.appendChild(guidesEl);
  }

  writer.enumPages(
      [&](const PageId& pageId, int numericId) { this->writePageSettings(doc, filterEl, pageId, numericId); });

  return filterEl;
}

void Task::writePageSettings(QDomDocument& doc, QDomElement& filterEl, const PageId& pageId, int numericId) const {
  const std::unique_ptr<::page_layout::Params> params(m_settings->getPageParams(pageId));
  if (!params) {
    return;
  }

  QDomElement pageEl(doc.createElement("page"));
  pageEl.setAttribute("id", numericId);
  pageEl.appendChild(params->toXml(doc, "params"));

  filterEl.appendChild(pageEl);
}

}  // namespace page_layout
}  // namespace cli
