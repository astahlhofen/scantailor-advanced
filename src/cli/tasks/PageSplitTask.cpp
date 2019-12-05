// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PageSplitTask.h"
#include "core/filters/page_split/PageLayoutAdapter.h"
#include "core/filters/page_split/PageLayoutEstimator.h"
#include "core/logger/Logger.h"


namespace page_split {
namespace cli {


Task::Task(intrusive_ptr<Settings> settings,
           intrusive_ptr<ProjectPages> pages,
           intrusive_ptr<deskew::cli::Task> nextTask,
           const PageInfo& pageInfo,
           bool batchProcessing,
           bool debug)
    : m_settings(std::move(settings)),
      m_pages(std::move(pages)),
      m_nextTask(std::move(nextTask)),
      m_pageInfo(pageInfo),
      m_batchProcessing(batchProcessing) {}

Task::~Task() = default;

static ProjectPages::LayoutType toPageLayoutType(const PageLayout& layout) {
  switch (layout.type()) {
    case PageLayout::SINGLE_PAGE_UNCUT:
    case PageLayout::SINGLE_PAGE_CUT:
      return ProjectPages::ONE_PAGE_LAYOUT;
    case PageLayout::TWO_PAGES:
      return ProjectPages::TWO_PAGE_LAYOUT;
  }

  assert(!"Unreachable");

  return ProjectPages::ONE_PAGE_LAYOUT;
}

bool Task::process(const TaskStatus& status, const FilterData& data) {
  Logger::debug() << "Task::process(): Running page splitting task for image with id " << m_pageInfo.imageId().page()
                  << " (" << m_pageInfo.imageId().filePath().toStdString() << ")" << Logger::eol;
  status.throwIfCancelled();

  Settings::Record record(m_settings->getPageRecord(m_pageInfo.imageId()));
  Dependencies deps(data.origImage().size(), data.xform().preRotation(), record.combinedLayoutType());

  while (true) {
    const Params* const params = record.params();

    LayoutType newLayoutType = record.combinedLayoutType();
    AutoManualMode splitLineMode = MODE_AUTO;
    PageLayout newLayout;

    if (!params || !deps.compatibleWith(*params)) {
      if (!params || (record.combinedLayoutType() == AUTO_LAYOUT_TYPE)) {
        newLayout = PageLayoutEstimator::estimatePageLayout(record.combinedLayoutType(), data.grayImage(), data.xform(),
                                                            data.bwThreshold(), m_dbg.get());

        status.throwIfCancelled();
      } else {
        newLayout = PageLayoutAdapter::adaptPageLayout(params->pageLayout(), data.xform().resultingRect());
        newLayoutType = newLayout.toLayoutType();
        splitLineMode = params->splitLineMode();
      }
    } else {
      PageLayout correctedPageLayout = params->pageLayout();
      PageLayoutAdapter::correctPageLayoutType(&correctedPageLayout);
      if (correctedPageLayout.type() == params->pageLayout().type()) {
        break;
      } else {
        newLayout = correctedPageLayout;
        newLayoutType = newLayout.toLayoutType();
        splitLineMode = params->splitLineMode();
      }
    }
    deps.setLayoutType(newLayoutType);
    const Params newParams(newLayout, deps, splitLineMode);

    Settings::UpdateAction update;
    update.setLayoutType(newLayoutType);
    update.setParams(newParams);

#ifndef NDEBUG
    {
      Settings::Record updatedRecord(record);
      updatedRecord.update(update);
      assert(!updatedRecord.hasLayoutTypeConflict());
      // This assert effectively verifies that PageLayoutEstimator::estimatePageLayout()
      // returned a layout with of a type consistent with the requested one.
      // If it didn't, it's a bug which will in fact cause a dead loop.
    }
#endif

    bool conflict = false;
    record = m_settings->conditionalUpdate(m_pageInfo.imageId(), update, &conflict);
    if (conflict && !record.params()) {
      // If there was a conflict, it means
      // the record was updated by another
      // thread somewhere between getPageRecord()
      // and conditionalUpdate().  If that
      // external update didn't leave page
      // parameters clear, we are just going
      // to use it's data, otherwise we need
      // to process this page again for the
      // new layout type.
      continue;
    }

    break;
  }

  const PageLayout& layout = record.params()->pageLayout();

  m_pages->setLayoutTypeFor(m_pageInfo.imageId(), toPageLayoutType(layout));

  if (m_nextTask != nullptr) {
    ImageTransformation newXform(data.xform());
    newXform.setPreCropArea(layout.pageOutline(m_pageInfo.id().subPage()).toPolygon());

    return m_nextTask->process(status, FilterData(data, newXform));
  }
  return false;
}


}  // namespace cli
}  // namespace page_split
