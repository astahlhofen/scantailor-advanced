// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CLI_TASKS_SELECTCONTENTTASK_H_
#define SCANTAILOR_CLI_TASKS_SELECTCONTENTTASK_H_

#include "core/FilterData.h"
#include "core/filters/select_content/Settings.h"
#include "foundation/TaskStatus.h"
#include "foundation/intrusive_ptr.h"
#include "foundation/ref_countable.h"
#include "imageproc/DebugImages.h"
#include "tasks/PageLayoutTask.h"

namespace select_content {
namespace cli {

class Task : public ref_countable {
 public:
  Task(intrusive_ptr<Settings> settings,
       const PageId& pageId,
       intrusive_ptr<page_layout::cli::Task> nextTask,
       bool batch,
       bool debug);
  ~Task() override;

  bool process(const TaskStatus& status, const FilterData& data);

 private:
  intrusive_ptr<page_layout::cli::Task> m_nextTask;
  intrusive_ptr<Settings> m_settings;
  std::unique_ptr<DebugImages> m_dbg;
  PageId m_pageId;
  bool m_batchProcessing;
};

}  // namespace cli
}  // namespace select_content

#endif  // SCANTAILOR_CLI_TASKS_SELECTCONTENTTASK_H_
