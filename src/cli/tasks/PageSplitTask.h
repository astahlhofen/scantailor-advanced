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
#include "foundation/ref_countable.h"
#include "imageproc/DebugImages.h"
#include "tasks/DeskewTask.h"

namespace page_split {
namespace cli {

class Task : public ref_countable {
 public:
  Task(intrusive_ptr<Settings> settings,
       intrusive_ptr<ProjectPages> pages,
       intrusive_ptr<deskew::cli::Task> nextTask,
       const PageInfo& pageInfo,
       bool batchProcessing,
       bool debug);
  ~Task() override;

  bool process(const TaskStatus& status, const FilterData& data);

 private:
  intrusive_ptr<Settings> m_settings;
  intrusive_ptr<ProjectPages> m_pages;
  intrusive_ptr<deskew::cli::Task> m_nextTask;
  std::unique_ptr<DebugImages> m_dbg;
  PageInfo m_pageInfo;
  bool m_batchProcessing;
};

}  // namespace cli
}  // namespace page_split

#endif  // SCANTAILOR_CLI_TASKS_PAGESPLITTASK_H_
