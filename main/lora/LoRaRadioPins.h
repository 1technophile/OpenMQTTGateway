#ifndef LoRaRadioPins_h
#define LoRaRadioPins_h

// Historical OpenMQTTGateway SX127x defaults. Board environments can
// override every pin; the Heltec V4 profile does so for its SX1262 wiring.
#ifndef LORA_SCK
#  define LORA_SCK 5
#endif

#ifndef LORA_MISO
#  define LORA_MISO 19
#endif

#ifndef LORA_MOSI
#  define LORA_MOSI 27
#endif

#ifndef LORA_SS
#  define LORA_SS 18
#endif

#ifndef LORA_RST
#  define LORA_RST 14
#endif

#ifndef LORA_DI0
#  define LORA_DI0 26
#endif

// Preserve the historical LORA_DI0 spelling used by existing environments.
#ifndef LORA_DIO0
#  define LORA_DIO0 LORA_DI0
#endif

#ifndef LORA_DIO1
#  define LORA_DIO1 -1
#endif

#ifndef LORA_BUSY
#  define LORA_BUSY -1
#endif

#endif
