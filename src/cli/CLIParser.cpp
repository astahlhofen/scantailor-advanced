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
      m_inputFiles{} {
  m_colorParams = CLIParser::defaultColorParams();
  m_colorCommonOptions = CLIParser::defaultColorCommonOptions();
  m_posterizationOptions = CLIParser::defaultPosterizationOptions();
  m_blackWhiteOptions = CLIParser::defaultBlackWhiteOptions();
  m_colorSegmenterOptions = CLIParser::defaultColorSegmenterOptions();
}

CLIParser::~CLIParser() = default;

ParserResult CLIParser::process(int argc, char** argv) {
  auto generalGroup = m_app.add_option_group("General");
  auto outputGroup = m_app.add_option_group("Output");
  auto outputPosterizationGroup = outputGroup->add_option_group("Posterization");
  auto outputBlackWhiteGroup = outputGroup->add_option_group("Black & White");
  auto outputColorSegmentationGroup = outputGroup->add_option_group("Color Segmentation");

  // GENERAL - OUTPUT_DIRECTORY
  generalGroup->add_option("-o,--output-directory", m_outputDir, "The required output directory")
      ->required()
      ->type_name("[OUTPUT_DIRECTORY]")
      ->check(::cli::ExistsOrCreate);

  generalGroup->add_flag("--gp,--generate-project", m_generateProjectFile,
                         "Generate project file inside the output directory");

  // GENERAL - LOG_LEVEL
  std::vector<std::pair<std::string, LogLevel>> logLevelMapping{
      {"error", LogLevel::ERROR}, {"warning", LogLevel::WARNING}, {"info", LogLevel::INFO}, {"debug", LogLevel::DEBUG}};
  generalGroup->add_option("-l,--log-level", m_logLevel, "Set the log level to use.")
      ->type_name("[LOG_LEVEL]")
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
  generalGroup
      ->add_option_function<std::string>(
          "--sid,--set-input-dpi",
          [this](const std::string& _dpiString) {
            m_customFixDpi = true;
            m_customDpi = CLIParser::stringToDpi(_dpiString);
          },
          "Force to set the metadata of the input images to the given dpi.\n"
          "The image file itself is not changed by scantailor. The DPI \n"
          "specification must be of the form <xdpi>x<ydpi> or <dpi> for \n"
          "both x- and y-direction.")
      ->type_name("[DPI]");

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

  // ##############################################################################################
  // # Output
  // ##############################################################################################
  outputGroup->add_flag_callback(
      "--eeic,--enable-equalize-illumination-cl",
      std::bind(&::output::ColorCommonOptions::setNormalizeIllumination, &m_colorCommonOptions, true),
      "Enable normalize illumination in color mode");
  outputGroup->add_flag_callback("--efm,--enable-fill-margins",
                                 std::bind(&::output::ColorCommonOptions::setFillMargins, &m_colorCommonOptions, true),
                                 "Enable fill margins at the output step");
  outputGroup->add_flag_callback("--efo,--enable-fill-offcut",
                                 std::bind(&::output::ColorCommonOptions::setFillOffcut, &m_colorCommonOptions, true),
                                 "Enable fill offcut at the output step");

  outputGroup
      ->add_option_function<std::string>(
          "--od,--output-dpi",
          [this](const std::string& _dpiString) { m_outputDpi = CLIParser::stringToDpi(_dpiString); }, "The output DPI")
      ->default_val("600x600")
      ->type_name("[DPI]");

  std::vector<std::pair<std::string, ::output::ColorMode>> colorModeMapping{
      {"bw", ::output::ColorMode::BLACK_AND_WHITE},
      {"cgray", ::output::ColorMode::COLOR_GRAYSCALE},
      {"mixed", ::output::ColorMode::MIXED}};
  outputGroup
      ->add_option_function<::output::ColorMode>(
          "--cm,--color-mode", [this](::output::ColorMode colorMode) { m_colorParams.setColorMode(colorMode); },
          "The color mode to use\n"
          "  - bw = Black & White\n"
          "  - cgray = Color Grayscale\n"
          "  - mixed = Mixed Mode")
      ->type_name("[COLOR_MODE]")
      ->default_val(std::to_string(m_colorParams.colorMode()))
      ->transform(CLI::CheckedTransformer(colorModeMapping, CLI::ignore_case));

  std::vector<std::pair<std::string, ::output::FillingColor>> fillingColorMapping{
      {"bg", ::output::FillingColor::FILL_BACKGROUND}, {"w", ::output::FillingColor::FILL_WHITE}};
  outputGroup
      ->add_option_function<::output::FillingColor>(
          "--fc,--filling-color",
          [this](::output::FillingColor fillingColor) { m_colorCommonOptions.setFillingColor(fillingColor); },
          "The filling color at the output step (w=white, bg=background)")
      ->type_name("[FILLING_COLOR]")
      ->transform(CLI::CheckedTransformer(fillingColorMapping, CLI::ignore_case))
      ->default_val(std::to_string(m_colorCommonOptions.getFillingColor()));

  // ##############################################################################################
  // # Output - Posterization
  // ##############################################################################################
  outputPosterizationGroup->add_flag_callback(
      "--ep,--enable-posterization",
      std::bind(&::output::ColorCommonOptions::PosterizationOptions::setEnabled, &m_posterizationOptions, true),
      "Enable posterization");
  outputPosterizationGroup->add_flag_callback(
      "--fbw,--posterization-force-bw",
      std::bind(&::output::ColorCommonOptions::PosterizationOptions::setForceBlackAndWhite, &m_posterizationOptions,
                true),
      "Enable force black and white at posterization step");
  outputPosterizationGroup->add_flag_callback(
      "--pen,--posterization-enabled-normalization",
      std::bind(&::output::ColorCommonOptions::PosterizationOptions::setNormalizationEnabled, &m_posterizationOptions,
                true),
      "Enable normalization at posterization step");

  outputPosterizationGroup
      ->add_option_function<int>(
          "--pl,--posterization-level", [this](int level) { m_posterizationOptions.setLevel(level); },
          "The posterization level")
      ->type_name("[LEVEL]")
      ->transform(CLI::Range(2, 6))
      ->default_val(std::to_string(m_posterizationOptions.getLevel()));

  // ##############################################################################################
  // # Output - Black & White
  // ##############################################################################################
  outputBlackWhiteGroup->add_flag_callback(
      "--ems,--enable-morphological-smoothing",
      std::bind(&::output::BlackWhiteOptions::setMorphologicalSmoothingEnabled, &m_blackWhiteOptions, true),
      "Enables morphological smoothing");
  outputBlackWhiteGroup->add_flag_callback(
      "--eeib,--enable-equalize-illumination-bw",
      std::bind(&::output::BlackWhiteOptions::setNormalizeIllumination, &m_blackWhiteOptions, true),
      "Enables illumination normalization before binarization");
  outputBlackWhiteGroup->add_flag_callback(
      "--esgs,--enable-savitzky-golay-smoothing",
      std::bind(&::output::BlackWhiteOptions::setSavitzkyGolaySmoothingEnabled, &m_blackWhiteOptions, true),
      "Enables Savitzky Golay smoothing");

  std::vector<std::pair<std::string, ::output::BinarizationMethod>> binarizationMethodMapping{
      {"otsu", ::output::BinarizationMethod::OTSU},
      {"sauvola", ::output::BinarizationMethod::SAUVOLA},
      {"wolf", ::output::BinarizationMethod::WOLF}};
  outputGroup
      ->add_option_function<::output::BinarizationMethod>(
          "--bm,--binarization-method",
          [this](::output::BinarizationMethod binarizationMethod) {
            m_blackWhiteOptions.setBinarizationMethod(binarizationMethod);
          },
          "Set the binarization method to use")
      ->type_name("[METHOD]")
      ->default_val(std::to_string(m_blackWhiteOptions.getBinarizationMethod()))
      ->transform(CLI::CheckedTransformer(binarizationMethodMapping, CLI::ignore_case));

  outputGroup
      ->add_option_function<int>(
          "--wlb,--wolf-lower-bound",
          std::bind(&::output::BlackWhiteOptions::setWolfLowerBound, &m_blackWhiteOptions, std::placeholders::_1),
          "Set the wolf lower bound which is the minimum possible gray level that can be made black")
      ->type_name("[VALUE]")
      ->default_val(std::to_string(m_blackWhiteOptions.getWolfLowerBound()))
      ->transform(CLI::Range(1, 254));
  outputGroup
      ->add_option_function<int>(
          "--wub,--wolf-upper-bound",
          std::bind(&::output::BlackWhiteOptions::setWolfUpperBound, &m_blackWhiteOptions, std::placeholders::_1),
          "Set the wolf upper bound which is the maximum possible gray level that can be made black")
      ->type_name("[VALUE]")
      ->default_val(std::to_string(m_blackWhiteOptions.getWolfUpperBound()))
      ->transform(CLI::Range(1, 254));
  outputGroup
      ->add_option_function<double>(
          "--wco,--wolf-coef",
          std::bind(&::output::BlackWhiteOptions::setWolfCoef, &m_blackWhiteOptions, std::placeholders::_1),
          "The wolf coefficient")
      ->type_name("[VALUE]")
      ->default_val(std::to_string(m_blackWhiteOptions.getWolfCoef()))
      ->transform(CLI::Range(0.01, 9.99));
  outputGroup
      ->add_option_function<int>(
          "--ota,--otsu-threshold-adjustment",
          std::bind(&::output::BlackWhiteOptions::setThresholdAdjustment, &m_blackWhiteOptions, std::placeholders::_1),
          "Threshold adjustment used by otsu binarization method")
      ->type_name("[THRESHOLD]")
      ->transform(CLI::Range(-30, 30))
      ->default_val(std::to_string(m_blackWhiteOptions.thresholdAdjustment()));
  outputGroup
      ->add_option_function<double>(
          "--sco,--sauvola-coef",
          std::bind(&::output::BlackWhiteOptions::setSauvolaCoef, &m_blackWhiteOptions, std::placeholders::_1),
          "The sauvola coefficient")
      ->type_name("[VALUE]")
      ->default_val(std::to_string(m_blackWhiteOptions.getSauvolaCoef()))
      ->transform(CLI::Range(0.01, 9.99));
  outputGroup
      ->add_option_function<int>(
          "--ws,--window-size",
          std::bind(&::output::BlackWhiteOptions::setWindowSize, &m_blackWhiteOptions, std::placeholders::_1),
          "The window size used by the sauvola and wolf binarization method")
      ->type_name("[SIZE]")
      ->default_val(std::to_string(m_blackWhiteOptions.getWindowSize()))
      ->transform(CLI::Range(5, 9999));

  // ##############################################################################################
  // # Output - Black & White - Color Segmentation Options
  // ##############################################################################################
  outputColorSegmentationGroup->add_flag_callback(
      "--ecs,--enable-color-segmentation",
      std::bind(&::output::BlackWhiteOptions::ColorSegmenterOptions::setEnabled, &m_colorSegmenterOptions, true),
      "Enables color segmentation");

  outputColorSegmentationGroup
      ->add_option_function<int>(
          "--rta,--red-threshold-adjustment",
          std::bind(&::output::BlackWhiteOptions::ColorSegmenterOptions::setRedThresholdAdjustment,
                    &m_colorSegmenterOptions, std::placeholders::_1),
          "The red threshold adjustment")
      ->type_name("[VALUE]")
      ->default_val(std::to_string(m_colorSegmenterOptions.getRedThresholdAdjustment()))
      ->transform(CLI::Range(-99, 99));
  outputColorSegmentationGroup
      ->add_option_function<int>(
          "--gta,--green-threshold-adjustment",
          std::bind(&::output::BlackWhiteOptions::ColorSegmenterOptions::setGreenThresholdAdjustment,
                    &m_colorSegmenterOptions, std::placeholders::_1),
          "The green threshold adjustment")
      ->type_name("[VALUE]")
      ->default_val(std::to_string(m_colorSegmenterOptions.getGreenThresholdAdjustment()))
      ->transform(CLI::Range(-99, 99));
  outputColorSegmentationGroup
      ->add_option_function<int>(
          "--bta,--blue-threshold-adjustment",
          std::bind(&::output::BlackWhiteOptions::ColorSegmenterOptions::setBlueThresholdAdjustment,
                    &m_colorSegmenterOptions, std::placeholders::_1),
          "The blue threshold adjustment")
      ->type_name("[VALUE]")
      ->default_val(std::to_string(m_colorSegmenterOptions.getBlueThresholdAdjustment()))
      ->transform(CLI::Range(-99, 99));
  outputColorSegmentationGroup
      ->add_option_function<int>("--nr,--noise-reduction",
                                 std::bind(&::output::BlackWhiteOptions::ColorSegmenterOptions::setNoiseReduction,
                                           &m_colorSegmenterOptions, std::placeholders::_1),
                                 "")
      ->type_name("[VALUE]")
      ->default_val(std::to_string(m_colorSegmenterOptions.getNoiseReduction()))
      ->transform(CLI::Range(0, 999));

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

Dpi CLIParser::stringToDpi(const std::string& dpiString) {
  std::regex dpiRegex{"^([0-9]{1,})([xX]([0-9]{1,}))?$"};
  std::smatch match;
  if (!std::regex_match(dpiString, match, dpiRegex)) {
    throw CLI::ValidationError("The given dpi string '" + dpiString + "' is not valid.");
  }

  int xdpi = std::stoi(match[1].str());
  int ydpi = (match[3].str().empty()) ? xdpi : std::stoi(match[3].str());
  return Dpi{xdpi, ydpi};
}

::output::ColorParams CLIParser::defaultColorParams() {
  ::output::ColorParams outputColorParams{};
  outputColorParams.setColorMode(::output::ColorMode::BLACK_AND_WHITE);
  return outputColorParams;
}

::output::ColorCommonOptions CLIParser::defaultColorCommonOptions() {
  ::output::ColorCommonOptions outputColorCommonOptions{};
  outputColorCommonOptions.setNormalizeIllumination(false);
  outputColorCommonOptions.setFillMargins(false);
  outputColorCommonOptions.setFillingColor(::output::FillingColor::FILL_BACKGROUND);
  outputColorCommonOptions.setFillOffcut(false);
  return outputColorCommonOptions;
}


::output::ColorCommonOptions::PosterizationOptions CLIParser::defaultPosterizationOptions() {
  ::output::ColorCommonOptions::PosterizationOptions outputPosterizationOptions{};
  outputPosterizationOptions.setEnabled(false);
  outputPosterizationOptions.setForceBlackAndWhite(false);
  outputPosterizationOptions.setLevel(4);
  outputPosterizationOptions.setNormalizationEnabled(false);
  return outputPosterizationOptions;
}

::output::BlackWhiteOptions CLIParser::defaultBlackWhiteOptions() {
  ::output::BlackWhiteOptions outputBlackWhiteOptions{};
  outputBlackWhiteOptions.setMorphologicalSmoothingEnabled(false);
  outputBlackWhiteOptions.setBinarizationMethod(::output::BinarizationMethod::OTSU);
  outputBlackWhiteOptions.setNormalizeIllumination(false);
  outputBlackWhiteOptions.setWolfUpperBound(254);
  outputBlackWhiteOptions.setWolfLowerBound(1);
  outputBlackWhiteOptions.setWolfCoef(0.3);
  outputBlackWhiteOptions.setSavitzkyGolaySmoothingEnabled(false);
  outputBlackWhiteOptions.setThresholdAdjustment(0);
  outputBlackWhiteOptions.setSauvolaCoef(0.34);
  outputBlackWhiteOptions.setWindowSize(200);
  return outputBlackWhiteOptions;
}

::output::BlackWhiteOptions::ColorSegmenterOptions CLIParser::defaultColorSegmenterOptions() {
  ::output::BlackWhiteOptions::ColorSegmenterOptions outputColorSegmenterOptions;
  outputColorSegmenterOptions.setGreenThresholdAdjustment(0);
  outputColorSegmenterOptions.setEnabled(false);
  outputColorSegmenterOptions.setBlueThresholdAdjustment(0);
  outputColorSegmenterOptions.setNoiseReduction(7);
  outputColorSegmenterOptions.setRedThresholdAdjustment(0);
  return outputColorSegmenterOptions;
}

}  // namespace cli
