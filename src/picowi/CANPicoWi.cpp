/*
   MERG CBUS Module Library - RasberryPi Pico SDK port
   Copyright (c) Kevin Kimber 2023

   Based on work by Duncan Greenwood
   Copyright (C) Duncan Greenwood 2017 (duncan_greenwood@hotmail.com)

   This work is licensed under the:
      Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
   To view a copy of this license, visit:
      http://creativecommons.org/licenses/by-nc-sa/4.0/
   or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.

   License summary:
    You are free to:
      Share, copy and redistribute the material in any medium or format
      Adapt, remix, transform, and build upon the material

    The licensor cannot revoke these freedoms as long as you follow the license terms.

    Attribution : You must give appropriate credit, provide a link to the license,
                  and indicate if changes were made. You may do so in any reasonable manner,
                  but not in any way that suggests the licensor endorses you or your use.

    NonCommercial : You may not use the material for commercial purposes. **(see note below)

    ShareAlike : If you remix, transform, or build upon the material, you must distribute
                 your contributions under the same license as the original.

    No additional restrictions : You may not apply legal terms or technological measures that
                                 legally restrict others from doing anything the license permits.

   ** For commercial use, please contact the original copyright holder(s) to agree licensing terms

    This software is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE

*/

// CBUS library header files

#include "CBUSACAN2040.h" // CAN controller and CBUS class
#include "CBUSSwitch.h"   // CBUS FLiM pushbutton switch
#include "CBUSLED.h"      // CBUS LEDs
#include "CBUSConfig.h"   // CBUS module configuration
#include "CBUSParams.h"   // CBUS parameters
#include "cbusdefs.h"     // MERG CBUS constants
#include "CBUSUtil.h"     // Utility macros

#include "CBUSWiFi.h"        // CBUS WiFi support
#include "CBUSGridConnect.h" // CBUS Grid Connect support

#include <cstdio>
#include <pico/stdlib.h>
#include <pico/binary_info.h>

// constants
constexpr uint8_t VER_MAJ = 1;   ///< module code major version
constexpr char VER_MIN = 'a';    ///< module code minor version
constexpr uint8_t VER_BETA = 2;  ///< module code beta sub-version
constexpr uint8_t MODULEID = 99; ///< CBUS module type

// Map CBUS LED's switch to HW
constexpr uint8_t LED_GRN = 9;  ///< CBUS Green SLiM LED pin
constexpr uint8_t LED_YLW = 15; ///< CBUS Yellow FLiM LED pin
constexpr uint8_t SWITCH0 = 22; ///< CBUS FLiM push button switch pin

// Map CAN2040 Tx and Rx pins
constexpr uint8_t CAN_RX = 14; ///< CAN2040 Rx pin
constexpr uint8_t CAN_TX = 13; ///< CAN2040 Tx pin

// Map Module IO
constexpr uint8_t MODULE_LED = 8;    ///< Module LED
constexpr uint8_t MODULE_SWITCH = 0; ///< Module Switch

// CBUS objects
CBUSConfig module_config; // configuration object

// Construct CBUS Object and assign the module configuration
CBUSACAN2040 CBUS(module_config);

// module objects
CBUSSwitch moduleSwitch; // an example switch as input
CBUSLED moduleLED;       // an example LED as output

// Consume own events
CBUScoe coe;

// CBUS WiFI
CBUSWiFi wifi;

// CBUS Grid Connect
CBUSGridConnect gcServer;

// module name, must be 7 characters, space padded.
module_name_t moduleName = {'P', 'i', 'c', 'o', 'W', 'i', ' '};

// forward function declarations
void eventhandler(uint8_t index, const CANFrame &msg);
void processModuleSwitchChange(void);

//
/// setup CBUS - runs once at power on from setup()
//

