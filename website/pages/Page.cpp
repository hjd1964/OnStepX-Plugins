// SWS standard page
#include <Arduino.h>
#include "Page.h"

#include "Pages.common.h"

void pageHeader(int selected) {
  char temp[80];
  String data = "";

  data.concat(FPSTR(html_onstep_header_begin));
  
  if (strlen(status.configName) > 0) data.concat(status.configName); else data.concat(F("OnStep"));
  data.concat(FPSTR(html_onstep_header_title));
  data.concat(F(" (OnStep"));
  if (status.getVersionStr(temp)) data.concat(temp); else data.concat("?");
  www.sendContentAndClear(data);

  data.concat(FPSTR(html_onstep_header_links));

  data.concat(FPSTR(html_links_idx_begin));
  if (selected == PAGE_CONTROLLER) data.concat(FPSTR(html_links_selected));
  data.concat(FPSTR(html_links_idx_end));

  if (status.mountFound == SD_TRUE) {
    data.concat(FPSTR(html_links_mnt_begin));
    if (selected == PAGE_MOUNT) data.concat(FPSTR(html_links_selected));
    data.concat(FPSTR(html_links_mnt_end));
  }

  if (status.rotatorFound == SD_TRUE) {
    data.concat(FPSTR(html_links_rot_begin));
    if (selected == PAGE_ROTATOR) data.concat(FPSTR(html_links_selected));
    data.concat(FPSTR(html_links_rot_end));
    www.sendContentAndClear(data);
  }

  if (status.focuserFound == SD_TRUE) {
    data.concat(FPSTR(html_links_foc_begin));
    if (selected == PAGE_FOCUSER) data.concat(FPSTR(html_links_selected));
    data.concat(FPSTR(html_links_foc_end));
  }

  if (status.auxiliaryFound == SD_TRUE) {
    data.concat(FPSTR(html_links_aux_begin));
    if (selected == PAGE_AUXILIARY) data.concat(FPSTR(html_links_selected));
    data.concat(FPSTR(html_links_aux_end));
  }

  if (status.onStepFound) {
    data.concat(FPSTR(html_links_net_begin));
    if (selected == PAGE_NETWORK) data.concat(FPSTR(html_links_selected));
    data.concat(FPSTR(html_links_net_end));
  }

  #if defined(WEBSITE_PLUGIN_PAGE1_TITLE) && defined(WEBSITE_PLUGIN_PAGE1_URL)
    #ifndef WEBSITE_PLUGIN_PAGE1_TARGET
      #define WEBSITE_PLUGIN_PAGE1_TARGET ""
    #endif
    data.concat(F("<a href='" WEBSITE_PLUGIN_PAGE1_URL "' target='" WEBSITE_PLUGIN_PAGE1_TARGET "'>" WEBSITE_PLUGIN_PAGE1_TITLE "</a>"));
  #endif

  #if defined(WEBSITE_PLUGIN_PAGE2_TITLE) && defined(WEBSITE_PLUGIN_PAGE2_URL)
    #ifndef WEBSITE_PLUGIN_PAGE2_TARGET
      #define WEBSITE_PLUGIN_PAGE2_TARGET ""
    #endif
    data.concat(F("<a href='" WEBSITE_PLUGIN_PAGE2_URL "' target='" WEBSITE_PLUGIN_PAGE2_TARGET "'>" WEBSITE_PLUGIN_PAGE2_TITLE "</a>"));
  #endif

  #if defined(WEBSITE_PLUGIN_PAGE3_TITLE) && defined(WEBSITE_PLUGIN_PAGE3_URL)
    #ifndef WEBSITE_PLUGIN_PAGE3_TARGET
      #define WEBSITE_PLUGIN_PAGE3_TARGET ""
    #endif
    data.concat(F("<a href='" WEBSITE_PLUGIN_PAGE3_URL "' target='" WEBSITE_PLUGIN_PAGE3_TARGET "'>" WEBSITE_PLUGIN_PAGE3_TITLE "</a>"));
  #endif

  #if defined(WEBSITE_PLUGIN_PAGE4_TITLE) && defined(WEBSITE_PLUGIN_PAGE4_URL)
    #ifndef WEBSITE_PLUGIN_PAGE4_TARGET
      #define WEBSITE_PLUGIN_PAGE4_TARGET ""
    #endif
    data.concat(F("<a href='" WEBSITE_PLUGIN_PAGE4_URL "' target='" WEBSITE_PLUGIN_PAGE4_TARGET "'>" WEBSITE_PLUGIN_PAGE4_TITLE "</a>"));
  #endif

  #if defined(WEBSITE_PLUGIN_PAGE5_TITLE) && defined(WEBSITE_PLUGIN_PAGE5_URL)
    #ifndef WEBSITE_PLUGIN_PAGE5_TARGET
      #define WEBSITE_PLUGIN_PAGE5_TARGET ""
    #endif
    data.concat(F("<a href='" WEBSITE_PLUGIN_PAGE5_URL "' target='" WEBSITE_PLUGIN_PAGE5_TARGET "'>" WEBSITE_PLUGIN_PAGE5_TITLE "</a>"));
  #endif



  data.concat(FPSTR(html_onstep_header_end));
  www.sendContentAndClear(data);
}
