#include "director/main/director_main_delegate.h"

#include <string>
#include "base/logging_win.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "content/public/common/content_switches.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"
#include "director/main/director_content_browser_client.h"

namespace director {

namespace {

// {83FAC8EE-7A0E-4dbb-A3F6-6F500D7CAB1A}
const GUID kViewsExamplesProviderName =
    { 0x83fac8ee, 0x7a0e, 0x4dbb,
        { 0xa3, 0xf6, 0x6f, 0x50, 0xd, 0x7c, 0xab, 0x1a } };

}  // namespace

DirectorMainDelegate::DirectorMainDelegate() {
}

DirectorMainDelegate::~DirectorMainDelegate() {
}

bool DirectorMainDelegate::BasicStartupComplete(int* exit_code) {
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
  std::string process_type =
      command_line.GetSwitchValueASCII(switches::kProcessType);

  content::SetContentClient(&content_client_);

  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  bool success = logging::InitLogging(settings);
  CHECK(success);

  logging::LogEventProvider::Initialize(kViewsExamplesProviderName);

  return false;
}

void DirectorMainDelegate::PreSandboxStartup() {
  InitializeResourceBundle();
}

content::ContentBrowserClient*
    DirectorMainDelegate::CreateContentBrowserClient() {
  browser_client_.reset(new DirectorContentBrowserClient);
  return browser_client_.get();
}

void DirectorMainDelegate::InitializeResourceBundle() {
  ui::ResourceBundle::InitSharedInstanceWithLocale("en-US", NULL);
}

}  // namespace director
