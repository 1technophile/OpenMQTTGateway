#pragma once
// Make compatible with both Arduino and native testing environments
#ifndef PROGMEM
#  define PROGMEM
#endif

static const char* ss_client_key PROGMEM = R"EOF("
-----BEGIN RSA PRIVATE KEY-----
...
-----END RSA PRIVATE KEY-----
")EOF";
