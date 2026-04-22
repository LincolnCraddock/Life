#include <charconv>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <string>
#include <tuple>

#include "life_view.h"
#include "util.h"

#define MOTION_TO_STR(MOTION)                                                  \
  ((MOTION).dx == 0 && (MOTION).dy == 0                                        \
     ? " "                                                                     \
     : (std::abs ((MOTION).dx / (MOTION).dy) <= 0.5                            \
          ? "|"                                                                \
          : (std::abs ((MOTION).dx / (MOTION).dy) >= 2                         \
               ? "-"                                                           \
               : ((MOTION).dx / std::abs ((MOTION).dx) ==                      \
                      (MOTION).dy / std::abs ((MOTION).dy)                     \
                    ? "\\"                                                     \
                    : "/"))))

#define SUBSTANCE_VAL_TO_RGB(SUB_VAL)                                          \
  (unsigned short) (log2 (SUB_VAL) * 255 / log2 (substance_t::MAX_VAL))
#define MOTION_TO_RGB(MOT)                                                     \
  (unsigned short) (log2 ((MOT).magnitude () + 1) * 100 /                      \
                      log2 (motion_t::MAX_MAGNITUDE) +                         \
                    155)

// int values for CLI specific actions
const int ALWAYS = -1, PRINT = -2, VIEW = -3, CLEAR = -4, CONTINUE = -5;

bool shouldIContinue = false;

// names for each action
const std::map<std::string, int> actionTable = {
  { "step", STEP },     { "s", STEP },          { "display", DISPLAY },
  { "disp", DISPLAY },  { "exit", EXIT },       { "quit", EXIT },
  { "help", HELP },     { "nothing", NOTHING }, { "always", ALWAYS },
  { "print", PRINT },   { "p", PRINT },         { "examine", PRINT },
  { "x", PRINT },       { "inspect", PRINT },   { "view", VIEW },
  { "viewport", VIEW }, { "v", VIEW },          { "window", VIEW },
  { "clear", CLEAR },   { "clr", CLEAR },       { "continue", CONTINUE }
};

// canonical names for each action
const std::map<int, std::string> oneToOneActionTable = {
  { STEP, "step" },  { DISPLAY, "display" }, { EXIT, "quit" },
  { HELP, "help" },  { NOTHING, "nothing" }, { ALWAYS, "always" },
  { PRINT, "print" }
};

inline std::ostream&
operator<< (std::ostream& os, Command const& arg)
{
  auto it = oneToOneActionTable.find (arg.action);
  if (it != oneToOneActionTable.end ())
    os << it->second;
  else
    os << "undefined";
  if (arg.repeat != 1)
    os << arg.repeat;
  if (arg.args != std::string {})
    os << arg.args;
  return os;
};

/* Forward declarations */

// prints a single character representing a 1 × 1 square in the world
void
printSquare (square_t sqr);

// prints a 2D grid of characters representing a view of the world
void
printWorld (const std::vector<std::vector<square_t>>& world);

// prints the values of each of the model settings
void
printSettings (const settings_t& settings);

// prints some instructions on how to use the CLI
void
printHelp ();

// prints the values of certain parts of the model, including the world,
// settings, etc.
//
// the string components should list the components to be printed
// space-separated (e.g. "square 1 1 square 1 2 always start cells")
void
printModelComponents (const std::string& components, const Model& model);

// parse a space-separated string of commands into a vector of commands
std::vector<Command>
parseCommands (const std::string& str);

/* Body */

rect_t viewport;

std::string lastInput = "display";
std::queue<Command> commands {};
std::vector<Command> alwaysDo {};

View::ViewInputStream&
View::ViewInputStream::operator>> (Command& cmd)
{
  if (commands.empty ())
  {
    if (shouldIContinue)
    {
      for (Command c : alwaysDo)
        commands.push (c);
    }
    else
    {
      /* Parse more input from user */
      std::string input;
      std::getline (std::cin, input);

      if (input == "")
      {
        input = lastInput;
        std::cout << CURSOR_UP_ESC << BLACK_ESC << lastInput
                  << DEFAULT_COLOR_ESC << std::endl;
      }

      for (Command c : parseCommands (input))
        commands.push (c);
      for (Command c : alwaysDo)
        commands.push (c);

      lastInput = input;
    }
  }

  cmd = commands.front ();
  commands.pop ();
  if (m_view->performImpCommand (cmd))
    cmd = { NOTHING };

  return *this;
}

