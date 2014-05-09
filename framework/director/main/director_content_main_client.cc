#include "director/main/director_content_main_client.h"

#include "director/main/director_main_parts.h"

namespace director {

DirectorContentMainClient::DirectorContentMainClient()
    : director_main_parts_(NULL) {
}

DirectorContentMainClient::~DirectorContentMainClient() {
}

content::MainParts* DirectorContentMainClient::CreateBrowserMainParts(
    const content::MainFunctionParams& parameters) {
  director_main_parts_ =  new DirectorMainParts(parameters);
  return director_main_parts_;
}

}  // namespace director
