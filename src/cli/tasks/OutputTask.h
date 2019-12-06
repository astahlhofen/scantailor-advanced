// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_TASKS_CLI_OUTPUTTASK_H_
#define SCANTAILOR_TASKS_CLI_OUTPUTTASK_H_

#include <QPolygonF>
#include "core/FilterData.h"
#include "core/OutputFileNameGenerator.h"
#include "core/filters/output/Settings.h"
#include "foundation/TaskStatus.h"
#include "foundation/intrusive_ptr.h"
#include "imageproc/DebugImages.h"
#include "tasks/AbstractTask.h"

namespace cli {
namespace output {

class Task : public AbstractTask {
 public:
  Task(intrusive_ptr<::output::Settings> settings,
       const PageId& pageId,
       const OutputFileNameGenerator& outFileNameGen,
       bool batch,
       bool debug);
  virtual ~Task();

  bool process(const TaskStatus& status, const FilterData& data, const QPolygonF& contentRectPhys);

  // ##############################################################################################
  // # AbstractTask implementation
  // ##############################################################################################
 public:
  QDomElement saveSettings(const ::cli::ProjectWriter& writer, QDomDocument& doc) const override;

 private:
  void writePageSettings(QDomDocument& doc, QDomElement& filterEl, const PageId& pageId, int numericId) const;

 private:
  void deleteMutuallyExclusiveOutputFiles();

  intrusive_ptr<::output::Settings> m_settings;
  std::unique_ptr<DebugImages> m_dbg;
  PageId m_pageId;
  OutputFileNameGenerator m_outFileNameGen;
  bool m_batchProcessing;
  bool m_debug;
};

}  // namespace output
}  // namespace cli

#endif  // SCANTAILOR_CLI_TASKS_OUTPUTTASK_H_
