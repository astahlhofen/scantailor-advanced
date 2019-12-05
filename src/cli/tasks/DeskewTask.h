// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.


#ifndef SCANTAILOR_CLI_TASKS_DESKEWTASK_H_
#define SCANTAILOR_CLI_TASKS_DESKEWTASK_H_

#include "core/FilterData.h"
#include "core/ImageSettings.h"
#include "core/filters/deskew/Settings.h"
#include "foundation/TaskStatus.h"
#include "foundation/intrusive_ptr.h"
#include "foundation/ref_countable.h"
#include "imageproc/DebugImages.h"
#include "tasks/SelectContentTask.h"

namespace deskew {
namespace cli {

class Task : public ref_countable {
 public:
  Task(intrusive_ptr<Settings> settings,
       intrusive_ptr<ImageSettings> imageSettings,
       intrusive_ptr<select_content::cli::Task> nextTask,
       const PageId& pageId,
       bool batchProcessing,
       bool debug);
  ~Task() override;

  bool process(const TaskStatus& status, FilterData data);

 private:
  static void cleanup(const TaskStatus& status, imageproc::BinaryImage& img, const Dpi& dpi);

  static int from150dpi(int size, int targetDpi);

  static QSize from150dpi(const QSize& size, const Dpi& targetDpi);

  void updateFilterData(const TaskStatus& status, FilterData& data, bool needUpdate);

 private:
  intrusive_ptr<Settings> m_settings;
  intrusive_ptr<ImageSettings> m_imageSettings;
  intrusive_ptr<select_content::cli::Task> m_nextTask;
  std::unique_ptr<DebugImages> m_dbg;
  PageId m_pageId;
  bool m_batchProcessing;
};

}  // namespace cli
}  // namespace deskew

#endif  // SCANTAILOR_CLI_TASKS_DESKEWTASK_H_
