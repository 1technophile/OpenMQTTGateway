#pragma once

// Make compatible with both Arduino and native testing environments
#ifndef PROGMEM
#  define PROGMEM
#endif

static const char* ss_client_cert PROGMEM = R"EOF("
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
")EOF";
