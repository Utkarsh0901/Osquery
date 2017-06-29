/*
 *  Copyright (c) 2014-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <exception>
#include <iostream>
#include <string>

#include <osquery/filesystem.h>
#include <osquery/flags.h>
#include <osquery/logger.h>
#include <curl/curl.h>

namespace fs = boost::filesystem;

/**
 * This is the mode that Glog uses for logfiles.
 * Must be at the top level (i.e. outside of the `osquery` namespace).
 */
DECLARE_int32(logfile2_mode);

namespace osquery {

FLAG(string,
     logger2_path,
     OSQUERY_LOG_HOME,
     "Directory path for ERROR/WARN/INFO and results logging");
/// Legacy, backward compatible "osquery_log_dir" CLI option.
FLAG_ALIAS(std::string, osquery_log2_dir, logger2_path);

FLAG(int32, logger2_mode, 0640, "Decimal mode for log files (default '0640')");
FLAG(string, ip_address, OSQUERY_LOG_HOME, "ipinvaid");

const std::string kFilesystemLogger2Filename = "osqueryd.results.log";
const std::string kFilesystemLogger2Snapshots = "osqueryd.snapshots.log";

class FilesystemLogger2Plugin : public LoggerPlugin {
 public:
  Status setUp() override;

  /// Log results (differential) to a distinct path.
  Status logString(const std::string& s) override;

  /// Log snapshot data to a distinct path.
  Status logSnapshot(const std::string& s) override;

  /**
   * @brief Initialize the logger plugin after osquery has begun.
   *
   * The filesystem logger plugin is somewhat unique, it is the only logger
   * that will return an error during initialization. This allows Glog to
   * write directly to files.
   */
  void init(const std::string& name,
            const std::vector<StatusLogLine>& log2) override;

  /// Write a status to Glog.
  Status logStatus(const std::vector<StatusLogLine>& log2) override;

 private:
  /// The plugin-internal filesystem writer method.
  Status logStringToFile(const std::string& s,
                         const std::string& filename,
                         bool empty = false);

 private:
  /// The folder where Glog and the result/snapshot files are written.
  fs::path log2_path_;

  /// Filesystem writer mutex.
  Mutex mutex_;

 private:
  FRIEND_TEST(FilesystemLoggerTests, test_filesystem_init);
};

REGISTER(FilesystemLogger2Plugin, "logger", "filesystem2");

Status FilesystemLogger2Plugin::setUp() {
  log2_path_ = fs::path(FLAGS_logger2_path);

  // Ensure that the Glog status logs use the same mode as our results log.
  // Glog 0.3.4 does not support a logfile mode.
  // FLAGS_logfile_mode = FLAGS_logger_mode;

  // Ensure that we create the results log here.
  return logStringToFile("", kFilesystemLogger2Filename, true);
}

Status FilesystemLogger2Plugin::logString(const std::string& s) {
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //std::cout<<"ok"<<std::endl;
  CURL *curl;
  CURLcode res;
  const std::string m = std::string("[") + s + std::string("]");
  const char* postthis = m.c_str();
  //const std::string l="192.168.150.129:8765";
 
  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, FLAGS_ip_address.c_str() );
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postthis);
 
    /* if we don't provide POSTFIELDSIZE, libcurl will strlen() by
       itself */ 
    //curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, std::strlen(*postthis));
 
    /* Perform the request, res will get the return code */ 
    res = curl_easy_perform(curl);
    /* Check for errors */ 
    if(res != CURLE_OK)
      std::cout<<"curl_easy_perform() failed: "<<curl_easy_strerror(res)<<std::endl;
 
    /* always cleanup */ 
    curl_easy_cleanup(curl);
  }
  return logStringToFile(m, kFilesystemLogger2Filename);
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

Status FilesystemLogger2Plugin::logStringToFile(const std::string& s,
                                               const std::string& filename,
                                               bool empty) {
  WriteLock lock(mutex_);
  Status status;
  try {
    status = writeTextFile((log2_path_ / filename).string(),
                           (empty) ? "" : s + '\n',
                           FLAGS_logger2_mode,
                           true);
  } catch (const std::exception& e) {
    return Status(1, e.what());
  }
  return status;
}

Status FilesystemLogger2Plugin::logStatus(
    const std::vector<StatusLogLine>& log2) {
  for (const auto& item : log2) {
    // Emit this intermediate log to the Glog filesystem logger.
    google::LogMessage(item.filename.c_str(),
                       static_cast<int>(item.line),
                       (google::LogSeverity)item.severity)
            .stream()
        << item.message;
  }

  return Status(0, "OK");
}

Status FilesystemLogger2Plugin::logSnapshot(const std::string& s) {
  // Send the snapshot data to a separate filename.
  //std::cout<<"ok"<<s<<std::endl;
  return logStringToFile(s, kFilesystemLogger2Snapshots);
}

void FilesystemLogger2Plugin::init(const std::string& name,
                                  const std::vector<StatusLogLine>& log2) {
  // Stop the internal Glog facilities.
  google::ShutdownGoogleLogging();

  // The log dir is used for status logging and the filesystem results logs.
  if (isWritable(log2_path_.string()).ok()) {
    FLAGS_log_dir = log2_path_.string();
    FLAGS_logtostderr = false;
  } else {
    // If we cannot write logs to the filesystem, fallback to stderr.
    // The caller (flags/options) might 'also' be logging to stderr using
    // debug, verbose, etc.
    FLAGS_logtostderr = true;
  }

  // Restart the Glog facilities using the name `init` was provided.
  google::InitGoogleLogging(name.c_str());

  // We may violate Glog global object assumptions. So set names manually.
  auto basename = (log2_path_ / name).string();

  google::SetLogDestination(google::GLOG_INFO, (basename + ".INFO.").c_str());
  google::SetLogDestination(google::GLOG_WARNING,
                            (basename + ".WARNING.").c_str());
  google::SetLogDestination(google::GLOG_ERROR, (basename + ".ERROR.").c_str());

  // Store settings for logging to stderr.
  bool log2_to_stderr = FLAGS_logtostderr;
  bool also_log2_to_stderr = FLAGS_alsologtostderr;
  int stderr_threshold = FLAGS_stderrthreshold;
  FLAGS_alsologtostderr = false;
  FLAGS_logtostderr = false;
  FLAGS_stderrthreshold = 5;

  // Now funnel the intermediate status logs provided to `init`.
  logStatus(log2);

  // The filesystem logger cheats and uses Glog to log to the filesystem so
  // we can return failure here and stop the custom log sink.
  // Restore settings for logging to stderr.
  FLAGS_logtostderr = log2_to_stderr;
  FLAGS_alsologtostderr = also_log2_to_stderr;
  FLAGS_stderrthreshold = stderr_threshold;
}
}
