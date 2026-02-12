#include <cmath>
#include <cstdlib>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <string>
#include <tuple>

#include "life_view.h"
#include <charconv>

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
const int ALWAYS = -1, PRINT = -2;

// names for each action
std::map<std::string, int> const actionTable = {
  { "step", STEP },    { "s", STEP },          { "display", DISPLAY },
  { "disp", DISPLAY }, { "exit", EXIT },       { "quit", EXIT },
  { "help", HELP },    { "nothing", NOTHING }, { "always", ALWAYS },
  { "print", PRINT },  { "p", PRINT },         { "examine", PRINT },
  { "x", PRINT },      { "inspect", PRINT }
};

// canonical names for each action
std::map<int, std::string> const oneToOneActionTable = {
  { STEP, "step" },  { DISPLAY, "display" }, { EXIT, "quit" },
  { HELP, "help" },  { NOTHING, "nothing" }, { ALWAYS, "always" },
  { PRINT, "print" }
};

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

void
printWorld (const std::vector<std::vector<square_t>>& world)
{
  for (std::vector<square_t> row : world)
  {
    for (square_t sqr : row)
      printSquare (sqr);
    std::cout << "\n";
  }
  std::cout << std::flush;
}

void
printSettings (const settings_t& settings)
{
  std::cout << "Height: " << settings.height << "\nWidth: " << settings.width
            << "\nFlow Sources: " << settings.numFlowSrcs
            << "\nFlow Sinks: " << settings.numFlowSinks
            << "\nStart Cells: " << settings.numStartCells << std::endl;
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
      unsigned r = 0;
      auto [ptr, ec] = std::from_chars (begin, end, r);
      if (ptr == end)
      {
        if (ec == std::errc::result_out_of_range)
        {
          std::cout << RED_ESC << word << " is too large of a number."
                    << DEFAULT_COLOR_ESC << std::endl;
          continue;
        }
        else if (ec == std::errc ())
        {
          ret.back ().repeat = r;
          continue;
        }
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
        case ALWAYS:
        case PRINT:
          std::getline (inputStream, cmd.args);
          break;
      }

      ret.push_back (cmd);
    }
    else
      ret.push_back ({ UNDEFINED });

    if (isFirstWord)
      isFirstWord = false;
  }
  return ret;
}

/* Body */

std::string lastInput = "display";
std::queue<Command> commands {};
std::vector<Command> alwaysDo {};

