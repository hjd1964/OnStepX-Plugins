// -----------------------------------------------------------------------------------
// StallGuard monitor tile
#pragma once

#include "../htmlHeaders.h"
#include "../htmlMessages.h"
#include "../htmlScripts.h"

#if DISPLAY_STALLGUARD_MONITOR == ON

extern void stallGuardTile(String &data);
extern void stallGuardTileAjax(String &data);
extern void stallGuardTileGet();

const char html_stallGuardSelect[] PROGMEM = "<button id='sgax%d' onpointerdown=\"s('sgax','%d')\" type='button' class='bb'>%c</button>";

const char html_stallGuardGraph[] PROGMEM = "<canvas id='stallGuardCanvas' width='420' height='220' style='border:1px solid #000000;'></canvas> \n";

const char html_stallGuardScript[] PROGMEM =
  "<script>\n"
  "var sgu=setInterval(updateStallGuard,500);"
  "var sgr=false;"
  "var sgtm=0;"
  "var sgData=[];"
  "var sgTrip=[];"
  "var sgw=210;"
  "var sgh=220;"
  "var sgx=0;"
  "var sgy=0;"
  "var sgty=0;\n"

  "function updateStallGuard(){"
    "var canvas=document.getElementById('stallGuardCanvas');"
    "var ctx=canvas.getContext('2d');"
    "var sgText=document.getElementById('sgV').innerText;"
    "var tripText=document.getElementById('sgT').innerText;"
    "if(sgText=='?'||tripText=='?'){inactiveStallGuard(ctx);sgr=false;return;}"
    "var sg=Number(sgText);"
    "var trip=Number(tripText);"
    "if(sg<0){inactiveStallGuard(ctx);sgr=false;return;}"
    "if(!sgr){for(i=0;i<sgw;i++){sgData[i]=0;sgTrip[i]=0;}sgr=true;sgtm=0;sgx=0;sgy=sgh;sgty=sgh;clearStallGuard(ctx);}"
    "sgtm++;"
    "var x=sgtm;"
    "if(x>=sgw-1){x=sgw-1;for(i=1;i<=x;i++){sgData[i-1]=sgData[i];sgTrip[i-1]=sgTrip[i];}clearStallGuard(ctx);}"
    "sgData[x]=sg;"
    "sgTrip[x]=trip;"
    "var y=sgh-sg*(sgh/1023);"
    "var ty=sgh-trip*(sgh/1023);"
    "ctx.beginPath();ctx.moveTo(sgx*2,sgy);ctx.lineTo(x*2,y);ctx.strokeStyle='" COLOR_SERVO_PEN_4 "';ctx.stroke();"
    "ctx.beginPath();ctx.moveTo(sgx*2,sgty);ctx.lineTo(x*2,ty);ctx.strokeStyle='" COLOR_SERVO_PEN_2 "';ctx.stroke();"
    "sgx=x;sgy=y;sgty=ty;"
  "}\n"

  "function inactiveStallGuard(ctx){"
    "ctx.fillStyle='" COLOR_SERVO_BACKGROUND_1 "';"
    "ctx.fillRect(0,0,sgw*2,sgh);"
    "ctx.fillStyle='" COLOR_SERVO_BACKGROUND_2 "';"
    "ctx.font='bold 36px Arial';"
    "ctx.fillText('Inactive',sgw-70,sgh/2-9);"
  "}\n"

  "function clearStallGuard(ctx){"
    "ctx.fillStyle='" COLOR_SERVO_BACKGROUND_1 "';"
    "ctx.fillRect(0,0,sgw*2,sgh);"
    "ctx.strokeStyle='" COLOR_SERVO_PEN_1 "';"
    "ctx.beginPath();"
    "for(i=60;i<sgw*2;i+=60){ctx.moveTo(i,0);ctx.lineTo(i,sgh);}"
    "ctx.stroke();"
    "ctx.strokeStyle='" COLOR_SERVO_PEN_3 "';"
    "ctx.beginPath();ctx.moveTo(0,sgh/2);ctx.lineTo(sgw*2,sgh/2);ctx.stroke();"
    "ctx.strokeStyle='" COLOR_SERVO_PEN_2 "';"
    "ctx.beginPath();ctx.moveTo(0,sgh*0.25);ctx.lineTo(sgw*2,sgh*0.25);ctx.moveTo(0,sgh*0.75);ctx.lineTo(sgw*2,sgh*0.75);ctx.stroke();"
    "ctx.fillStyle='" COLOR_SERVO_BACKGROUND_3 "';"
    "ctx.font='bold 12px Arial';"
    "ctx.textBaseline='top';"
    "ctx.fillText('1023',2,2);"
    "ctx.fillText('768',2,sgh*0.25+2);"
    "ctx.fillText('512',2,sgh/2+2);"
    "ctx.fillText('256',2,sgh*0.75+2);"
    "ctx.fillText('SG',sgw*2-52,2);"
    "ctx.fillText('Trip',sgw*2-52,18);"
    "ctx.fillStyle='" COLOR_SERVO_PEN_4 "';ctx.fillRect(sgw*2-72,6,14,3);"
    "ctx.fillStyle='" COLOR_SERVO_PEN_2 "';ctx.fillRect(sgw*2-72,22,14,3);"
    "ctx.strokeStyle='" COLOR_SERVO_PEN_4 "';"
    "ctx.beginPath();sgx=0;sgy=sgh-sgData[0]*(sgh/1023);"
    "for(i=0;i<=sgtm;i++){ctx.moveTo(sgx*2,sgy);sgx=i;sgy=sgh-sgData[i]*(sgh/1023);ctx.lineTo(sgx*2,sgy);}ctx.stroke();"
    "ctx.strokeStyle='" COLOR_SERVO_PEN_2 "';"
    "ctx.beginPath();sgx=0;sgty=sgh-sgTrip[0]*(sgh/1023);"
    "for(i=0;i<=sgtm;i++){ctx.moveTo(sgx*2,sgty);sgx=i;sgty=sgh-sgTrip[i]*(sgh/1023);ctx.lineTo(sgx*2,sgty);}ctx.stroke();"
  "}\n"
  "</script>\n";

#endif