void setupCBUS()
{
   // Declare binary info for Picotool
   bi_decl(bi_program_description("CBUS Pico WiFi module"));

   // Notify pin setup for Picotool
   bi_decl(bi_1pin_with_name(LED_GRN, "CBUS Green LED"));
   bi_decl(bi_1pin_with_name(LED_YLW, "CBUS Yellow LED"));
   bi_decl(bi_1pin_with_name(SWITCH0, "CBUS FLiM Switch"));
   bi_decl(bi_1pin_with_name(CAN_TX, "CAN2040 Tx"));
   bi_decl(bi_1pin_with_name(CAN_RX, "CAN2040 Rx"));

   bi_decl(bi_1pin_with_name(MODULE_LED, "Module LED"));
   bi_decl(bi_1pin_with_name(MODULE_SWITCH, "Module Switch"));

   // set config layout parameters
   module_config.EE_NVS_START = 10;    // Offset start of Node Variables
   module_config.EE_NUM_NVS = 10;      // Number of Node Variables
   module_config.EE_EVENTS_START = 20; // Offset start of Events
   module_config.EE_MAX_EVENTS = 32;   // Maximum number of events
   module_config.EE_NUM_EVS = 1;       // Number of Event Variables per event
   module_config.EE_BYTES_PER_EVENT = (module_config.EE_NUM_EVS + 4);

   // initialise and load configuration
   module_config.setEEPROMtype(EEPROM_TYPE::EEPROM_USES_FLASH);
   module_config.begin();

   // set module parameters
   CBUSParams params(module_config);
   params.setVersion(VER_MAJ, VER_MIN, VER_BETA);
   params.setModuleId(MODULEID);
   params.setFlags(PF_FLiM | PF_COMBI);

   // assign to CBUS
   CBUS.setParams(params.getParams());
   CBUS.setName(&moduleName);
   CBUS.consumeOwnEvents(&coe);
   CBUS.setGridConnectServer(&gcServer);

   // Get the internal CBUS UI objects
   CBUSLED &ledGrn = CBUS.getCBUSGreenLED();
   CBUSLED &ledYlw = CBUS.getCBUSYellowLED();
   CBUSSwitch &sw = CBUS.getCBUSSwitch();

   // set CBUS LED pins
   ledGrn.setPin(LED_GRN);
   ledYlw.setPin(LED_YLW);

   // initialise CBUS switch
   sw.setPin(SWITCH0, false);
   sw.run();

   // module reset - if switch is depressed at startup and module is in SLiM mode
   if (sw.isPressed() && !module_config.getFLiM())
   {
      module_config.resetModule(ledGrn, ledYlw, sw);
   }

   // opportunity to set default NVs after module reset
   if (module_config.isResetFlagSet())
   {
      module_config.clearResetFlag();
   }

   // register our CBUS event handler, to receive event messages of learned events
   CBUS.setEventHandlerCB(eventhandler);

   // set CBUS LEDs to indicate mode
   CBUS.indicateFLiMMode(module_config.getFLiM());

   // configure and start CAN bus and CBUS message processing
   CBUS.setNumBuffers(16, 4);    // more buffers = more memory used, fewer = less
   CBUS.setPins(CAN_TX, CAN_RX); // select pins for CAN tx and rx

   if (!CBUS.begin())
   {
      // Init OK
   }
}

//
/// setup - runs once at power on
//