View::ViewInputStream&
View::ViewInputStream::operator>> (Command& cmd)
{
  if (commands.empty ())
  {
    /* Parse more input from user */
    std::string input;
    std::getline (std::cin, input);

    if (input == "")
    {
      input = lastInput;
      std::cout << CURSOR_UP_ESC << BLACK_ESC << lastInput << DEFAULT_COLOR_ESC
                << std::endl;
    }

    for (Command c : parseCommands (input))
      commands.push (c);
    for (Command c : alwaysDo)
      commands.push (c);

    lastInput = input;
  }

  cmd = commands.front ();
  commands.pop ();

  switch (cmd.action)
  {
    case ALWAYS:
      std::cout << BLACK_ESC << "always:" << cmd.args << DEFAULT_COLOR_ESC
                << std::endl;
      alwaysDo = parseCommands (cmd.args);
      cmd = { NOTHING };
      break;
    case PRINT:
      std::istringstream argStream (cmd.args);
      for (std::string arg; argStream >> arg;)
      {
        if (arg == "world")
        {
          std::cout << BOLD_ESC << "World:\n" << DEFAULT_STYLE_ESC;
          printWorld (m_view->m_model->world ());
        }
        else if (arg == "settings")
        {
          std::cout << BOLD_ESC << "Settings:\n" << DEFAULT_STYLE_ESC;
          printSettings (m_view->m_model->settings ());
        }
        else if (arg == "always")
        {
          std::cout << BOLD_ESC << "Always:\n" << DEFAULT_STYLE_ESC;
          for (Command c : alwaysDo)
          {
            auto it = oneToOneActionTable.find (c.action);
            if (it != oneToOneActionTable.end ())
            {
              std::cout << it->second << (c.args == "" ? " " : c.args);
            }
            else
            {
              std::cout << "undefined ";
            }
          }
          std::cout << std::endl;
        }
        else if (arg == "size")
        {
          std::cout << BOLD_ESC << "Size:\n"
                    << DEFAULT_STYLE_ESC
                    << "Height: " << m_view->m_model->settings ().height
                    << "\nWidth: " << m_view->m_model->settings ().width
                    << std::endl;
        }
        else if (arg == "height")
        {
          std::cout << BOLD_ESC << "Height:\n"
                    << DEFAULT_STYLE_ESC << m_view->m_model->settings ().height
                    << std::endl;
        }
        else if (arg == "width")
        {
          std::cout << BOLD_ESC << "Width:\n"
                    << DEFAULT_STYLE_ESC << m_view->m_model->settings ().width
                    << std::endl;
        }
        else if (arg == "flow")
        {
          std::string nextWord;
          argStream >> nextWord;
          if (nextWord == "source" || nextWord == "sources")
          {
            std::cout << BOLD_ESC << "Flow Sources:\n"
                      << DEFAULT_STYLE_ESC
                      << m_view->m_model->settings ().numFlowSrcs << std::endl;
          }
          else if (nextWord == "sink" || nextWord == "sinks")
          {
            std::cout << BOLD_ESC << "Flow Sinks:\n"
                      << DEFAULT_STYLE_ESC
                      << m_view->m_model->settings ().numFlowSinks << std::endl;
          }
          else
          {
            std::cout << RED_ESC << "I don't know how to print flow "
                      << nextWord << "." << DEFAULT_COLOR_ESC << std::endl;
          }
        }
        else if (arg == "start")
        {
          std::string nextWord;
          argStream >> nextWord;
          if (nextWord == "cell" || nextWord == "cells")
          {
            std::cout << BOLD_ESC << "Start Cells:\n"
                      << DEFAULT_STYLE_ESC
                      << m_view->m_model->settings ().numStartCells
                      << std::endl;
          }
          else
          {
            std::cout << RED_ESC << "I don't know how to print start "
                      << nextWord << "." << DEFAULT_COLOR_ESC << std::endl;
          }
        }
        else if (arg == "seed")
        {
          std::cout << BOLD_ESC << "Seed:\n"
                    << DEFAULT_STYLE_ESC << m_view->m_model->settings ().seed
                    << std::endl;
        }
        // TODO
        // else if (arg == "square")
        // {
        //   bool isValidSquare = true;
        //   std::string nextWord;
        //   argStream >> nextWord;
        //   char* begin = nextWord.data ();
        //   char* end = nextWord.data () + nextWord.size ();
        //   size_t x = 0;
        //   {
        //     auto [ptr, ec] = std::from_chars (begin, end, x);
        //     if (ptr == end)
        //     {
        //       if (ec == std::errc::result_out_of_range)
        //       {
        //         isValidSquare = false;
        //         std::cout << RED_ESC << nextWord << " is too large of a
        //         number."
        //                   << DEFAULT_COLOR_ESC << std::endl;
        //       }
        //       else if (ec != std::errc () && )
        //       {
        //         ret.back ().repeat = r;
        //         continue;
        //       }
        //     }
        //   }
        // }
        else
        {
          std::cout << RED_ESC << "I don't know how to print " << arg << "."
                    << DEFAULT_COLOR_ESC << std::endl;
        }
      }
      cmd = { NOTHING };
      break;
  }
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
  out << BOLD_ESC << "Game of Life:" << DEFAULT_STYLE_ESC << show;
}

void
View::update ()
{
  printWorld (m_model->world ());
}

void
View::help ()
{
  out << BOLD_ESC << "Help:\n"
      << DEFAULT_STYLE_ESC
      << "Enter commands in the format: <Action1> [<Action2>]...\n"
      << "\nActions:\n"
      << "step, s        step the simulation forward 1 tick\n"
      << "display, disp  print a 2D character representation of the world\n"
      << "exit, quit     exit the program\n"
      << "help           show this message\n"
      << "\nSpecial Commands:\n"
      << "always [<Action>]  execute <Action> after every command. No <Action> "
         "resets the command\n"
      << "<Action> <Number>  repeats <Action> <Number> times\n"
      << View::show;
}