template<>
View::ViewOutputStream&
View::ViewOutputStream::operator<< (const std::string& output)
{
  buf << output;
  return *this;
}

template<>
View::ViewOutputStream&
View::ViewOutputStream::operator<< (const size_t& output)
{
  buf << output;
  return *this;
}

template<>
View::ViewOutputStream&
View::ViewOutputStream::operator<< (const unsigned int& output)
{
  buf << output;
  return *this;
}

View::ViewOutputStream&
View::ViewOutputStream::operator<< (const char* output)
{
  buf << output;
  return *this;
}

template<>
View::ViewErrorStream&
View::ViewErrorStream::operator<< (const std::string& output)
{
  buf << output;
  return *this;
}

template<>
View::ViewErrorStream&
View::ViewErrorStream::operator<< (const size_t& output)
{
  buf << output;
  return *this;
}

template<>
View::ViewErrorStream&
View::ViewErrorStream::operator<< (const unsigned int& output)
{
  buf << output;
  return *this;
}

View::ViewErrorStream&
View::ViewErrorStream::operator<< (const char* output)
{
  buf << output;
  return *this;
}

void
View::ViewOutputStream::showContentsOfStream ()
{
  std::cout << buf.str () << std::endl;
  buf.str ("");
  buf.clear ();
}

void
View::ViewErrorStream::showContentsOfStream ()
{
  std::cerr << RED_ESC << buf.str () << DEFAULT_COLOR_ESC << std::endl;
  buf.str ("");
  buf.clear ();
}

View::ViewOutputStream&
View::show (View::ViewOutputStream& vos)
{
  vos.showContentsOfStream ();
  return vos;
}

View::ViewErrorStream&
View::show (View::ViewErrorStream& ves)
{
  ves.showContentsOfStream ();
  return ves;
}

View::View (Model* model)
    : in (this),
      out (),
      err (),
      m_model (model)
{
  const rect_t defaultViewPort { 0, 0, 25, 25 };
  viewport = defaultViewPort;
  if (viewport.x2 >= model->settings ().width)
    viewport.x2 = 0;
  if (viewport.y2 >= model->settings ().height)
    viewport.y2 = 0;
  out << BOLD_ESC << "Game of Life:" << DEFAULT_STYLE_ESC << show;
}

void
View::update ()
{
  if (viewport.x1 > 0 || viewport.y1 > 0 ||
      viewport.x2 < m_model->settings ().width ||
      viewport.y2 < m_model->settings ().height)
    std::cout << viewport << ":\n";
  printWorld (m_model->world ());
}

void
View::help ()
{
  out << BOLD_ESC << "Help:\n" << DEFAULT_STYLE_ESC << show;
  printHelp ();
}

