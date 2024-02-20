# Configuration INI

CANPicoWi requires a configuration file on an SD card, which is formatted as a FAT or FAT32 file system.

This configuration file, allows for configuration of your WiFi credentials, along with options as to if the Grid Connect server is able, and if so on which port, and the similar for the Engine Driver throttle service.

The file must be named 'config.ini' and needs to be loaded in the root directory of the SD card.  Below is a template example for the config.ini contents.  You need to change (at a minimum) the ssid and password values:

      ; MERG PicoWi configuration
      ; Example file:
      ; Modify and write to a FAT formatted SD Card

      [wifi]
      country = UK             ; Country code for WiFi
      ssid = XXXXXXXXX         ; WiFi router SSID
      password = XXXXXXXXXXXX  ; WiFi router password
      wpa_auth = false         ; Enable WiFi WPA authentication [WPA TPIP PSK]
      wpa2_auth = true         ; Enable WiFi WPA2 authentication [WP2 AES PSK]

      [gridconnect]
      enable = true            ; Enable CBUS Grid Connect server
      port = 5555              ; Port number for the Grid Connnect server

      [edthrottle]
      enable = true            ; Enable the Engine Driver Throttle server
      port = 4444              ; Port number for the ED Throttle server

## WiFi configuration

CANPicoWi currently only supports WiFi in station / router mode, i.e. the CANPicoWi module connects to a WiFi router, as such the CANPicoWi needs knowledge of the credentials to connect to the router.

The default config.ini, as above, is setup to authenticate with the router using WPA2, however this can be changed.

### Open mode

To connect to a router in open mode, aka no authentication, then set both wpa_auth and wpa2_auth false, i.e.:

      [wifi]
      country = UK             ; Country code for WiFi
      ssid = XXXXXXXXX         ; WiFi router SSID
      password = XXXXXXXXXXXX  ; WiFi router password
      wpa_auth = false         ; Enable WiFi WPA authentication [WPA TPIP PSK]
      wpa2_auth = false        ; Enable WiFi WPA2 authentication [WP2 AES PSK]

### WPA mode

To use the legacy WPA authentication method, set wpa_auth true and wpa2_auth false, i.e.:

      [wifi]
      country = UK             ; Country code for WiFi
      ssid = XXXXXXXXX         ; WiFi router SSID
      password = XXXXXXXXXXXX  ; WiFi router password
      wpa_auth = true          ; Enable WiFi WPA authentication [WPA TPIP PSK]
      wpa2_auth = false         ; Enable WiFi WPA2 authentication [WP2 AES PSK]

### WPA/WPA2 Mixed mode

To use mixed WPA/WPA2 authentication, set both wpa_auth and wpa2_auth true, i.e.:

      [wifi]
      country = UK             ; Country code for WiFi
      ssid = XXXXXXXXX         ; WiFi router SSID
      password = XXXXXXXXXXXX  ; WiFi router password
      wpa_auth = true          ; Enable WiFi WPA authentication [WPA TPIP PSK]
      wpa2_auth = true         ; Enable WiFi WPA2 authentication [WP2 AES PSK]

## WiFi Country configuration

The CANPicoWi WiFi configuration is currently hard coded to support UK WiFi frequencies.  The country setting in the config.ini is currently not read and used in firmware.

\warning **DO NOT** under any circumstances run the default CANPicoWi outside of the UK without modifying the source code to change the country code.  The country code can be set in CBUSWiFi.cpp, around line 220.

      // Set the country code 
      /// @todo setup from INI
      if (cyw43_arch_init_with_country(CYW43_COUNTRY_UK))
      {
         return false;
      }


<div class="section_buttons">
 
| Previous                            |            Next |
|:------------------------------------|----------------:|
| [Introduction](README.md)           | [CBUS](cbus.md) |
 
</div>
