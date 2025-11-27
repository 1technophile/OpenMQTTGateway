#pragma once
#include <TheengsLogs.h>
#include <config_JSONMessages.h>
#include <storage/IJsonable.h>
#include <storage/IStorage.h>

class AbstractStorageObject : public IJsonable {
public:
  AbstractStorageObject(IStorage& storageRef, const char* rootKey)
      : storage(storageRef), rootStorageKey(rootKey) {}

  virtual ~AbstractStorageObject() = default;

  virtual bool saveOnStorage() {
    bool out = false;
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject jo = jsonBuffer.to<JsonObject>();
    this->to(jo);
    char conf[JSON_MSG_BUFFER] = {0};
    const size_t written = serializeJson(jsonBuffer, conf, sizeof(conf));
    if (written >= sizeof(conf)) {
      THEENGS_LOG_ERROR(F("Storage save overflow: %u >= %u" CR), written, sizeof(conf));
      return false;
    } else {
      storage.begin(false);
      int result = storage.putString(rootStorageKey, conf);
      out = (result > 0);
      storage.end();
      THEENGS_LOG_NOTICE(F("Storage saved: %s, result: %d" CR), conf, result);
      return out;
    }
  };

  virtual bool loadFromStorage() {
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    storage.begin(true);

    if (storage.isKey(rootStorageKey)) {
      auto error = deserializeJson(jsonBuffer, storage.getString(rootStorageKey, "{}"));
      storage.end();
      if (error) {
        THEENGS_LOG_ERROR(F("%s deserialization failed: %s, buffer capacity: %u" CR), rootStorageKey, error.c_str(), jsonBuffer.capacity());
        return false;
      }
      if (jsonBuffer.isNull()) {
        THEENGS_LOG_WARNING(F("%s is null" CR), rootStorageKey);
        return false;
      }
      JsonObject jo = jsonBuffer.as<JsonObject>();
      this->from(jo);
      THEENGS_LOG_NOTICE(F("%s loaded" CR), rootStorageKey);
    } else {
      storage.end();
    }
    return true;
  };

  virtual bool eraseStorage() {
    bool state = false;
    storage.begin(false);
    if (storage.isKey(rootStorageKey)) {
      int result = storage.remove(rootStorageKey);
      THEENGS_LOG_NOTICE(F("%s erase result: %d" CR), rootStorageKey, result);
      state = (result == 1);
    } else {
      THEENGS_LOG_NOTICE(F("%s not found" CR), rootStorageKey);
    }
    storage.end();
    return state;
  }

private:
  IStorage& storage;
  const char* rootStorageKey;
};