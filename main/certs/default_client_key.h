#ifndef DEFAULT_CLIENT_KEY_H
#define DEFAULT_CLIENT_KEY_H

#include <pgmspace.h>

const char* ss_client_key PROGMEM = R"EOF("
-----BEGIN RSA PRIVATE KEY-----
...
-----END RSA PRIVATE KEY-----
")EOF";

#endif // DEFAULT_CLIENT_KEY_H