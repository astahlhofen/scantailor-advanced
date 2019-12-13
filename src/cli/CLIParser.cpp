// Copyright (C) 2019  Andreas Stahlhofen <andreas.stahlhofen@gmail.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "CLIParser.h"
#include <QDir>
#include <regex>
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
      m_generateProjectFile{false},
      m_customFixDpi{false},
      m_customDpi{} {}

CLIParser::~CLIParser() = default;

ParserResult CLIParser::process(int argc, char** argv) {
  auto generalGroup = m_app.add_option_group("general");

  // GENERAL - OUTPUT_DIRECTORY
  generalGroup->add_option("-o,--output-directory", m_outputDir, "The required output directory.")
      ->required()
      ->type_name("[OUTPUT_DIRECTORY]")
      ->check(::cli::ExistsOrCreate);

  generalGroup->add_flag("--gp,--generate-project", m_generateProjectFile,
                         "If this flag is set, a file 'project.ScanTailor' is generate inside the output directory.");

  // GENERAL - LOG_LEVEL
  std::vector<std::pair<std::string, LogLevel>> logLevelMapping{
      {"error", LogLevel::ERROR}, {"warning", LogLevel::WARNING}, {"info", LogLevel::INFO}, {"debug", LogLevel::DEBUG}};
  generalGroup->add_option("-l,--log-level", m_logLevel, "Set the log level to use.")
      ->default_val(std::to_string(LogLevel::WARNING))
      ->transform(CLI::CheckedTransformer(logLevelMapping, CLI::ignore_case));

  // GENERAL - VERSION
  generalGroup->add_flag_callback(
      "--version",
      [this]() {
        std::cerr << m_app.get_name() << " - " << VERSION << std::endl;
        throw CLI::Success();
      },
      "Show version");

  // GENERAL - AUTO FIX DPI
  generalGroup->add_option_function<std::string>(
      "--sid,--set-input-dpi",
      [this](const std::string& _dpiString) {
        std::regex dpiRegex{"^([0-9]{1,})([xX]([0-9]{1,}))?$"};
        std::smatch match;
        if (!std::regex_match(_dpiString, match, dpiRegex)) {
          throw CLI::ValidationError("The given dpi string '" + _dpiString + "' is not valid.");
        }

        int xdpi = std::stoi(match[1].str());
        int ydpi = (match[3].str().empty()) ? xdpi : std::stoi(match[3].str());
        m_customFixDpi = true;
        m_customDpi = Dpi{xdpi, ydpi};
      },
      "Force to set the metadata of the input images to the given dpi. The image file itself is not "
      "changed by scantailor. The DPI specification must be of the form <xdpi>x<ydpi> or <dpi> "
      "for both x- and y-direction.");

  // GENERAL - INPUT FILES
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

  // Parse the arguments.
  try {
    m_app.parse((argc), (argv));
  } catch (const CLI::CallForHelp& e) {
    m_app.exit(e);
    return ParserResult::HelpRequested;
  } catch (const CLI::CallForAllHelp& e) {
    m_app.exit(e);
    return ParserResult::HelpRequested;
  } catch (const CLI::Success& e) {
    m_app.exit(e);
    return ParserResult::VersionRequested;
  } catch (const CLI::ParseError& e) {
    m_app.exit(e);
    return ParserResult::Error;
  }

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

  return ParserResult::Ok;
}

}  // namespace cli