// perform a CLI specific command, or returns false if the command wasn't unique
// to the CLI
bool
View::performImpCommand (const Command& cmd)
{
  switch (cmd.action)
  {
    case ALWAYS:
    {
      std::cout << BLACK_ESC << "always:" << cmd.args << DEFAULT_COLOR_ESC
                << std::endl;
      alwaysDo = parseCommands (cmd.args);
      // pop the other commands from alwaysDo
      // TODO: this is sloppy
      while (!commands.empty ())
        commands.pop ();
      return true;
    }
    case PRINT:
    {
      printModelComponents (cmd.args, *(m_model));
      return true;
    }
    case VIEW:
    {
      // TODO: specify x, y, and size instead of a rect
      std::istringstream argStream (cmd.args);
      bool isValidRect = true;
      std::string nextWord;
      if (argStream >> nextWord)
      {
        rect_t r;
        char* begin = nextWord.data ();
        char* end = nextWord.data () + nextWord.size ();
        auto [ptr, ec] = std::from_chars (begin, end, r.x1);
        if (ec == std::errc::result_out_of_range)
        {
          std::cout << RED_ESC << nextWord << " is too large."
                    << DEFAULT_COLOR_ESC << std::endl;
          isValidRect = false;
        }
        else if (ptr != end || ec != std::errc ())
        {
          std::cout << RED_ESC << nextWord << " isn't a number."
                    << DEFAULT_COLOR_ESC << std::endl;
          isValidRect = false;
        }
        if (argStream >> nextWord)
        {
          char* begin = nextWord.data ();
          char* end = nextWord.data () + nextWord.size ();
          auto [ptr, ec] = std::from_chars (begin, end, r.y1);
          if (ec == std::errc::result_out_of_range)
          {
            std::cout << RED_ESC << nextWord << " is too large."
                      << DEFAULT_COLOR_ESC << std::endl;
            isValidRect = false;
          }
          else if (ptr != end || ec != std::errc ())
          {
            std::cout << RED_ESC << nextWord << " isn't a number."
                      << DEFAULT_COLOR_ESC << std::endl;
            isValidRect = false;
          }
          if (argStream >> nextWord)
          {
            char* begin = nextWord.data ();
            char* end = nextWord.data () + nextWord.size ();
            auto [ptr, ec] = std::from_chars (begin, end, r.x2);
            if (ec == std::errc::result_out_of_range)
            {
              std::cout << RED_ESC << nextWord << " is too large."
                        << DEFAULT_COLOR_ESC << std::endl;
              isValidRect = false;
            }
            else if (ptr != end || ec != std::errc ())
            {
              std::cout << RED_ESC << nextWord << " isn't a number."
                        << DEFAULT_COLOR_ESC << std::endl;
              isValidRect = false;
            }
            if (argStream >> nextWord)
            {
              char* begin = nextWord.data ();
              char* end = nextWord.data () + nextWord.size ();
              auto [ptr, ec] = std::from_chars (begin, end, r.y2);
              if (ec == std::errc::result_out_of_range)
              {
                std::cout << RED_ESC << nextWord << " is too large."
                          << DEFAULT_COLOR_ESC << std::endl;
                isValidRect = false;
              }
              else if (ptr != end || ec != std::errc ())
              {
                std::cout << RED_ESC << nextWord << " isn't a number."
                          << DEFAULT_COLOR_ESC << std::endl;
                isValidRect = false;
              }
              if (isValidRect)
              {
                if (m_model->isInsideWorld (r))
                {
                  if (r.isWellFormed ())
                  {
                    viewport = r;
                    printWorld (m_model->world ());
                  }
                  else
                  {
                    std::cout << RED_ESC << "The points "
                              << point_t { r.x1, r.y1 } << " and "
                              << point_t { r.x2, r.y2 }
                              << " do not form the lower left and upper right "
                                 "corners of a rectangle."
                              << DEFAULT_COLOR_ESC << std::endl;
                  }
                }
                else
                {
                  std::cout << RED_ESC << r
                            << " extends outside the boundaries of the world."
                            << DEFAULT_COLOR_ESC << std::endl;
                }
              }
            }
            else
            {
              std::cout << RED_ESC
                        << "You need to tell me the y coordinate of the upper "
                           "right corner of the new viewport."
                        << DEFAULT_COLOR_ESC << std::endl;
            }
          }
          else
          {
            std::cout << RED_ESC
                      << "You need to tell me the x and y coordinates of the "
                         "upper right corner of the new viewport."
                      << DEFAULT_COLOR_ESC << std::endl;
          }
        }
        else
        {
          std::cout << RED_ESC
                    << "You need to tell me the y coordinate of the lower left "
                       "corner of the new viewport."
                    << DEFAULT_COLOR_ESC << std::endl;
        }
      }
      else
      {
        std::cout
          << RED_ESC
          << "You need to tell me the x and y coordinates of the lower left "
             "corner and the upper right corner of the new viewport."
          << DEFAULT_COLOR_ESC << std::endl;
      }
      return true;
    }
    case CLEAR:
    {
      std::cout << CLEAR_ESC << std::endl;
      return true;
    }
    case CONTINUE:
    {
      shouldIContinue = true;
      return true;
    }
  }
  return false;
}

/* Display helpers */

