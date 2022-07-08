// SWS standard page
#include <Arduino.h>
#include "Page.h"

#include "Pages.common.h"

void pageHeader(int selected) {
  char temp[80];
  String data = "";

  data.concat(FPSTR(html_onstep_header_begin));
  
  data.concat("OnStep");
  data.concat(FPSTR(html_onstep_header_title));
 // data.concat(firmwareVersion.str);
  data.concat(" (OnStep");
  if (status.getVersionStr(temp)) data.concat(temp); else data.concat("?");
  www.sendContentAndClear(data);

  data.concat(FPSTR(html_onstep_header_links));

  data.concat(FPSTR(html_links_idx_begin));
  if (selected == PAGE_CONTROLLER) data.concat(FPSTR(html_links_selected));
  data.concat(FPSTR(html_links_idx_end));

  if (status.mountFound) {
    data.concat(FPSTR(html_links_mnt_begin));
    if (selected == PAGE_MOUNT) data.concat(FPSTR(html_links_selected));
    data.concat(FPSTR(html_links_mnt_end));
  }

  if (status.rotatorFound) {
    data.concat(FPSTR(html_links_rot_begin));
    if (selected == PAGE_ROTATOR) data.concat(FPSTR(html_links_selected));
    data.concat(FPSTR(html_links_rot_end));
    www.sendContentAndClear(data);
  }

  if (status.focuserFound) {
    data.concat(FPSTR(html_links_foc_begin));
    if (selected == PAGE_FOCUSER) data.concat(FPSTR(html_links_selected));
    data.concat(FPSTR(html_links_foc_end));
  }

  if (status.auxiliaryFound) {
    data.concat(FPSTR(html_links_aux_begin));
    if (selected == PAGE_AUXILIARY) data.concat(FPSTR(html_links_selected));
    data.concat(FPSTR(html_links_aux_end));
  }

  data.concat(FPSTR(html_onstep_header_end));
  www.sendContentAndClear(data);
}
