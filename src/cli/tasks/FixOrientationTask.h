// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CLI_TASKS_FIXORIENTATIONTASK_H_
#define SCANTAILOR_CLI_TASKS_FIXORIENTATIONTASK_H_

#include "core/FilterData.h"
#include "core/ImageSettings.h"
#include "core/filters/fix_orientation/Settings.h"
#include "foundation/TaskStatus.h"
#include "foundation/intrusive_ptr.h"
#include "foundation/ref_countable.h"
#include "tasks/PageSplitTask.h"

namespace fix_orientation {
namespace cli {

class Task : public ref_countable {
 public:
  Task(const PageId& pageId,
       intrusive_ptr<Settings> settings,
       intrusive_ptr<ImageSettings> imageSettings,
       intrusive_ptr<page_split::cli::Task> nextTask,
       bool batchProcessing);
  ~Task() override;

  bool process(const TaskStatus& status, FilterData data);

  void updateFilterData(FilterData& data);

 private:
  intrusive_ptr<Settings> m_settings;
  intrusive_ptr<ImageSettings> m_imageSettings;
  intrusive_ptr<page_split::cli::Task> m_nextTask;
  PageId m_pageId;
  ImageId m_imageId;
  bool m_batchProcessing;
};

}  // namespace cli
}  // namespace fix_orientation

#endif  // SCANTAILOR_CLI_TASKS_FIXORIENTATIONTASK_H_
