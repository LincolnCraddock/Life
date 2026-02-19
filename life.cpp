#include <charconv>
#include <getopt.h>
#include <iostream>
#include <string.h>
#include <string>
#include <unistd.h>

#include "life_model.h"
#include "life_view.h"

#define PRINT_USAGE(EXEC_NAME)                                                 \
  do                                                                           \
  {                                                                            \
    std::cerr                                                                  \
      << "Usage: " << std::string (EXEC_NAME) << " [OPTION]...\n"              \
      << "  -C, --start-cells NUM      Starts the simulation with NUM cells\n" \
      << "                               (default 100).\n"                     \
      << "  --flow-sinks NUM           Creates NUM flow sinks (default 1).\n"  \
      << "  --flow-sources NUM[, NUM]  Creates NUM flow sources and sinks\n"   \
      << "                               (default 1). If a second NUM is\n"    \
      << "                               provided, creates that number of\n"   \
      << "                               flow sinks istead.\n"                 \
      << "  -s --size LENGTH[, HEIGHT] Sets both side lengths of the world\n"  \
      << "                               to LENGTH (default 25). If HEIGHT\n"  \
      << "                               is provided, sets the height to\n"    \
      << "                               HEIGHT instead.\n"                    \
      << "  --seed NUM                 Uses NUM as the seed of the random\n"   \
      << "                               number generator.\n";                 \
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
   : (ERR_NUM) == 2 ? "should have positive numeric arguments."                \
   : (ERR_NUM) == 3 ? "has a numeric argument that is too large"               \
   : (ERR_NUM) == 4 ? "should have positive, non-zero numeric arguments."      \
                    : "UNDEFINED ERROR")
#define OPT_ERR(ERR_NUM)                                                       \
  ((ERR_NUM) == 0 ? "isn't a recognized command-line option."                  \
                  : "UNDEFINED ERROR")
#define NAME_OF_LONG_OPT(LONG_OPT_NUM)                                         \
  ((LONG_OPT_NUM) == 1   ? "flow-sources"                                      \
   : (LONG_OPT_NUM) == 2 ? "flow-sinks"                                        \
   : (LONG_OPT_NUM) == 3 ? "seed"                                              \
                         : "UNKNOWN OPTION")

// parse the command line args into settings
settings_t
parseSettings (int argc, char* argv[]);

