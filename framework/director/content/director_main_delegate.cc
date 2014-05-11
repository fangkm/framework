#include "director/content/director_main_delegate.h"

#include <string>
#include "base/logging_win.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"
#include "content/public/common/content_switches.h"
#include "director/content/director_content_main_client.h"

namespace director {

namespace {

// {39563016-6D52-49B5-B714-3ABB408B0E3C}
const GUID kDirectorProviderName =
    { 0x39563016, 0x6d52, 0x49b5, 
			{ 0xb7, 0x14, 0x3a, 0xbb, 0x40, 0x8b, 0xe, 0x3c } };

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

  logging::LogEventProvider::Initialize(kDirectorProviderName);

  return false;
}

void DirectorMainDelegate::PreSandboxStartup() {
  InitializeResourceBundle();
}

content::ContentMainClient* DirectorMainDelegate::CreateContentMainClient() {
  main_client_.reset(new DirectorContentMainClient);
  return main_client_.get();
}

void DirectorMainDelegate::InitializeResourceBundle() {
  ui::ResourceBundle::InitSharedInstanceWithLocale("en-US", NULL);
}

}  // namespace director
