/*
 * LoadFileTask.cpp
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

#include "LoadFileTask.h"
#include "core/FilterData.h"
#include "core/ImageLoader.h"
#include "core/logger/Logger.h"
#include "imageproc/Dpm.h"
#include "imageproc/Grayscale.h"

namespace cli {
namespace fix_orientation {

LoadFileTask::LoadFileTask(const PageInfo& page,
                           intrusive_ptr<ProjectPages> pages,
                           intrusive_ptr<::cli::fix_orientation::Task> nextTask)
    : m_imageId(page.imageId()),
      m_imageMetadata(page.metadata()),
      m_pages(std::move(pages)),
      m_nextTask(std::move(nextTask)) {
  assert(m_nextTask);
  m_cancelFlag.store(0);
}

LoadFileTask::~LoadFileTask() {}

void LoadFileTask::cancel() {
  m_cancelFlag.store(1);
}

bool LoadFileTask::isCancelled() const {
  return m_cancelFlag.load() != 0;
}

void LoadFileTask::throwIfCancelled() const {
  if (isCancelled()) {
    throw CancelledException();
  }
}

const char* LoadFileTask::CancelledException::what() const noexcept {
  return "LoadFileTask cancelled.";
}

bool LoadFileTask::process() {
  Logger::debug() << "LoadFileTask::process(): Load image file with id " << m_imageId.page() << " ("
                  << m_imageId.filePath().toStdString() << ")" << Logger::eol;

  QImage image = ImageLoader::load(m_imageId);

  try {
    throwIfCancelled();

    if (image.isNull()) {
      Logger::error() << "LoadFileTask::process(): Failed to load the image under the path '"
                      << m_imageId.filePath().toStdString() << "'" << Logger::eol;
      return false;
    } else {
      convertToSupportedFormat(image);
      updateImageSizeIfChanged(image);
      overrideDpi(image);

      return m_nextTask->process(*this, FilterData(image));
    }
  } catch (const CancelledException&) {
    return false;
  }
}

void LoadFileTask::updateImageSizeIfChanged(const QImage& image) {
  // The user might just replace a file with another one.
  // In that case, we update its size that we store.
  // Note that we don't do the same about DPI, because
  // a DPI mismatch between the image and the stored value
  // may indicate that the DPI was overridden.
  // TODO: do something about DPIs when we have the ability
  // to change DPIs at any point in time (not just when
  // creating a project).
  if (image.size() != m_imageMetadata.size()) {
    m_imageMetadata.setSize(image.size());
    m_pages->updateImageMetadata(m_imageId, m_imageMetadata);
  }
}


void LoadFileTask::convertToSupportedFormat(QImage& image) const {
  if (((image.format() == QImage::Format_Indexed8) && !image.isGrayscale()) || (image.depth() > 8)) {
    const QImage::Format fmt = image.hasAlphaChannel() ? QImage::Format_ARGB32 : QImage::Format_RGB32;
    image = image.convertToFormat(fmt);
  } else {
    image = imageproc::toGrayscale(image);
  }
}


void LoadFileTask::overrideDpi(QImage& image) const {
  // Beware: QImage will have a default DPI when loading
  // an image that doesn't specify one.
  const Dpm dpm(m_imageMetadata.dpi());
  image.setDotsPerMeterX(dpm.horizontal());
  image.setDotsPerMeterY(dpm.vertical());
}

}  // namespace fix_orientation
}  // namespace cli
