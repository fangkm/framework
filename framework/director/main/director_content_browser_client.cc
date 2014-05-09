#include "director/main/director_content_browser_client.h"

#include "director/main/director_browser_main_parts.h"

namespace director {

DirectorContentBrowserClient::DirectorContentBrowserClient()
    : examples_browser_main_parts_(NULL) {
}

DirectorContentBrowserClient::~DirectorContentBrowserClient() {
}

content::BrowserMainParts* DirectorContentBrowserClient::CreateBrowserMainParts(
    const content::MainFunctionParams& parameters) {
  examples_browser_main_parts_ =  new DirectorBrowserMainParts(parameters);
  return examples_browser_main_parts_;
}

}  // namespace director
