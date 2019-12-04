/*
 * CmdParser.h
 *
 *  Created on: 4 Dec 2019
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

#ifndef SCANTAILOR_CLI_CMDPARSER_H_
#define SCANTAILOR_CLI_CMDPARSER_H_

#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include "core/logger/LogLevel.h"
#include "core/logger/Logger.h"

struct CmdOptions {
  std::vector<QFileInfo> inputFiles;
  QDir outputDirectory;
};

enum ParserResult { OPTIONS_OK, OPTIONS_ERROR, OPTIONS_VERSION_REQUESTED, OPTIONS_HELP_REQUESTED };

ParserResult parseCommandLine(QCommandLineParser& _parser, CmdOptions* _options, QString* _errorMessage) {
  /* Add command line options. */
  QCommandLineOption logLevelInfoOpt{{"v", "verbose"}, "Tell me what you are doing."};
  QCommandLineOption logLevelVerboseOpt{{"d", "debug"}, "Set the verbosity level to max"};
  logLevelVerboseOpt.setFlags(logLevelVerboseOpt.flags() | QCommandLineOption::ShortOptionStyle);

  QCommandLineOption outputDirOpt{{"o", "output"}, "The output directory.", "output_dir"};

  _parser.addOptions({logLevelInfoOpt, logLevelVerboseOpt, outputDirOpt});

  const QCommandLineOption helpOpt = _parser.addHelpOption();
  const QCommandLineOption versionOpt{"version", "The version."};
  _parser.addOption(versionOpt);

  _parser.addPositionalArgument("input_images", "The images to modify using scan tailor", "[files...]");

  /* Parse command line options */
  if (!_parser.parse(QCoreApplication::arguments())) {
    *_errorMessage = _parser.errorText();
    return OPTIONS_ERROR;
  }

  // Parse the log level at the beginning so that we can use it later in parseCommandLine().
  Logger::instance()->setLogLevel(LogLevel::WARNING);
  if (_parser.isSet(logLevelInfoOpt)) {
    Logger::instance()->setLogLevel(LogLevel::INFO);
  } else if (_parser.isSet(logLevelVerboseOpt)) {
    Logger::instance()->setLogLevel(LogLevel::DEBUG);
  }

  // Check if help or version is specified.
  if (_parser.isSet(helpOpt)) {
    return OPTIONS_HELP_REQUESTED;
  }
  if (_parser.isSet(versionOpt)) {
    return OPTIONS_VERSION_REQUESTED;
  }

  // Read and check the output directory.
  if (!_parser.isSet(outputDirOpt)) {
    *_errorMessage = "You must specify an output directory using -o <output_dir> or --output <output_dir>";
    return OPTIONS_ERROR;
  }
  const QString outputDirStr = _parser.value(outputDirOpt);
  if (outputDirStr.isEmpty()) {
    *_errorMessage = "The output argument is empty";
    return OPTIONS_ERROR;
  }
  const QFileInfo outputDirInfo{outputDirStr};
  if (outputDirInfo.isFile()) {
    *_errorMessage = "The specified output directory '" + outputDirInfo.absoluteFilePath() + "' is a filed";
    return OPTIONS_ERROR;
  }
  const QDir outputDir{outputDirStr};
  if (!outputDir.exists()) {
    if (!QDir{}.mkpath(outputDir.absolutePath())) {
      *_errorMessage = "Failed to create the output directory '" + outputDir.absolutePath() + "'";
      return OPTIONS_ERROR;
    }
    Logger::debug() << "parseCommandLine(): Create the output directory '" << outputDir.absolutePath().toStdString()
                    << "'" << Logger::eol;
  }
  _options->outputDirectory = outputDir;


  // Parse input files as positional arguments.
  QStringList files = _parser.positionalArguments();
  if (files.isEmpty()) {
    *_errorMessage = "No input files specified";
    return OPTIONS_ERROR;
  }
  std::for_each(files.begin(), files.end(), [&](const QString& _file) {
    const QString path = QDir::cleanPath(_file);
    QFileInfo fileInfo{path};

    // Check if the specified input file is a directory.
    if (fileInfo.isDir()) {
      *_errorMessage = "\n  - The specified input file '" + path + "' is a directory";
      return;
    }

    // Check if the specified input file exists.
    if (!fileInfo.exists()) {
      *_errorMessage = "\n  - The specified input file '" + path + "' does not exist";
      return;
    }

    // Add the input file to the list of files to process.
    Logger::debug() << "parseCommandLine(): add input file '" << fileInfo.absoluteFilePath().toStdString() << "'"
                    << Logger::eol;
    _options->inputFiles.push_back(fileInfo);
  });

  if (!_errorMessage->isEmpty()) {
    return OPTIONS_ERROR;
  }

  return OPTIONS_OK;
}

#endif  // SCANTAILOR_CLI_CMDPARSER_H_
