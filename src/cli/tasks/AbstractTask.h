// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.


#ifndef SCANTAILOR_CLI_TASKS_ABSTRACTTASK_H_
#define SCANTAILOR_CLI_TASKS_ABSTRACTTASK_H_

#include <QDomDocument>

namespace cli {

class ProjectWriter;

class AbstractTask : public ref_countable {
 public:
  ~AbstractTask() override = default;

  virtual QDomElement saveSettings(const ProjectWriter& writer, QDomDocument& doc) const = 0;
};

}  // namespace cli

#endif  // CLI_TASKS_ABSTRACTTASK_H_