void
printSquare (square_t sqr)
{
  unsigned short sedimentR = SUBSTANCE_VAL_TO_RGB (sqr.sediment.a);
  unsigned short sedimentG = SUBSTANCE_VAL_TO_RGB (sqr.sediment.b);
  unsigned short sedimentB = SUBSTANCE_VAL_TO_RGB (sqr.sediment.c);
  unsigned short flowColor = MOTION_TO_RGB (sqr.flow);

  std::cout << BG_COLOR_ESC (sedimentR, sedimentG, sedimentB)
            << (sqr.cell ? COLOR_ESC (200, 200, 200) + "O"
                         : (COLOR_ESC (flowColor, flowColor, flowColor) +
                            (sqr.isFlowSrc ? "*" : MOTION_TO_STR (sqr.flow))))
            << DEFAULT_COLOR_ESC << DEFAULT_BG_COLOR_ESC;
}

// TODO: this is real messy fr
void
printWorld (const std::vector<std::vector<square_t>>& world)
{
  std::cout << "┌";
  bool firstCol = true;
  for (size_t x = viewport.x1; x != viewport.x2 || firstCol;
       x = addMod (x, 1, world[0].size ()), firstCol = false)
    std::cout << "─";
  std::cout << "┐\n";
  bool firstRow = true;
  for (size_t y = addMod (viewport.y2, -1, world.size ());
       y != addMod (viewport.y1, -1, world.size ()) || firstRow;
       y = addMod (y, -1, world.size ()), firstRow = false)
  {
    std::cout << "│";
    bool firstCol = true;
    for (size_t x = viewport.x1; x != viewport.x2 || firstCol;
         x = addMod (x, 1, world[0].size ()), firstCol = false)
      printSquare (world[y][x]);
    std::cout << "│\n";
  }
  std::cout << "└";
  firstCol = true;
  for (size_t x = viewport.x1; x != viewport.x2 || firstCol;
       x = addMod (x, 1, world[0].size ()), firstCol = false)
    std::cout << "─";
  std::cout << "┘" << std::endl;
}

void
printSettings (const settings_t& settings)
{
  std::cout << "Height: " << settings.height << "\nWidth: " << settings.width
            << "\nFlow Sources: " << settings.numFlowSrcs
            << "\nFlow Sinks: " << settings.numFlowSinks
            << "\nStart Cells: " << settings.numStartCells
            << "\nSeed: " << settings.seed << std::endl;
}

void
printHelp ()
{
  std::cout
    << "Enter commands in the format: <Action1> [<Action2>]...\n"
       "\n"
       "Actions:\n"
       "step, s        step the simulation forward 1 tick\n"
       "display, disp  print a 2D character representation of the world\n"
       "exit, quit     exit the program\n"
       "help           show this message\n"
       "view, v <X1 Y1 X2 Y2>  set the viewport to display the specified\n"
       "coordinates"
       "\n"
       "Special Commands:\n"
       "always [<Action>]           execute <Action> after every command. No\n"
       "                            <Action> clears the actions\n"
       "<Action> <Number>           repeat <Action> <Number> times\n"
       "print <Item1> [<Item2>]...  print items such as settings, squares,\n"
       "                            cells, etc."
    << std::endl;
}