void setup()
{
   // Setup CBUS Library
   setupCBUS();

   // configure the module switch, attached to GP0, active low
   moduleSwitch.setPin(MODULE_SWITCH, false);

   // configure the module LED, attached to Red LED GP8 via a 1K resistor
   moduleLED.setPin(MODULE_LED);

   // Connect WiFi
   if (!wifi.InitializeClient())
   {
      while (1)
      {
         // Blink the onboard LED
         cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
         sleep_ms(250);
         cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
         sleep_ms(250);
      }
   }

   // Turn onboard LED ON now we're connected
   cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

   // Map configuration settings to NV's
   uint8_t NVs[module_config.EE_NUM_NVS]{};

   NVs[0] = wifi.isGridConnectEnabled();               // NV1 Grid Connect enable
   NVs[1] = (wifi.getGridConnectPort() & 0xFF00) >> 8; // NV2 Grid Connect Port upper byte
   NVs[2] = (wifi.getGridConnectPort() & 0x00FF);      // NV3 Grid Connect Port lower byte
   NVs[3] = wifi.isEdThrottleEnabled();                // NV4 Engine Driver Throttle enable
   NVs[4] = (wifi.getEdThrottlePort() & 0xFF00) >> 8;  // NV5 Engine Driver Throttle Port upper byte
   NVs[5] = (wifi.getEdThrottlePort() & 0x00FF);       // NV6 Engine Driver Throttle Port lower byte

   // Set country code into NVs from the INI, assuming we have 2 chars or more
   const char* countryCode = wifi.getCountryCode();
   if (strlen(countryCode) >= 2)
   {
      NVs[6] = countryCode[0];
      NVs[7] = countryCode[1];
   }

   // Set NVs for WPA/WPA2 authentication flags
   NVs[8] = wifi.getWPAEnable();
   NVs[9] = wifi.getWPA2Enable();

   // Block write the NV's from the configuration settings from the SD card
   module_config.writeBytesEEPROM(module_config.EE_NVS_START, NVs, module_config.EE_NUM_NVS);

   // Initialize web server
   wifi.InitWebServer();

   // Start Grid Connect server?
   if (wifi.isGridConnectEnabled())
   {
      // Initialize GC server
      gcServer.startServer(wifi.getGridConnectPort());
   }

   // Start Engine Driver throttle server?
   if (wifi.isEdThrottleEnabled())
   {
      // Intialize Throttle server
      wifi.getEdThrottlePort();
   }
}

//
/// loop - runs forever
//

void loop()
{
   //
   /// do CBUS message, switch and LED processing
   //

   CBUS.process();

   //
   /// give the switch and LED code some time to run
   //

   moduleSwitch.run();
   moduleLED.run();

   //
   /// Check if switch changed and do any processing for this change.
   //

   processModuleSwitchChange();
}

//
/// test for switch input
/// as an example, it must be have been pressed or released for at least half a second
/// then send a long CBUS event with opcode ACON for on and ACOF for off
/// event number (EN) is 1

/// you can just watch for this event in FCU or JMRI, or teach it to another CBUS consumer module
//
void processModuleSwitchChange()
{
   if (moduleSwitch.stateChanged())
   {
      if (CBUS.sendMyEvent(1U, moduleSwitch.isPressed()))
      {
         // Sent OK
      }
   }
}

//
/// user-defined event processing function
/// called from the CBUS library when a learned event is received
/// it receives the event table index and the CAN frame
//

void eventhandler(uint8_t index, const CANFrame &msg)
{
   // as an example, control an LED

   // read the value of the first event variable (EV) associated with this learned event
   uint8_t evval = module_config.getEventEVval(index, 1);

   // set the LED according to the opcode of the received event, if the first EV equals 0
   // we turn on the LED and if the first EV equals 1 we use the blink() method of the LED object as an example

   if (msg.data[0] == OPC_ACON)
   {
      if (evval == 0)
      {
         moduleLED.on();
      }
      else if (evval == 1)
      {
         moduleLED.blink();
      }
   }
   else if (msg.data[0] == OPC_ACOF)
   {
      moduleLED.off();
   }
}

// MODULE MAIN ENTRY

extern "C" int main(int, char **)
{
   // Init stdio lib (only really required if UART logging etc.)
   stdio_init_all();

#if LIB_PICO_STDIO_SEMIHOSTING
   // Setp CRLF options
   stdio_set_translate_crlf(&stdio_semihosting, false);

   printf("CANPiWi : Initializing\n");
#endif

   // Initialize
   setup();

   // Run periodic processing - forever
   while (1)
   {
      loop();
      /// @todo provision for RTOS and / or basic OSTick / WFI
      sleep_ms(1);
   }
}