// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CLI_TASKS_FIXORIENTATIONTASK_H_
#define SCANTAILOR_CLI_TASKS_FIXORIENTATIONTASK_H_

#include "core/FilterData.h"
#include "core/ImageSettings.h"
#include "core/filters/fix_orientation/Settings.h"
#include "foundation/TaskStatus.h"
#include "foundation/intrusive_ptr.h"
#include "tasks/AbstractTask.h"
#include "tasks/PageSplitTask.h"

namespace cli {
namespace fix_orientation {

class Task : public AbstractTask {
 public:
  Task(const PageId& pageId,
       intrusive_ptr<::fix_orientation::Settings> settings,
       intrusive_ptr<ImageSettings> imageSettings,
       intrusive_ptr<::cli::page_split::Task> nextTask,
       bool batchProcessing);
  ~Task() override;

  bool process(const TaskStatus& status, FilterData data);

  void updateFilterData(FilterData& data);

  // ##############################################################################################
  // # AbstractTask implementation
  // ##############################################################################################
 public:
  QDomElement saveSettings(const ::cli::ProjectWriter& writer, QDomDocument& doc) const override;

 private:
  void writeParams(QDomDocument& doc, QDomElement& filterEl, const ImageId& imageId, int numericId) const;

  void saveImageSettings(const ProjectWriter& writer, QDomDocument& doc, QDomElement& filterEl) const;

  void writeImageParams(QDomDocument& doc, QDomElement& filterEl, const PageId& pageId, int numericId) const;

 private:
  intrusive_ptr<::fix_orientation::Settings> m_settings;
  intrusive_ptr<ImageSettings> m_imageSettings;
  intrusive_ptr<::cli::page_split::Task> m_nextTask;
  PageId m_pageId;
  ImageId m_imageId;
  bool m_batchProcessing;
};

}  // namespace fix_orientation
}  // namespace cli

#endif  // SCANTAILOR_CLI_TASKS_FIXORIENTATIONTASK_H_
