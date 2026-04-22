#include <fstream>

#include "life_view.h"

std::optional<std::string>
fileToString (const std::string& path)
{
  // from David Haim https://stackoverflow.com/a/32186637
  std::ifstream fileReader (path, std::ios::binary | std::ios::ate);
  if (!fileReader.is_open ())
    return {};

  auto fileSize = fileReader.tellg ();
  fileReader.seekg (std::ios::beg);
  std::string content (fileSize, 0);
  fileReader.read (&content[0], fileSize);

  return content;
}
