#ifndef DEFAULT_CLIENT_CERT_H
#define DEFAULT_CLIENT_CERT_H

#include <pgmspace.h>

const char* ss_client_cert PROGMEM = R"EOF("
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
")EOF";

#endif // DEFAULT_CLIENT_CERT_H