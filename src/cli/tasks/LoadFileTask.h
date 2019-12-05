/*
 * LoadFileTask.h
 *
 *  Created on: 5 Dec 2019
 *  Copyright (C) 2019 Andreas Stahlhofen <astahlhofen@uni-koblenz.de>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCANTAILOR_CLI_TASKS_LOADFILETASK_H_
#define SCANTAILOR_CLI_TASKS_LOADFILETASK_H_

#include <QAtomicInt>
#include "core/PageInfo.h"
#include "core/ProjectPages.h"
#include "foundation/TaskStatus.h"
#include "foundation/ref_countable.h"
#include "tasks/FixOrientationTask.h"

namespace fix_orientation {
namespace cli {

class LoadFileTask : public TaskStatus, public ref_countable {
 public:
  class CancelledException : public std::exception {
   public:
    const char* what() const noexcept override;
  };

 public:
  LoadFileTask(const PageInfo& page,
               intrusive_ptr<ProjectPages> pages,
               intrusive_ptr<fix_orientation::cli::Task> nextTask);
  ~LoadFileTask() override;

  // ##############################################################################################
  // # API
  // ##############################################################################################

  bool process();

  // ##############################################################################################
  // # TaskStatus implementation.
  // ##############################################################################################

  void cancel() override;

  bool isCancelled() const override;

  void throwIfCancelled() const override;

  // ##############################################################################################
  // # Private Helper
  // ##############################################################################################

 private:
  void updateImageSizeIfChanged(const QImage& image);

  void convertToSupportedFormat(QImage& image) const;

  void overrideDpi(QImage& image) const;

  // ##############################################################################################
  // # Private Member
  // ##############################################################################################

 private:
  QAtomicInt m_cancelFlag;
  ImageId m_imageId;
  ImageMetadata m_imageMetadata;
  const intrusive_ptr<ProjectPages> m_pages;
  const intrusive_ptr<fix_orientation::cli::Task> m_nextTask;
};

}  // namespace cli
}  // namespace fix_orientation

#endif  // SCANTAILOR_CLI_TASKS_LOADFILETASK_H_