void
printModelComponents (const std::string& components, const Model& model)
{
  std::istringstream argStream (components);
  for (std::string arg; argStream >> arg;)
  {
    if (arg == "world")
    {
      std::cout << BOLD_ESC << "World";
      if (viewport.x1 > 0 || viewport.y1 > 0 ||
          viewport.x2 < model.settings ().width ||
          viewport.y2 < model.settings ().height)
        std::cout << " " << viewport;
      std::cout << ":\n" << DEFAULT_STYLE_ESC;
      printWorld (model.world ());
    }
    else if (arg == "view" || arg == "viewport" || arg == "v" ||
             arg == "window")
    {
      std::cout << BOLD_ESC << "World " << viewport << ":\n"
                << DEFAULT_STYLE_ESC;
      printWorld (model.world ());
    }
    else if (arg == "settings" || arg == "setting")
    {
      std::cout << BOLD_ESC << "Settings:\n" << DEFAULT_STYLE_ESC;
      printSettings (model.settings ());
    }
    else if (arg == "always")
    {
      std::cout << BOLD_ESC << "Always:\n" << DEFAULT_STYLE_ESC;
      if (alwaysDo.empty ())
        std::cout << "nothing";
      else
        for (Command c : alwaysDo)
          std::cout << c << " ";
      std::cout << std::endl;
    }
    else if (arg == "size" || arg == "s" || arg == "--size" || arg == "-s")
    {
      std::cout << BOLD_ESC << "Size:\n"
                << DEFAULT_STYLE_ESC << "Height: " << model.settings ().height
                << "\nWidth: " << model.settings ().width << std::endl;
    }
    else if (arg == "height")
    {
      std::cout << BOLD_ESC << "Height:\n"
                << DEFAULT_STYLE_ESC << model.settings ().height << std::endl;
    }
    else if (arg == "width")
    {
      std::cout << BOLD_ESC << "Width:\n"
                << DEFAULT_STYLE_ESC << model.settings ().width << std::endl;
    }
    else if (arg == "flow")
    {
      std::string nextWord;
      argStream >> nextWord;
      if (nextWord == "source" || nextWord == "sources" || nextWord == "src" ||
          nextWord == "srcs")
      {
        std::cout << BOLD_ESC << "Flow Sources:\n"
                  << DEFAULT_STYLE_ESC << model.settings ().numFlowSrcs
                  << std::endl;
      }
      else if (nextWord == "sink" || nextWord == "sinks")
      {
        std::cout << BOLD_ESC << "Flow Sinks:\n"
                  << DEFAULT_STYLE_ESC << model.settings ().numFlowSinks
                  << std::endl;
      }
      else
      {
        std::cout << RED_ESC << "I don't know how to print flow " << nextWord
                  << "." << DEFAULT_COLOR_ESC << std::endl;
      }
    }
    else if (arg == "flow-sources" || arg == "--flow-sources")
    {
      std::cout << BOLD_ESC << "Flow Sources:\n"
                << DEFAULT_STYLE_ESC << model.settings ().numFlowSrcs
                << std::endl;
    }
    else if (arg == "flow-sinks" || arg == "--flow-sinks")
    {
      std::cout << BOLD_ESC << "Flow Sinks:\n"
                << DEFAULT_STYLE_ESC << model.settings ().numFlowSinks
                << std::endl;
    }
    else if (arg == "start")
    {
      std::string nextWord;
      argStream >> nextWord;
      if (nextWord == "cell" || nextWord == "cells")
      {
        std::cout << BOLD_ESC << "Start Cells:\n"
                  << DEFAULT_STYLE_ESC << model.settings ().numStartCells
                  << std::endl;
      }
      else
      {
        std::cout << RED_ESC << "I don't know how to print start " << nextWord
                  << "." << DEFAULT_COLOR_ESC << std::endl;
      }
    }
    else if (arg == "start-cells" || arg == "C" || arg == "--start-cells" ||
             arg == "-C")
    {
      std::cout << BOLD_ESC << "Start Cells:\n"
                << DEFAULT_STYLE_ESC << model.settings ().numStartCells
                << std::endl;
    }
    else if (arg == "seed" || arg == "--seed")
    {
      std::cout << BOLD_ESC << "Seed:\n"
                << DEFAULT_STYLE_ESC << model.settings ().seed << std::endl;
    }
    else if (arg == "square" || arg == "sqr")
    {
      bool isValidSquare = true;
      std::string nextWord;
      if (argStream >> nextWord)
      {
        point_t p;
        char* begin = nextWord.data ();
        char* end = nextWord.data () + nextWord.size ();
        auto [ptr, ec] = std::from_chars (begin, end, p.x);
        if (ec == std::errc::result_out_of_range)
        {
          std::cout << RED_ESC << nextWord << " is too large."
                    << DEFAULT_COLOR_ESC << std::endl;
          isValidSquare = false;
        }
        else if (ptr != end || ec != std::errc ())
        {
          std::cout << RED_ESC << nextWord << " isn't a number."
                    << DEFAULT_COLOR_ESC << std::endl;
          isValidSquare = false;
        }
        if (argStream >> nextWord)
        {
          char* begin = nextWord.data ();
          char* end = nextWord.data () + nextWord.size ();
          auto [ptr, ec] = std::from_chars (begin, end, p.y);
          if (ec == std::errc::result_out_of_range)
          {
            std::cout << RED_ESC << nextWord << " is too large."
                      << DEFAULT_COLOR_ESC << std::endl;
            isValidSquare = false;
          }
          else if (ptr != end || ec != std::errc ())
          {
            std::cout << RED_ESC << nextWord << " isn't a number."
                      << DEFAULT_COLOR_ESC << std::endl;
            isValidSquare = false;
          }
          if (isValidSquare)
          {
            if (model.isInsideWorld (p))
            {
              square_t sqr = model.world ()[p.y][p.x];
              std::cout << BOLD_ESC << "Square " << p << ":\n"
                        << DEFAULT_STYLE_ESC << "┌─┐\n│";
              printSquare (sqr);
              std::cout << "│\n└─┘\nFlow: ";
              if (sqr.isFlowSrc)
                std::cout << "--\n";
              else
                std::cout << sqr.flow << "\n";
              std::cout << "Sediment:\n"
                        << "  A: " << sqr.sediment.a
                        << "\n  B: " << sqr.sediment.b
                        << "\n  C: " << sqr.sediment.c << "\nCell: ";
              if (sqr.cell == nullptr)
                std::cout << "--";
              else
                std::cout << "yes";
              std::cout << std::endl;
            }
            else
            {
              std::cout << RED_ESC << p
                        << " is outside the boundaries of the world."
                        << DEFAULT_COLOR_ESC << std::endl;
            }
          }
        }
        else
        {
          std::cout << RED_ESC
                    << "You need to tell me the y coordinate of the "
                       "square to print."
                    << DEFAULT_COLOR_ESC << std::endl;
        }
      }
      else
      {
        std::cout << RED_ESC
                  << "You need to tell me the x and y coordinates of the "
                     "square to print."
                  << DEFAULT_COLOR_ESC << std::endl;
      }
    }
    else if (arg == "help")
    {
      std::cout << BOLD_ESC << "Help:\n" << DEFAULT_STYLE_ESC;
      printHelp ();
    }
    else
    {
      std::cout << RED_ESC << "I don't know how to print " << arg << "."
                << DEFAULT_COLOR_ESC << std::endl;
    }
  }
}

