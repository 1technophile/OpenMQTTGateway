#ifndef DEFAULT_SERVER_CERT_H
#define DEFAULT_SERVER_CERT_H

#include <pgmspace.h>

const char* ss_server_cert PROGMEM = R"EOF("
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
")EOF";

#endif // DEFAULT_SERVER_CERT_H