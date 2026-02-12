#include <charconv>
#include <getopt.h>
#include <iostream>
#include <string.h>
#include <unistd.h>

// #include "life_cl_view.cpp"
#include "life_view.h"
#include <string>

#define PRINT_USAGE(EXEC_NAME)                                                 \
  do                                                                           \
  {                                                                            \
    std::cerr                                                                  \
      << "Usage:\n"                                                            \
      << "\u001b[1m" << std::string (EXEC_NAME) << "\u001b[22m"                \
      << " [-s LENGTH[, HEIGHT]]\n"                                            \
      << "  -s LENGTH[, HEIGHT]\n"                                             \
      << "    Sets both side lengths of the world to LENGTH (default 25). If " \
         "HEIGHT is provided, sets the height to HEIGHT instead.\n"            \
      << "  -C, --start-cells NUM\n"                                           \
      << "    Starts the simulation with NUM cells (default 100).\n"           \
      << "  --flow-sources NUM[, NUM]\n"                                       \
      << "    Creates NUM flow sources and sinks (default 1). If a second "    \
         "NUM is provided, creates that number of flow sinks istead.\n"        \
      << "  --flow-sinks NUM\n"                                                \
      << "    Creates NUM flow sinks (default 1). Overrides the first NUM in " \
         "--flow-sources, if provided.\n";                                     \
  } while (0)
#define EXIT_WITH_USAGE_ERROR(OPT, ERR_MSG)                                    \
  do                                                                           \
  {                                                                            \
    std::cerr << "\u001b[31m" << (OPT) << " " << (ERR_MSG) << "\u001b[39m"     \
              << std::endl;                                                    \
    PRINT_USAGE (argv[0]);                                                     \
    exit (EXIT_FAILURE);                                                       \
  } while (0)
#define ARG_ERR(ERR_NUM)                                                       \
  ((ERR_NUM) == 0   ? "has too many arguments."                                \
   : (ERR_NUM) == 1 ? "has too few arguments."                                 \
   : (ERR_NUM) == 2 ? "should have numeric arguments."                         \
   : (ERR_NUM) == 3 ? "has a numeric argument that is too large"               \
                    : "UNDEFINED ERROR")
#define OPT_ERR(ERR_NUM)                                                       \
  ((ERR_NUM) == 0 ? "isn't a recognized command-line option."                  \
                  : "UNDEFINED ERROR")
#define NAME_OF_LONG_OPT(LONG_OPT_NUM)                                         \
  ((LONG_OPT_NUM) == 1   ? "flow-sources"                                      \
   : (LONG_OPT_NUM) == 2 ? "flow-sinks"                                        \
                         : "UNKNOWN OPTION")

settings_t
parseSettings (int argc, char* argv[]);

int
main (int argc, char* argv[])
{
  // debug
  //  int newArgc = 4;
  //  char* newArgv0 = argv[0];
  //  char* newArgv1 = new char[strlen("-s") + 1];
  //  strcpy(newArgv1, "-s");
  //  char* newArgv2 = new char[strlen("10") + 1];
  //  strcpy(newArgv2, "10");
  //  char* newArgv3 = new char[strlen("10") + 1];
  //  strcpy(newArgv3, "10");
  //  char* newArgv4 = NULL;
  //  char* newArgv[newArgc + 1] = {newArgv0, newArgv1, newArgv2, newArgv3,
  //  newArgv4}; for (int i = 0; i < newArgc; ++i) {
  //      std::cout << newArgv[i] << std::endl;
  //  }
  //  for (int i = 0; i < argc; ++i) {
  //      std::cout << argv[i] << std::endl;
  //  }

  settings_t settings = parseSettings (argc, argv);
  // settings_t settings = parseSettings(newArgc, newArgv);
  Model model (settings);
  View view (&model);

  view.update ();

  bool exit = false;
  while (!exit)
  {
    Command cmd;
    view.in >> cmd;

    for (unsigned i = 0; i < cmd.repeat; ++i)
    {
      switch (cmd.action)
      {
        case STEP:
          model.step ();
          break;
        case DISPLAY:
          view.update ();
          break;
        case EXIT:
          exit = true;
          break;
        case HELP:
          view.help ();
          break;
        case NOTHING:
          break;
        default:
          view.err << "I don't know that command." << View::show;
      }
    }
  }
}

