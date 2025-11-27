#ifdef ESP32
#  include "NVSPreferencesStorage.h"

#  include <User_config.h>

NVSPreferencesStorage::NVSPreferencesStorage()
    : preferences(new Preferences()), ownsPreferences(true) {}

NVSPreferencesStorage::NVSPreferencesStorage(Preferences* inPreferences)
    : preferences(inPreferences), ownsPreferences(false) {}

NVSPreferencesStorage::~NVSPreferencesStorage() {
  if (preferences) {
    preferences->end();
    if (ownsPreferences) {
      delete preferences;
    }
  }
}

const char* NVSPreferencesStorage::getNamespace() {
  return Gateway_Short_Name;
}

bool NVSPreferencesStorage::begin(bool readOnly) {
  return preferences->begin(getNamespace(), readOnly);
}

void NVSPreferencesStorage::end() {
  preferences->end();
}

bool NVSPreferencesStorage::isKey(const char* key) {
  return preferences->isKey(key);
}

const char* NVSPreferencesStorage::getString(const char* key, const char* defaultValue) {
  String arduinoString = preferences->getString(key, defaultValue);
  return arduinoString.c_str();
}

size_t NVSPreferencesStorage::putString(const char* key, const char* value) {
  String arduinoString(value);
  return preferences->putString(key, arduinoString);
}

int NVSPreferencesStorage::remove(const char* key) {
  return preferences->remove(key);
}
#endif