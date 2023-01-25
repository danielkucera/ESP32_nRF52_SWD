/*
   Copyright (c) 2021 Aaron Christophel ATCnetz.de
   SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "defines.h"
#include "web.h"
#include "glitcher.h"
#include "nrf_swd.h"
#include "swd.h"

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager/tree/feature_asyncwebserver
#include <ESPmDNS.h>

#ifdef ENABLE_OTA
#include <ArduinoOTA.h>
#endif

WiFiManager wm;

void setup()
{
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  wm.setConfigPortalBlocking(false);
  wm.setEnableConfigPortal(true);
  wm.autoConnect("AutoConnectAP");

  // Make accessible via http://swd.local using mDNS responder
  if (!MDNS.begin("swd"))
  {
    while (1)
    {
      Serial.println("Error setting up mDNS responder!");
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  MDNS.addService("http", "tcp", 80);

  Serial.println(WiFi.localIP());

  swd_begin();
  glitcher_begin();
  init_web();
  delay(1000);
  Serial.printf("SWD Id: 0x%08x\r\n", nrf_begin());

#ifdef ENABLE_OTA
  ArduinoOTA.begin();
#endif
}

int flash_firmware(){
  uint32_t temp = swd_init();
  nrf_abort_all();
  
  if (temp != 0x2ba01477){
    return 1;
  }

  for (int addr=0; addr < 50000; addr+=4096){
    if (erase_page(0x8000 + addr)){
      return 1;
    }
  }

  String filename = String("/gas_meter.img");

  return flash_file(0x8000, filename);
}

#define FLASH_BUTTON 0

void loop()
{
#ifdef ENABLE_OTA
  ArduinoOTA.handle();
#endif

  wm.process();

  if(digitalRead(FLASH_BUTTON)==0){
    digitalWrite(LED, 1);

    if (flash_firmware()){
      Serial.println("Flash failed");
    } else {
      Serial.println("Flash OK!");
      digitalWrite(LED, 0);
    }

  }

  if (get_glitcher())
  {
    do_glitcher();
  }
  else
  {
    do_nrf_swd();
  }
}