settings_t
parseSettings (int argc, char* argv[])
{
  settings_t settings;

  // state
  int opt = '\0';
  int longOpt = 0;
  char prevOpt = '\0';
  int numArgsPrevOpt = 0;

  bool flowSinksSet = false;

  option longOptions[] = { option { "size", 1, NULL, 's' },
                           option { "flow-sources", 1, &longOpt, 1 },
                           option { "flow-sinks", 1, &longOpt, 2 },
                           option { "start-cells", 1, NULL, 'C' },
                           option { "seed", 1, &longOpt, 3 },
                           option { 0, 0, 0, 0 } };
  while (true)
  {
    switch (opt = getopt_long (argc, argv, "-:s:C:", longOptions, NULL))
    {
      case 's':
      {
        prevOpt = 's';
        longOpt = 0;
        numArgsPrevOpt = 0;

        char* optArgCpy = strdup (optarg);
        char* tok = strtok (optArgCpy, ", \t\n");
        if (!tok)
          EXIT_WITH_USAGE_ERROR ('s', ARG_ERR (2));

        // 1st arg
        {
          ++numArgsPrevOpt;
          char* begin = tok;
          char* end = tok + strlen (tok);

          auto [ptr, ec] = std::from_chars (begin, end, settings.width);
          if (ec == std::errc::result_out_of_range)
            EXIT_WITH_USAGE_ERROR ('s', ARG_ERR (3));
          else if (ptr != end || ec != std::errc ())
            EXIT_WITH_USAGE_ERROR ('s', ARG_ERR (2));
          settings.height = settings.width;

          tok = strtok (NULL, ", \t\n");
          if (!tok)
            break;
        }

        // 2nd arg
        {
          ++numArgsPrevOpt;
          char* begin = tok;
          char* end = tok + strlen (tok);

          auto [ptr, ec] = std::from_chars (begin, end, settings.height);
          if (ec == std::errc::result_out_of_range)
            EXIT_WITH_USAGE_ERROR ('s', ARG_ERR (3));
          else if (ptr != end || ec != std::errc ())
            EXIT_WITH_USAGE_ERROR ('s', ARG_ERR (2));

          tok = strtok (NULL, " \t\n");
          if (tok)
            EXIT_WITH_USAGE_ERROR ('s', ARG_ERR (0));

          free (optArgCpy);
          break;
        }
      }
      case 'C':
      {
        prevOpt = 'C';
        longOpt = 0;
        numArgsPrevOpt = 1;
        const char* begin = optarg;
        const char* end = optarg + strlen (optarg);

        auto [ptr, ec] = std::from_chars (begin, end, settings.numStartCells);
        if (ec == std::errc::result_out_of_range)
          EXIT_WITH_USAGE_ERROR ('C', ARG_ERR (3));
        else if (ptr != end || ec != std::errc ())
          EXIT_WITH_USAGE_ERROR ('C', ARG_ERR (2));

        break;
      }
      case 1:
      {
        // 2nd or 3rd arg for an opt, or arg without an opt
        switch (prevOpt)
        {
          case 's':
          {
            if (numArgsPrevOpt > 1)
              EXIT_WITH_USAGE_ERROR ('s', ARG_ERR (0));

            // 2nd arg
            ++numArgsPrevOpt;
            const char* begin = optarg;
            const char* end = optarg + strlen (optarg);

            auto [ptr, ec] = std::from_chars (begin, end, settings.height);
            if (ec == std::errc::result_out_of_range)
              EXIT_WITH_USAGE_ERROR ('s', ARG_ERR (3));
            else if (ptr != end || ec != std::errc ())
              EXIT_WITH_USAGE_ERROR ('s', ARG_ERR (2));

            break;
          }
          case '\0':
          {
            // either long opt or arg without an opt
            switch (longOpt)
            {
              case 1:
              {
                // --flow-sources
                if (numArgsPrevOpt > 1)
                  EXIT_WITH_USAGE_ERROR ("flow-sources", ARG_ERR (0));

                ++numArgsPrevOpt;
                const char* begin = optarg;
                const char* end = optarg + strlen (optarg);

                auto [ptr, ec] =
                  std::from_chars (begin, end, settings.numFlowSinks);
                if (ec == std::errc::result_out_of_range)
                  EXIT_WITH_USAGE_ERROR ("flow-sources", ARG_ERR (3));
                else if (ptr != end || ec != std::errc ())
                  EXIT_WITH_USAGE_ERROR ("flow-sources", ARG_ERR (2));

                break;
              }
              case 0:
              {
                // arg without an opt
                // treat it like an option that was misspelled
                EXIT_WITH_USAGE_ERROR ("an option", OPT_ERR (0));
              }
              default:
              {
                // invalid 2nd arg
                EXIT_WITH_USAGE_ERROR (NAME_OF_LONG_OPT (longOpt), ARG_ERR (0));
              }
            }
            break;
          }
          default:
          {
            // invalid 2nd arg
            EXIT_WITH_USAGE_ERROR ((char) prevOpt, ARG_ERR (0));
          }
        }
        break;
      }
      case 0:
      {
        // long opt
        prevOpt = '\0';
        numArgsPrevOpt = 0;
        switch (longOpt)
        {
          case 1:
          {
            // --flow-sources
            char* optArgCpy = strdup (optarg);
            char* tok = strtok (optArgCpy, ", \t\n");
            if (!tok)
              EXIT_WITH_USAGE_ERROR ("flow-sources", ARG_ERR (2));

            // 1st arg
            {
              ++numArgsPrevOpt;
              char* begin = tok;
              char* end = tok + strlen (tok);

              auto [ptr, ec] =
                std::from_chars (begin, end, settings.numFlowSrcs);
              if (ec == std::errc::result_out_of_range)
                EXIT_WITH_USAGE_ERROR ("flow-sources", ARG_ERR (3));
              else if (ptr != end || ec != std::errc ())
                EXIT_WITH_USAGE_ERROR ("flow-sources", ARG_ERR (2));
              // flow-sinks overrides the 1st arg of flow-sources
              if (!flowSinksSet)
                settings.numFlowSinks = settings.numFlowSrcs;

              tok = strtok (NULL, ", \t\n");
              if (!tok)
                break;
            }

            // 2nd arg
            {
              ++numArgsPrevOpt;
              char* begin = tok;
              char* end = tok + strlen (tok);

              auto [ptr2, ec2] =
                std::from_chars (begin, end, settings.numFlowSinks);
              if (ec2 == std::errc::result_out_of_range)
                EXIT_WITH_USAGE_ERROR ("flow-sources", ARG_ERR (3));
              else if (ptr2 != end || ec2 != std::errc ())
                EXIT_WITH_USAGE_ERROR ("flow-sources", ARG_ERR (2));

              tok = strtok (NULL, " \t\n");
              if (tok)
                EXIT_WITH_USAGE_ERROR ("flow-sources", ARG_ERR (0));

              free (optArgCpy);
              break;
            }
          }
          case 2:
          {
            // --flow-sinks
            flowSinksSet = true;

            char* begin = optarg;
            char* end = optarg + strlen (optarg);

            auto [ptr, ec] =
              std::from_chars (begin, end, settings.numFlowSinks);
            if (ec == std::errc::result_out_of_range)
              EXIT_WITH_USAGE_ERROR ("seed", ARG_ERR (3));
            else if (ptr != end || ec != std::errc ())
              EXIT_WITH_USAGE_ERROR ("seed", ARG_ERR (2));
            break;
          }
          case 3:
          {
            // --seed
            char* begin = optarg;
            char* end = optarg + strlen (optarg);

            auto [ptr, ec] = std::from_chars (begin, end, settings.seed);
            if (ec == std::errc::result_out_of_range)
              EXIT_WITH_USAGE_ERROR ("seed", ARG_ERR (3));
            else if (ptr != end || ec != std::errc ())
              EXIT_WITH_USAGE_ERROR ("seed", ARG_ERR (2));
          }
        }
        break;
      }
      case -1:
      {
        // no more args
        return settings;
      }
      case ':':
      {
        // missing arg
        if (optopt)
          EXIT_WITH_USAGE_ERROR ((char) optopt, ARG_ERR (1));
        else // long opt
          EXIT_WITH_USAGE_ERROR ("an option", ARG_ERR (1));
        break;
      }
      case '?':
      {
        // unknown opt
        if (optopt)
          EXIT_WITH_USAGE_ERROR ((char) optopt, OPT_ERR (0));
        else
          EXIT_WITH_USAGE_ERROR ("an option", OPT_ERR (0));
      }
    }
  }
}