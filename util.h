#include <optional>
#include <string>

#define CURSOR_UP_ESC ("\u001b[1A")

#define COLOR_ESC(R, G, B)                                                     \
  ("\u001b[38;2;" + std::to_string (R) + ";" + std::to_string (G) + ";" +      \
   std::to_string (B) + "m")
#define BLACK_ESC ("\u001b[30m")
#define RED_ESC ("\u001b[31m")
#define DEFAULT_COLOR_ESC ("\u001b[39m")

#define BG_COLOR_ESC(R, G, B)                                                  \
  ("\u001b[48;2;" + std::to_string (R) + ";" + std::to_string (G) + ";" +      \
   std::to_string (B) + "m")
#define DEFAULT_BG_COLOR_ESC ("\u001b[49m")

#define BOLD_ESC ("\u001b[1m")
#define DEFAULT_STYLE_ESC ("\u001b[22m")

#define CLEAR_ESC ("\u001b[2J")

std::optional<std::string>
fileToString (const std::string& path);
