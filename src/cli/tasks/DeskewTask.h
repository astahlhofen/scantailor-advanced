// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.


#ifndef SCANTAILOR_CLI_TASKS_DESKEWTASK_H_
#define SCANTAILOR_CLI_TASKS_DESKEWTASK_H_

#include <QDomDocument>
#include <QDomElement>
#include "cli/ProjectWriter.h"
#include "core/FilterData.h"
#include "core/ImageSettings.h"
#include "core/filters/deskew/Settings.h"
#include "foundation/TaskStatus.h"
#include "foundation/intrusive_ptr.h"
#include "imageproc/DebugImages.h"
#include "tasks/AbstractTask.h"
#include "tasks/SelectContentTask.h"

namespace cli {
namespace deskew {

class Task : public AbstractTask {
 public:
  Task(intrusive_ptr<::deskew::Settings> settings,
       intrusive_ptr<ImageSettings> imageSettings,
       intrusive_ptr<::cli::select_content::Task> nextTask,
       const PageId& pageId,
       bool batchProcessing,
       bool debug);
  ~Task() override;

  bool process(const TaskStatus& status, FilterData data);

  // ##############################################################################################
  // # AbstractTask implementation
  // ##############################################################################################
  QDomElement saveSettings(const ::cli::ProjectWriter& writer, QDomDocument& doc) const override;

 private:
  static void cleanup(const TaskStatus& status, imageproc::BinaryImage& img, const Dpi& dpi);

  static int from150dpi(int size, int targetDpi);

  static QSize from150dpi(const QSize& size, const Dpi& targetDpi);

  void updateFilterData(const TaskStatus& status, FilterData& data, bool needUpdate);

  void saveImageSettings(const ProjectWriter& writer, QDomDocument& doc, QDomElement& filterEl) const;

  void writeImageParams(QDomDocument& doc, QDomElement& filterEl, const PageId& pageId, int numericId) const;

  void writeParams(QDomDocument& doc, QDomElement& filterEl, const PageId& pageId, int numericId) const;

 private:
  intrusive_ptr<::deskew::Settings> m_settings;
  intrusive_ptr<ImageSettings> m_imageSettings;
  intrusive_ptr<::cli::select_content::Task> m_nextTask;
  std::unique_ptr<DebugImages> m_dbg;
  PageId m_pageId;
  bool m_batchProcessing;
};

}  // namespace deskew
}  // namespace cli

#endif  // SCANTAILOR_CLI_TASKS_DESKEWTASK_H_
