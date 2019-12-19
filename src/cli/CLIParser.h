// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef CLI_CLIParser_H_
#define CLI_CLIParser_H_

#include <QDir>
#include <sstream>
#include "core/filters/output/ColorParams.h"
#include "core/logger/Logger.h"
#include "imageproc/Dpi.h"

namespace CLI {

std::istringstream& operator>>(std::istringstream& in, QDir& directory);

}  // namespace CLI

#include "vendor/CLI11.hpp"

namespace cli {

enum ParserResult { Ok, VersionRequested, HelpRequested, Error };

class CLIParser {
 public:
  CLIParser();
  virtual ~CLIParser();

  // ##############################################################################################
  // # API
  // ##############################################################################################

 public:
  ParserResult process(int argc, char** argv);

  const QDir& outputDir() const { return m_outputDir; }
  LogLevel logLevel() const { return m_logLevel; }
  const std::vector<QFileInfo>& inputFiles() const { return m_inputFiles; }
  bool generateProjectFile() const { return m_generateProjectFile; }

  Dpi customDpi() const { return m_customDpi; }
  bool customFixDpi() const { return m_customFixDpi; }

  Dpi outputDpi() const { return m_outputDpi; }

  const ::output::ColorParams& colorParams() const { return m_colorParams; }

  const ::output::ColorCommonOptions& colorCommonOptions() const { return m_colorCommonOptions; }

  const ::output::ColorCommonOptions::PosterizationOptions& posterizationOptions() const {
    return m_posterizationOptions;
  }

  const ::output::BlackWhiteOptions& blackWhiteOptions() const { return m_blackWhiteOptions; }

  const ::output::BlackWhiteOptions::ColorSegmenterOptions& colorSegmenterOptions() const {
    return m_colorSegmenterOptions;
  }

 private:
  ::CLI::App m_app;

  QDir m_outputDir;
  LogLevel m_logLevel;
  std::vector<QFileInfo> m_inputFiles;
  bool m_generateProjectFile = false;

  bool m_customFixDpi = false;
  Dpi m_customDpi{300, 300};

  // ##############################################################################################
  // # Output Params.
  // ##############################################################################################
  Dpi m_outputDpi{600, 600};
  ::output::ColorParams m_colorParams{};
  ::output::ColorCommonOptions m_colorCommonOptions{};
  ::output::ColorCommonOptions::PosterizationOptions m_posterizationOptions{};
  ::output::BlackWhiteOptions m_blackWhiteOptions{};
  ::output::BlackWhiteOptions::ColorSegmenterOptions m_colorSegmenterOptions{};

  // ##############################################################################################
  // # Helper functions.
  // ##############################################################################################

 private:
  static Dpi stringToDpi(const std::string& dpiString);

  static ::output::ColorParams defaultColorParams();
  static ::output::ColorCommonOptions defaultColorCommonOptions();
  static ::output::ColorCommonOptions::PosterizationOptions defaultPosterizationOptions();
  static ::output::BlackWhiteOptions defaultBlackWhiteOptions();
  static ::output::BlackWhiteOptions::ColorSegmenterOptions defaultColorSegmenterOptions();
};

}  // namespace cli

#endif  // CLI_CLIParser_H_