int
main (int argc, char* argv[])
{
  settings_t settings = parseSettings (argc, argv);
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

  /* State */
  int opt = '\0';
  int longOpt = 0;
  char prevOpt = '\0';
  int numArgsPrevOpt = 0;

  bool flowSinksSet = false;

  /* Parse using getopt_long */
  option longOptions[] = { option { "help", 0, NULL, 'h' },
                           option { "size", 1, NULL, 's' },
                           option { "flow-sources", 1, &longOpt, 1 },
                           option { "flow-sinks", 1, &longOpt, 2 },
                           option { "start-cells", 1, NULL, 'C' },
                           option { "seed", 1, &longOpt, 3 },
                           option { 0, 0, 0, 0 } };
  const int numLongOptions = 6;

  while (true)
  {
    switch (opt = getopt_long (argc, argv, "-:s:C:h", longOptions, NULL))
    {
      case 'h':
      {
        PRINT_USAGE (argv[0]);
        exit (EXIT_SUCCESS);
      }
      case 's':
      {
        prevOpt = 's';
        longOpt = 0;
        numArgsPrevOpt = 0;

        char* optArgCpy = strdup (optarg);
        char* tok = strtok (optArgCpy, ", \t\n");
        if (!tok)
          EXIT_WITH_USAGE_ERROR ('s', ARG_ERR (2));

        /* 1st arg */
        {
          ++numArgsPrevOpt;
          char* begin = tok;
          char* end = tok + strlen (tok);

          auto [ptr, ec] = std::from_chars (begin, end, settings.width);
          if (ec == std::errc::result_out_of_range)
            EXIT_WITH_USAGE_ERROR ('s', ARG_ERR (3));
          else if (ptr != end || ec != std::errc () || settings.width == 0)
            EXIT_WITH_USAGE_ERROR ('s', ARG_ERR (4));
          settings.height = settings.width;

          tok = strtok (NULL, ", \t\n");
          if (!tok)
            break;
        }

        /* 2nd arg */
        {
          ++numArgsPrevOpt;
          char* begin = tok;
          char* end = tok + strlen (tok);

          auto [ptr, ec] = std::from_chars (begin, end, settings.height);
          if (ec == std::errc::result_out_of_range)
            EXIT_WITH_USAGE_ERROR ('s', ARG_ERR (3));
          else if (ptr != end || ec != std::errc () || settings.height == 0)
            EXIT_WITH_USAGE_ERROR ('s', ARG_ERR (4));

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
        /* 2nd or 3rd arg for an opt, or arg without an opt */
        switch (prevOpt)
        {
          case 's':
          {
            if (numArgsPrevOpt > 1)
              EXIT_WITH_USAGE_ERROR ('s', ARG_ERR (0));

            /* 2nd arg */
            ++numArgsPrevOpt;
            const char* begin = optarg;
            const char* end = optarg + strlen (optarg);

            auto [ptr, ec] = std::from_chars (begin, end, settings.height);
            if (ec == std::errc::result_out_of_range)
              EXIT_WITH_USAGE_ERROR ('s', ARG_ERR (3));
            else if (ptr != end || ec != std::errc () || settings.height == 0)
              EXIT_WITH_USAGE_ERROR ('s', ARG_ERR (4));

            break;
          }
          case '\0':
          {
            /* Either arg for long opt arg without an opt */
            switch (longOpt)
            {
              case 1:
              {
                /* --flow-sources */
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
                /* Arg without an opt */
                // treat it like an option that was misspelled
                EXIT_WITH_USAGE_ERROR (argv[optind - 1], OPT_ERR (0));
              }
              default:
              {
                /* Invalid 2nd arg */
                EXIT_WITH_USAGE_ERROR (NAME_OF_LONG_OPT (longOpt), ARG_ERR (0));
              }
            }
            break;
          }
          default:
          {
            /* Invalid 2nd arg */
            EXIT_WITH_USAGE_ERROR ((char) prevOpt, ARG_ERR (0));
          }
        }
        break;
      }
      case 0:
      {
        /* Long opt */
        prevOpt = '\0';
        numArgsPrevOpt = 0;
        switch (longOpt)
        {
          case 1:
          {
            /* --flow-sources */
            char* optArgCpy = strdup (optarg);
            char* tok = strtok (optArgCpy, ", \t\n");
            if (!tok)
              EXIT_WITH_USAGE_ERROR ("flow-sources", ARG_ERR (2));

            /* 1st arg */
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

            /* 2nd arg */
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
            /* --flow-sinks */
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
            /* --seed */
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
        /* No more opts or args */
        return settings;
      }
      case ':':
      {
        /* Missing arg */
        std::string unknownOpt = argv[optind - 1];
        if (unknownOpt[0] == '-' && unknownOpt[1] == '-')
        {
          std::string name = unknownOpt.substr (2);
          for (int i = 0; i < numLongOptions; ++i)
          {
            option o = longOptions[i];
            if (name == o.name && o.flag == NULL)
              EXIT_WITH_USAGE_ERROR ((char) o.val, ARG_ERR (1));
          }

          EXIT_WITH_USAGE_ERROR (name, ARG_ERR (1));
        }
        else
          EXIT_WITH_USAGE_ERROR ((char) optopt, ARG_ERR (1));
        break;
      }
      case '?':
      {
        /* Unknown opt */
        std::string unknownOpt = argv[optind - 1];
        if (unknownOpt[0] == '-' && unknownOpt[1] == '-')
        {
          size_t i = unknownOpt.find ('=');
          std::string name =
            unknownOpt.substr (2, std::min (i, unknownOpt.size ()) - 2);
          for (int i = 0; i < numLongOptions; ++i)
          {
            option o = longOptions[i];
            if (name == o.name)
            {
              if (o.flag == NULL)
              {
                if (optopt != 'h')
                  EXIT_WITH_USAGE_ERROR ((char) o.val, ARG_ERR (0));
                else
                {
                  PRINT_USAGE (argv[0]);
                  exit (EXIT_SUCCESS);
                }
              }
              else
                EXIT_WITH_USAGE_ERROR (name, ARG_ERR (0));
            }
          }

          EXIT_WITH_USAGE_ERROR (name, OPT_ERR (0));
        }
        else
          EXIT_WITH_USAGE_ERROR ((char) optopt, OPT_ERR (0));
      }
    }
  }
}