// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/file_util.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_paths.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "content/test/content_browser_test.h"
#include "content/test/content_browser_test_utils.h"
#include "gpu/config/gpu_test_config.h"
#include "gpu/config/gpu_test_expectations_parser.h"
#include "net/base/net_util.h"

namespace content {

class WebGLConformanceTest : public ContentBrowserTest {
 public:
  WebGLConformanceTest() {}

  virtual void SetUpCommandLine(CommandLine* command_line) OVERRIDE {
    // Allow privileged WebGL extensions.
    command_line->AppendSwitch(switches::kEnablePrivilegedWebGLExtensions);
#if defined(OS_ANDROID)
    command_line->AppendSwitch(
        switches::kDisableGestureRequirementForMediaPlayback);
#endif
  }

  virtual void SetUpInProcessBrowserTestFixture() OVERRIDE {
    base::FilePath webgl_conformance_path;
    PathService::Get(base::DIR_SOURCE_ROOT, &webgl_conformance_path);
    webgl_conformance_path = webgl_conformance_path.Append(
        FILE_PATH_LITERAL("third_party"));
    webgl_conformance_path = webgl_conformance_path.Append(
        FILE_PATH_LITERAL("webgl_conformance"));
    ASSERT_TRUE(base::DirectoryExists(webgl_conformance_path))
        << "Missing conformance tests: " << webgl_conformance_path.value();

    PathService::Get(DIR_TEST_DATA, &test_path_);
    test_path_ = test_path_.Append(FILE_PATH_LITERAL("gpu"));
    test_path_ = test_path_.Append(FILE_PATH_LITERAL("webgl_conformance.html"));

    ASSERT_TRUE(bot_config_.LoadCurrentConfig(NULL))
        << "Fail to load bot configuration";
    ASSERT_TRUE(bot_config_.IsValid())
        << "Invalid bot configuration";

    base::FilePath path;
    ASSERT_TRUE(PathService::Get(DIR_TEST_DATA, &path));
    path = path.Append(FILE_PATH_LITERAL("gpu"))
        .Append(FILE_PATH_LITERAL("webgl_conformance_test_expectations.txt"));
    ASSERT_TRUE(base::PathExists(path));
    ASSERT_TRUE(test_expectations_.LoadTestExpectations(path));
  }

  void RunTest(std::string url, std::string test_name) {
    int32 expectation =
        test_expectations_.GetTestExpectation(test_name, bot_config_);
    if (expectation != gpu::GPUTestExpectationsParser::kGpuTestPass) {
      LOG(WARNING) << "Test " << test_name << " is bypassed";
      return;
    }

    DOMMessageQueue message_queue;
    NavigateToURL(shell(), net::FilePathToFileURL(test_path_));

    std::string message;
    NavigateToURL(shell(), GURL("javascript:start('" + url + "');"));
    ASSERT_TRUE(message_queue.WaitForMessage(&message));

    EXPECT_STREQ("\"SUCCESS\"", message.c_str()) << message;
  }

 private:
  base::FilePath test_path_;
  gpu::GPUTestBotConfig bot_config_;
  gpu::GPUTestExpectationsParser test_expectations_;
};

#define CONFORMANCE_TEST(name, url) \
IN_PROC_BROWSER_TEST_F(WebGLConformanceTest, MANUAL_##name) { \
  RunTest(url, #name);                                        \
}

// The test declarations are located in webgl_conformance_test_list_autogen.h,
// because the list is automatically generated by a script.
// See: generate_webgl_conformance_test_list.py
#include "webgl_conformance_test_list_autogen.h"

}  // namespace content