/* Parsing Helpers */

std::vector<Command>
parseCommands (const std::string& str)
{
  std::vector<Command> ret;

  std::istringstream inputStream (str);
  bool isFirstWord = true;
  for (std::string word; inputStream >> word;)
  {
    /* Parse repetition number */
    if (!isFirstWord && ret.back ().repeat == 1)
    {
      char* begin = word.data ();
      char* end = word.data () + word.size ();
      auto [ptr, ec] = std::from_chars (begin, end, ret.back ().repeat);
      if (ptr == end)
      {
        if (ec == std::errc::result_out_of_range)
        {
          std::cout << RED_ESC << word << " is too large of a number."
                    << DEFAULT_COLOR_ESC << std::endl;
          ret.pop_back ();
          continue;
        }
        else if (ec == std::errc ())
          continue;
      }
    }

    /* Parse action */
    auto it = actionTable.find (word);
    if (it != actionTable.end ())
    {
      Command cmd = { it->second };

      /* Parse args */
      switch (it->second)
      {
        // TODO: maybe make VIEW take only 4 args
        case ALWAYS:
        case PRINT:
        case VIEW:
          /* Unlimited args */
          std::getline (inputStream, cmd.args);
          ret.push_back (cmd);
          break;
        default:
          /* No args */
          if (!isFirstWord && cmd.action == ret.back ().action)
          {
            /* Duplicate commands, increase repeat instead */
            unsigned limit =
              std::numeric_limits<unsigned int>::max () - ret.back ().repeat;
            if (cmd.repeat <= limit)
            {
              ret.back ().repeat += cmd.repeat;
            }
            else
            {
              ret.back ().repeat = std::numeric_limits<unsigned int>::max ();
              cmd.repeat -= limit;
              ret.push_back (cmd);
            }
          }
          else
            ret.push_back (cmd);
      }
    }
    else
      ret.push_back ({ UNDEFINED });

    if (isFirstWord)
      isFirstWord = false;
  }
  return ret;
}