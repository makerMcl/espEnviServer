# espEnviServer

Small web server for the AZ-Envy/espEnvi board by Niklas Heinzel built on top of makerMcl's universalUi module.

## Functionality
* show all available data from sensor
* provide long-term-logging via console log, exportable to your favorite spreadsheet tool

* demonstrates minimal efforts for building simple webUI using universalUi


## Build preparations
* check out project
* copy `universalUIsettings.h_sample` into `universalUIsettings.h` and provide WLAN and OTA settings
    * for MD5 generation you can use any (online) tool
    * configuration of WIFI credentials
    * define OTA authentication data (port and password to be accepted for OTA requests)
* copy and customize local instance of `platformio_local.ini`, use `platformio_local.ini_sample` as template
    * configure OTA authentication (to be used by OTA tool)

* copy static resources for webserver to SPIFFS:
    * replace empty favicon.ico - repo contains only empty file so you are free to choose
    * upload in PIO-Console using: `pio run --target uploadfs`


## Some comments on implementation
* keep your code DRY - that's one reason for the classes in universalUi
* I do prefer proper objects on heap (just like `MQ2 *mq2 = new MQ2(A0);` instead of objects on stack like `MQ2 mq2(A0);`) - observed some problems when using instances in classes, caused by dynamic stack allocation. The static approach only works with objects allocated in main sketch.
* universalUi is a static instance on heap, to allow use via external reference in sub-libs of project (not used this in this project, see mmControl for an example)
* SPIFFS is deprecated now, but still used by AsyncWebServer - thus the temporary deprecation disabling in method `serverSetup()`
* I prefer default baudrate of bootloader, thus using 74800 for Serial
* in loop you find a nice hack for executing a task periodically (but not in first loop): `if ((millis() % 5000) == 4000)`
* I prefer HTML template content (index.html, log.html) on SPIFFS not only to save flash but first because you can read that code more easily, including the template variables


## Licensing & Contact
Licensed under GPL v3.

Email: makerMcl (at) clauss.pro

Please only email me if it is more appropriate than creating an Issue / PR. I will not respond to requests for adding support for particular boards, unless of course you are the creator of the board and would like to cooperate on the project. I will also ignore any emails asking me to tell you how to implement your ideas. However, if you have a private inquiry that you would only apply to you and you would prefer it to be via email, by all means.

## Copyright

Copyright 2021 Matthias Clauﬂ

Note: To avoid unmonitored commercial use of this work while giving back to the community, I choose the GPL licence.

## TODOs
* improve styling of HTML
* REST-API, use AJAX/jQuery to update measurements on index.html
* bugfix: sometimes reboots (suspect is memory-overload from EspAsyncWebserver)
