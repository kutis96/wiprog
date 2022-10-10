#ifndef CONFIGSTORE_H
#define CONFIGSTORE_H

#include <map>
#include <string>

class ConfigStore
{
public:
  ConfigStore(String _path = "/config") : path(_path){};
  void save();
  void load();
  bool contains(String key);
  void setString(String key, String value);
  String getString(String key, String defaultValue);
  void unset(String key);

private:
  String path;
  std::map<String, String> configData = {};
};

#endif