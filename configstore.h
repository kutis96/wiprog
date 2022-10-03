#if (ARDUINO >= 100)
#include "Arduino.h"
#endif

#include <map>

#ifndef CONFIGSTORE_H
#define CONFIGSTORE_H

class ConfigStore {
  public:
    ConfigStore(String _path = "/config") : path(_path) {};
    void   save();
    void   load();
    bool   contains(char *key);
    void   setString(char *key, String value);
    String getString(char *key, String defaultValue);
    void   unset(char *key);
  private:
    String path;
    std::map<String, String> configData = {};
};

#endif