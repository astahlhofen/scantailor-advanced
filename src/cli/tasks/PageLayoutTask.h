// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.


#ifndef SCANTAILOR_CLI_TASKS_PAGELAYOUTTASK_H_
#define SCANTAILOR_CLI_TASKS_PAGELAYOUTTASK_H_

#include "core/filters/page_layout/Settings.h"
#include "foundation/TaskStatus.h"
#include "foundation/intrusive_ptr.h"
#include "tasks/AbstractTask.h"
#include "tasks/OutputTask.h"

namespace cli {
namespace page_layout {

class Task : public AbstractTask {
 public:
  Task(intrusive_ptr<::page_layout::Settings> settings,
       const PageId& pageId,
       intrusive_ptr<::cli::output::Task> nextTask,
       bool batch,
       bool debug);
  ~Task() override;

  bool process(const TaskStatus& status, const FilterData& data, const QRectF& pageRect, const QRectF& contentRect);

  // ##############################################################################################
  // # AbstractTask implementation
  // ##############################################################################################
 public:
  QDomElement saveSettings(const ::cli::ProjectWriter& writer, QDomDocument& doc) const override;

 private:
  void writePageSettings(QDomDocument& doc, QDomElement& filterEl, const PageId& pageId, int numericId) const;

 private:
  intrusive_ptr<::page_layout::Settings> m_settings;
  PageId m_pageId;
  intrusive_ptr<::cli::output::Task> m_nextTask;
  bool m_batchProcessing;
};

}  // namespace page_layout
}  // namespace cli

#endif  // SCANTAILOR_CLI_TASKS_PAGELAYOUTTASK_H_
