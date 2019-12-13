// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.


#ifndef SCANTAILOR_CLI_CLIRunner_H_
#define SCANTAILOR_CLI_CLIRunner_H_

#include <vector>
#include "CLIParser.h"
#include "core/ImageFileInfo.h"
#include "foundation/intrusive_ptr.h"
#include "tasks/FixOrientationTask.h"
#include "tasks/OutputTask.h"
#include "tasks/PageLayoutTask.h"
#include "tasks/PageSplitTask.h"
#include "tasks/SelectContentTask.h"

namespace cli {

class CLIRunner {
  template <typename T>
  using TaskVector = std::vector<intrusive_ptr<T>>;

 public:
  CLIRunner();
  virtual ~CLIRunner();

 public:
  int run(int argc, char** argv);

 private:
  void checkInputImages(std::vector<ImageFileInfo>& inputImages);

  TaskVector<::cli::output::Task> getOutputTasks(const PageSequence& pages, bool isBatch, bool isDebug);

  TaskVector<::cli::page_layout::Task> getPageLayoutTasks(const TaskVector<::cli::output::Task>& outputTasks,
                                                          const PageSequence& pages,
                                                          bool isBatch,
                                                          bool isDebug);

  TaskVector<::cli::select_content::Task> getSelectContentTasks(
      const TaskVector<::cli::page_layout::Task>& pageLayoutTask,
      const PageSequence& pages,
      bool isBatch,
      bool isDebug);

  TaskVector<::cli::deskew::Task> getDeskewTasks(const TaskVector<::cli::select_content::Task>& selectContentTask,
                                                 intrusive_ptr<ImageSettings> imageSettings,
                                                 const PageSequence& pages,
                                                 bool isBatch,
                                                 bool isDebug);

  TaskVector<::cli::page_split::Task> getPageSplitTasks(const TaskVector<::cli::deskew::Task>& deskewTasks,
                                                        intrusive_ptr<ProjectPages> projectPages,
                                                        const PageSequence& pages,
                                                        bool isBatch,
                                                        bool isDebug);

  TaskVector<::cli::fix_orientation::Task> getFixOrientationTasks(
      const TaskVector<::cli::page_split::Task>& pageSplitTasks,
      intrusive_ptr<ImageSettings> imageSettings,
      const PageSequence& pages,
      bool isBatch,
      bool isDebug);

 private:
  ::cli::CLIParser m_parser;
};

}  // namespace cli

#endif  // SCANTAILOR_CLI_CLIRunner_H_
