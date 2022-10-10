#include <LittleFS.h>
#include "ConfigStore.h"

/**
 * This implementation uses a separate file for each config key. All such files are stored in the same directory.
 *
 * I'm too tired of C-flavored C++ to look into doing things properly with all values in a single file.
 * So, here's to hoping this won't come and bite me in the near future. Cheers.
 */

void ConfigStore::load()
{
  // Check if config directory exists
  if (!LittleFS.exists(path))
  {
    // Create it if not
    LittleFS.mkdir(path);
  }

  Dir configDir = LittleFS.openDir(path);
  while (configDir.next())
  {
    if (configDir.fileSize() > 0 && configDir.isFile())
    { // no recursing, ignore directories.
      File f = configDir.openFile("r");
      configData[configDir.fileName()] = f.readString(); // open file, load contents in RAM
      f.close();
    }
  }
}

void ConfigStore::save()
{
  for (std::map<String, String>::iterator p = configData.begin(); p != configData.end(); p++)
  {
    String key = p->first;
    String value = p->second;
    File file = LittleFS.open(path + "/" + key, "w+");
    file.write(value.c_str());
    file.close();
  }
}

bool ConfigStore::contains(String key)
{
  return configData.count(key) > 0;
}

void ConfigStore::unset(String key)
{
  configData.erase(key);

  if (LittleFS.exists(path + "/" + key))
  {
    LittleFS.remove(path + "/" + key);
  }
}

void ConfigStore::setString(String key, String value)
{
  configData[key] = value;
  save();
}

String ConfigStore::getString(String key, String defaultValue)
{
  if (!contains(key))
  {
    setString(key, defaultValue);
  }
  return configData[key];
}