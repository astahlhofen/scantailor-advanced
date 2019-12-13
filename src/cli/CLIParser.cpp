// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "CLIParser.h"
#include <QDir>
#include "version.h"

using namespace CLI::enums;


// ##############################################################################################
// # Validators
// ##############################################################################################

namespace cli {

struct ExistsOrCreateValidator : public CLI::Validator {
  ExistsOrCreateValidator() : CLI::Validator("ExistsOrCreate") {
    func_ = [](const std::string& directoryPath) {
      struct stat buffer;
      bool exist = stat(directoryPath.c_str(), &buffer) == 0;
      bool is_dir = (buffer.st_mode & S_IFDIR) != 0;
      if (!exist) {
        // Create the directory if not exists.
        if (!QDir{}.mkpath(QDir{directoryPath.c_str()}.absolutePath())) {
          return "Failed to create directory: " + directoryPath;
        }
      } else if (!is_dir) {
        return "Directory is actually a file: " + directoryPath;
      }
      return std::string{};
    };
  }
};
const static ExistsOrCreateValidator ExistsOrCreate;

}  // namespace cli

// ##############################################################################################
// # Converters
// ##############################################################################################

std::istringstream& CLI::operator>>(std::istringstream& in, QDir& directory) {
  std::string directoryPath;
  in >> directoryPath;
  directory = QDir(directoryPath.c_str());
  return in;
}

// ##############################################################################################
// # CLIParser
// ##############################################################################################

namespace cli {

CLIParser::CLIParser()
    : m_app{"The advanced scantailor command line interface application.", "scantailor-cli"},
      m_outputDir{},
      m_logLevel{LogLevel::WARNING},
      m_inputFiles{},
      m_generateProjectFile{false} {}

CLIParser::~CLIParser() = default;

int CLIParser::process(int argc, char** argv) {
  auto generalGroup = m_app.add_option_group("general");

  // OUTPUT_DIRECTORY
  generalGroup->add_option("-o,--output-directory", m_outputDir, "The required output directory.")
      ->required()
      ->type_name("[OUTPUT_DIRECTORY]")
      ->check(::cli::ExistsOrCreate);

  generalGroup->add_option("--gp,--generate-project", m_generateProjectFile,
                           "If this flag is set, a file 'project.ScanTailor' is generate inside the output directory.");

  // LOG_LEVEL
  std::vector<std::pair<std::string, LogLevel>> logLevelMapping{
      {"error", LogLevel::ERROR}, {"warning", LogLevel::WARNING}, {"info", LogLevel::INFO}, {"debug", LogLevel::DEBUG}};
  generalGroup->add_option("-l,--log-level", m_logLevel, "Set the log level to use.")
      ->default_val(std::to_string(LogLevel::WARNING))
      ->transform(CLI::CheckedTransformer(logLevelMapping, CLI::ignore_case));

  // VERSION
  generalGroup->add_flag_callback(
      "--version",
      [this]() {
        std::cerr << m_app.get_name() << " - " << VERSION << std::endl;
        throw CLI::Success();
      },
      "Show version");

  // INPUT FILES;
  generalGroup
      ->add_option_function<std::vector<std::string>>(
          "input_files",
          [this](const std::vector<std::string>& _inputFiles) {
            std::for_each(_inputFiles.cbegin(), _inputFiles.cend(),
                          [this](const std::string& _inputFile) { m_inputFiles.emplace_back(_inputFile.c_str()); });
          },
          "Files to process.")
      ->type_name("[INPUT_FILES]")
      ->required(true)
      ->check(CLI::ExistingFile);


  CLI11_PARSE(m_app, argc, argv);

  // Set the log level.
  Logger::instance()->setLogLevel(m_logLevel);

  // Debug the parsed options.
  Logger::debug() << "CLIParser::process(): output_directory = " << m_outputDir.absolutePath().toStdString()
                  << Logger::eol;
  Logger::debug() << "CLIParser::process(): log_level        = " << m_logLevel << Logger::eol;
  Logger::debug() << "CLIParser::process(): input_files      = " << Logger::eol;
  std::for_each(m_inputFiles.cbegin(), m_inputFiles.cend(), [](const QFileInfo& _file) {
    Logger::debug() << "CLIParser::process():   - " << _file.absoluteFilePath().toStdString() << Logger::eol;
  });

  return 0;
}

}  // namespace cli
