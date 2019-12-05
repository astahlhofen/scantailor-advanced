// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.


#ifndef SCANTAILOR_CLI_TASKS_PAGELAYOUTTASK_H_
#define SCANTAILOR_CLI_TASKS_PAGELAYOUTTASK_H_

#include "core/filters/page_layout/Settings.h"
#include "foundation/TaskStatus.h"
#include "foundation/intrusive_ptr.h"
#include "foundation/ref_countable.h"
#include "tasks/OutputTask.h"

namespace page_layout {
namespace cli {

class Task : public ref_countable {
 public:
  Task(intrusive_ptr<Settings> settings,
       const PageId& pageId,
       intrusive_ptr<output::cli::Task> nextTask,
       bool batch,
       bool debug);
  ~Task() override;

  bool process(const TaskStatus& status, const FilterData& data, const QRectF& pageRect, const QRectF& contentRect);

 private:
  intrusive_ptr<Settings> m_settings;
  PageId m_pageId;
  intrusive_ptr<output::cli::Task> m_nextTask;
  bool m_batchProcessing;
};

}  // namespace cli
}  // namespace page_layout

#endif  // SCANTAILOR_CLI_TASKS_PAGELAYOUTTASK_H_
