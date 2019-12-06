// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.


#ifndef SCANTAILOR_CLI_TASKS_PAGESPLITTASK_H_
#define SCANTAILOR_CLI_TASKS_PAGESPLITTASK_H_

#include <QImage>
#include "core/FilterData.h"
#include "core/ImageTransformation.h"
#include "core/ProjectPages.h"
#include "core/filters/page_split/Settings.h"
#include "foundation/TaskStatus.h"
#include "foundation/intrusive_ptr.h"
#include "imageproc/DebugImages.h"
#include "tasks/AbstractTask.h"
#include "tasks/DeskewTask.h"

namespace cli {
namespace page_split {

class Task : public AbstractTask {
 public:
  Task(intrusive_ptr<::page_split::Settings> settings,
       intrusive_ptr<ProjectPages> pages,
       intrusive_ptr<::cli::deskew::Task> nextTask,
       const PageInfo& pageInfo,
       bool batchProcessing,
       bool debug);
  ~Task() override;

  bool process(const TaskStatus& status, const FilterData& data);

  // ##############################################################################################
  // # AbstractTask implementation
  // ##############################################################################################
 public:
  QDomElement saveSettings(const ::cli::ProjectWriter& writer, QDomDocument& doc) const override;

 private:
  void writeImageSettings(QDomDocument& doc, QDomElement& filterEl, const ImageId& imageId, int numericId) const;

 private:
  intrusive_ptr<::page_split::Settings> m_settings;
  intrusive_ptr<ProjectPages> m_pages;
  intrusive_ptr<::cli::deskew::Task> m_nextTask;
  std::unique_ptr<DebugImages> m_dbg;
  PageInfo m_pageInfo;
  bool m_batchProcessing;
};

}  // namespace page_split
}  // namespace cli

#endif  // SCANTAILOR_CLI_TASKS_PAGESPLITTASK_H_
