// -----------------------------------------------------------------------------------
// PEC tile
#pragma once

#include "../htmlHeaders.h"
#include "../htmlMessages.h"
#include "../htmlScripts.h"

extern void pecTile(String &data);
extern void pecTileAjax(String &data);
extern void pecTileGet();

const char html_pec2[] PROGMEM = 
"<div style='float: left'>" L_PEC_STATUS ":</div><br/>"
"<div id='pec_sta'>?</div><br class='clear' />";

const char html_pecControls0[] PROGMEM =
"<div class='b1' style='width: 20em'>"
"<div style='float: left'>" L_PEC_CONTROL ":</div><br />";
const char html_pecControls1[] PROGMEM =
"<form method='get' action='/pec.htm'>"
"<button name='pe' value='pl' type='submit'>" L_PEC_PLAY "</button>";
const char html_pecControls2[] PROGMEM =
"<button name='pe' value='st' type='submit'>" L_STOP "</button><br /><br />"
"<button name='pe' value='cl' type='submit'>" L_PEC_CLEAR "</button>"
"<button name='pe' value='re' type='submit'>" L_PEC_REC "</button><br />";
const char html_pecControls3[] PROGMEM =
L_PEC_CLEAR_MESSAGE "<br />";
const char html_pecControls4[] PROGMEM =
"<br /><button name='pe' value='wr' type='submit'>" L_PEC_EEWRITE "</button><br />" L_PEC_EEWRITE_MESSAGE "<br /><br />";
const char html_pecControls5[] PROGMEM =
"</form></div>"
"<br class='clear' /><br />\n";
