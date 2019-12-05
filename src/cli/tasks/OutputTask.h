// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_TASKS_CLI_OUTPUTTASK_H_
#define SCANTAILOR_TASKS_CLI_OUTPUTTASK_H_

#include <QPolygonF>
#include "core/FilterData.h"
#include "core/OutputFileNameGenerator.h"
#include "core/filters/output/Settings.h"
#include "foundation/TaskStatus.h"
#include "imageproc/DebugImages.h"

namespace output {
namespace cli {

class Task : public ref_countable {
 public:
  Task(intrusive_ptr<Settings> settings,
       const PageId& pageId,
       const OutputFileNameGenerator& outFileNameGen,
       bool batch,
       bool debug);
  virtual ~Task();

  bool process(const TaskStatus& status, const FilterData& data, const QPolygonF& contentRectPhys);

 private:
  void deleteMutuallyExclusiveOutputFiles();

  intrusive_ptr<Settings> m_settings;
  std::unique_ptr<DebugImages> m_dbg;
  PageId m_pageId;
  OutputFileNameGenerator m_outFileNameGen;
  bool m_batchProcessing;
  bool m_debug;
};

}  // namespace cli
}  // namespace output

#endif  // SCANTAILOR_CLI_TASKS_OUTPUTTASK_H_
