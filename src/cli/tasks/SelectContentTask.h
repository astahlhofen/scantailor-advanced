// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CLI_TASKS_SELECTCONTENTTASK_H_
#define SCANTAILOR_CLI_TASKS_SELECTCONTENTTASK_H_

#include "core/FilterData.h"
#include "core/filters/select_content/Settings.h"
#include "foundation/TaskStatus.h"
#include "foundation/intrusive_ptr.h"
#include "imageproc/DebugImages.h"
#include "tasks/AbstractTask.h"
#include "tasks/PageLayoutTask.h"

namespace cli {
namespace select_content {

class Task : public AbstractTask {
 public:
  Task(intrusive_ptr<::select_content::Settings> settings,
       const PageId& pageId,
       intrusive_ptr<::cli::page_layout::Task> nextTask,
       bool batch,
       bool debug);
  ~Task() override;

  bool process(const TaskStatus& status, const FilterData& data);

  // ##############################################################################################
  // # AbstractTask implementation
  // ##############################################################################################
 public:
  QDomElement saveSettings(const ::cli::ProjectWriter& writer, QDomDocument& doc) const override;

 private:
  void writePageSettings(QDomDocument& doc, QDomElement& filterEl, const PageId& pageId, int numericId) const;

 private:
  intrusive_ptr<::cli::page_layout::Task> m_nextTask;
  intrusive_ptr<::select_content::Settings> m_settings;
  std::unique_ptr<DebugImages> m_dbg;
  PageId m_pageId;
  bool m_batchProcessing;
};

}  // namespace select_content
}  // namespace cli

#endif  // SCANTAILOR_CLI_TASKS_SELECTCONTENTTASK_H_
