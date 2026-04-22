#include <charconv>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <optional>
#include <string.h>
#include <string>
#include <unistd.h>

#include "life_model.h"
#include "life_view.h"
#include "styml.h"
#include "util.h"

// clang-format off
/*
  default                                    looks for config at ./ and uses it if available before applying further args
  config                                     uses the config at ./ before applying further args, erroring if the config isn't there
  config=<path>                              uses the config at <path> before applying further args, erroring if the config isn't there
  no-config                                  doesn't use any config, only args
  gen-config                                 doesn't use any config, only args, and generates a new config with the provided args at ./
  gen-config=<path>                          doesn't use any config, only args, and generates a new config with the proved args at <path>

  config no-config                           errors
  config gen-config                          uses the config at ./ before applying further args, erroring if the config isn't there, and generates a new config at ./ using the config and the provided args
  config gen-config=<path>                   uses the config at ./ before applying further args, erroring if the config isn't there, and generates a new config at <path> using the config and the provided opts
  config=<path> no-config                    errors
  config=<path> gen-config                   uses the config at <path> before applying further args, erroring if the config isn't there, and generates a new config at ./ using the config and the provided opts
  config=<path1> gen-config=<path2>          uses the config at <path1> before applying further args, erroring if the config isn't there, and generates a new config at <path2> using the config and the provided opts
  no-config gen-config                       doesn't use any config, only args, and generates a new config with the provided args at ./
  no-config gen-config=<path>                doesn't use any config, only args, and generates a new config with the proved args at <path>

  config no-config gen-config                errors
  config no-config gen-config=<path>         errors
  config=<path> no-config gen-config         errors
  config=<path> no-config gen-config=<path>  errors
*/
// clang-format on

#define EXIT_WITH_ERROR(ERR_MSG)                                               \
  do                                                                           \
  {                                                                            \
    std::cerr << "\u001b[31m" << (ERR_MSG) << "\u001b[39m" << std::endl;       \
    exit (EXIT_FAILURE);                                                       \
  } while (0)
#define PRINT_USAGE(EXEC_NAME)                                                 \
  do                                                                           \
  {                                                                            \
    std::cerr                                                                  \
      << "Usage: " << std::string (EXEC_NAME) << " [OPTION]...\n"              \
      << BOLD_ESC << "  -C, --start-cells NUM\n"                               \
      << DEFAULT_STYLE_ESC                                                     \
      << "         Starts the simulation with NUM cells (default 100).\n"      \
      << BOLD_ESC << "      --config [PATH]\n"                                 \
      << DEFAULT_STYLE_ESC                                                     \
      << "         Requires there to be a valid config file at PATH\n"         \
         "          (default .life.yaml).\n"                                   \
      << BOLD_ESC << "      --flow-sinks NUM\n"                                \
      << DEFAULT_STYLE_ESC << "         Creates NUM flow sinks (default 1).\n" \
      << BOLD_ESC << "      --flow-sources NUM[, NUM]\n"                       \
      << DEFAULT_STYLE_ESC                                                     \
      << "         Creates NUM flow sources and sinks (default 1). If a\n"     \
         "          second NUM is provided, creates that many flow sinks\n"    \
         "          instead.\n"                                                \
      << BOLD_ESC << "      --gen-config [PATH]\n"                             \
      << DEFAULT_STYLE_ESC                                                     \
      << "         Generates new config file at PATH (default .life.yaml)\n"   \
         "          using the supplied command line arguments.\n"              \
      << BOLD_ESC << "      --no-config\n"                                     \
      << DEFAULT_STYLE_ESC                                                     \
      << "         Ignores the config file at .life.yaml.\n"                   \
      << BOLD_ESC << "  -s, --size LENGTH[, HEIGHT]\n"                         \
      << DEFAULT_STYLE_ESC                                                     \
      << "         Sets both side lengths of the world to LENGTH (default\n"   \
         "          25). If HEIGHT is provided, sets the height to HEIGHT\n"   \
         "          instead.\n"                                                \
      << BOLD_ESC << "      --seed NUM\n"                                      \
      << DEFAULT_STYLE_ESC                                                     \
      << "         Uses NUM as the seed of the random number generator.\n";    \
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
   : (ERR_NUM) == 5 ? "has an invalid file path as an argument."               \
                    : "UNDEFINED ERROR")
#define OPT_ERR(ERR_NUM)                                                       \
  ((ERR_NUM) == 0   ? "isn't a recognized command-line option."                \
   : (ERR_NUM) == 1 ? "and its 'no-' version can't both be specified."         \
   : (ERR_NUM) == 2 ? "was specified, but no config file named " +             \
                        DEFAULT_PATH_TO_CONFIG + " could be found."            \
                    : "UNDEFINED ERROR")
#define NAME_OF_LONG_OPT(LONG_OPT_NUM)                                         \
  ((LONG_OPT_NUM) == 1   ? "flow-sources"                                      \
   : (LONG_OPT_NUM) == 2 ? "flow-sinks"                                        \
   : (LONG_OPT_NUM) == 3 ? "seed"                                              \
   : (LONG_OPT_NUM) == 4 ? "config"                                            \
   : (LONG_OPT_NUM) == 5 ? "no-config"                                         \
   : (LONG_OPT_NUM) == 6 ? "gen-config"                                        \
                         : "UNKNOWN OPTION")

const std::string DEFAULT_PATH_TO_CONFIG = ".life.yaml";
const std::string CONFIG_VERSION = "1.0.0";

// parse the path of the config files, or display help information and exit if
// the -h opt is supplied
//
// returns a pair containing the a path to the config file, if provided, and a
// path to the config file to be generated, if provided
//
// prints an error and exits if both the --no-config opt and --config opts are
// supplied
std::pair<std::optional<std::string>, std::optional<std::string>>
parseConfigPathsFromArgs (int argc, char* argv[]);

// parse a config file into settings
//
// returns true if the config file could be found
//
// prints an error and exits if the config is in an invalid format
bool
parseSettingsFromConfig (const std::string& pathToFile, settings_t& settings);

// parse the command line args into settings
//
// prints an error and exits if any of the opts are invalid
void
parseSettingsFromArgs (int argc, char* argv[], settings_t& settings);

// generate a config file that corresponds to the settings
//
// returns true if the file was successfully written to
bool
genConfigFromSettings (const std::string& pathToFile,
                       const settings_t& settings = {});

int
main (int argc, char* argv[])
{
  /* Configure program settings */
  auto [pathToConfig, pathToGenConfig] = parseConfigPathsFromArgs (argc, argv);
  settings_t settings {};
  if (pathToGenConfig.has_value () && pathToGenConfig.value () != "")
  {
    // only parse from config if a path was specified
    if (pathToConfig.has_value () && pathToConfig.value () != "")
    {
      if (!parseSettingsFromConfig (pathToConfig.value (), settings))
        EXIT_WITH_USAGE_ERROR ("config", ARG_ERR (5));
    }

    parseSettingsFromArgs (argc, argv, settings);
    if (!genConfigFromSettings (pathToGenConfig.value (), settings))
      EXIT_WITH_USAGE_ERROR ("gen-config", ARG_ERR (5));
  }
  else
  {
    // always parse but only throw an error if a config path was specified
    if (!parseSettingsFromConfig (
          pathToConfig.value_or (DEFAULT_PATH_TO_CONFIG), settings))
    {
      if (!pathToConfig.has_value ())
        genConfigFromSettings (DEFAULT_PATH_TO_CONFIG);
      else if (pathToConfig.value () != "")
      {
        if (pathToConfig.value () == DEFAULT_PATH_TO_CONFIG)
          EXIT_WITH_USAGE_ERROR ("config", OPT_ERR (2));
        else
          EXIT_WITH_USAGE_ERROR ("config", ARG_ERR (5));
      }
    }

    parseSettingsFromArgs (argc, argv, settings);
  }

  /* Init */
  Model model (settings);
  View view (&model);

  view.update ();

  /* Main loop */
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

std::pair<std::optional<std::string>, std::optional<std::string>>
parseConfigPathsFromArgs (int argc, char* argv[])
{
  /* State */
  int longOpt = 0;
  std::optional<std::string> pathToConfig {};
  std::optional<std::string> pathToGenConfig {};

  /* Parse using getopt_long */
  const option longOptions[] = { option { "help", 0, NULL, 'h' },
                                 option { "config", 2, &longOpt, 4 },
                                 option { "no-config", 0, &longOpt, 5 },
                                 option { "gen-config", 2, &longOpt, 6 },
                                 option { 0, 0, 0, 0 } };
  while (true)
  {
    switch (getopt_long (argc, argv, "-:h", longOptions, NULL))
    {
      case 'h':
      {
        PRINT_USAGE (argv[0]);
        exit (EXIT_SUCCESS);
      }
      case 0:
      {
        /* Long opt */
        switch (longOpt)
        {
          case 4:
          {
            /* --config */
            if (pathToConfig.has_value () && pathToConfig.value () == "")
              EXIT_WITH_USAGE_ERROR ("config", OPT_ERR (1));

            if (optarg)
              pathToConfig = optarg;
            else
              pathToConfig = DEFAULT_PATH_TO_CONFIG;

            break;
          }
          case 5:
          {
            /* --no-config */
            if (pathToConfig.has_value () && pathToConfig.value () != "")
              EXIT_WITH_USAGE_ERROR ("config", OPT_ERR (1));

            pathToConfig = "";

            break;
          }
          case 6:
          {
            /* --gen-config */
            if (optarg)
              pathToGenConfig = optarg;
            else
              pathToGenConfig = DEFAULT_PATH_TO_CONFIG;

            break;
          }
        }
        break;
      }
      case -1:
      {
        /* No more opts or args */
        return { pathToConfig, pathToGenConfig };
      }
    }
  }
}

bool
parseSettingsFromConfig (const std::string& pathToFile, settings_t& settings)
{
  std::optional<std::string> txt = fileToString (pathToFile);
  if (!txt.has_value ())
    return false;

  styml::Document root;
  try
  {
    root = styml::parse (txt.value ());
  }
  catch (styml::ParseException& e)
  {
    EXIT_WITH_ERROR (e.what ());
  }

  if (root["version"].as<std::string> () != CONFIG_VERSION)
    EXIT_WITH_ERROR (pathToFile +
                     " has an unsupported version number.\n"
                     "The version must be " +
                     CONFIG_VERSION);
  if (root.hasKey ("width"))
    settings.width = root["width"].as<size_t> ();
  if (root.hasKey ("height"))
    settings.height = root["height"].as<size_t> ();
  if (root.hasKey ("flow_sources"))
    settings.numFlowSrcs = root["flow_srcs"].as<unsigned> ();
  if (root.hasKey ("flow_sinks"))
    settings.numFlowSinks = root["flow_sinks"].as<unsigned> ();
  if (root.hasKey ("start_cells"))
    settings.numStartCells = root["start_cells"].as<unsigned> ();
  if (root.hasKey ("seed"))
    settings.seed = root["seed"].as<unsigned> ();

  return true;
}

void
parseSettingsFromArgs (int argc, char* argv[], settings_t& settings)
{
  /* State */
  int opt = '\0';
  int longOpt = 0;
  char prevOpt = '\0';
  int numArgsPrevOpt = 0;
  // reset after parseConfigPathFromArgs
  optind = 1;

  bool flowSinksSet = false;

  /* Parse using getopt_long */
  const option longOptions[] = { option { "help", 0, NULL, 'h' },
                                 option { "size", 1, NULL, 's' },
                                 option { "flow-sources", 1, &longOpt, 1 },
                                 option { "flow-sinks", 1, &longOpt, 2 },
                                 option { "start-cells", 1, NULL, 'C' },
                                 option { "seed", 1, &longOpt, 3 },
                                 option { "config", 2, &longOpt, 4 },
                                 option { "no-config", 0, &longOpt, 5 },
                                 option { "gen-config", 2, &longOpt, 6 },
                                 option { 0, 0, 0, 0 } };
  const int numLongOptions = 9;
  while (true)
  {
    switch (opt = getopt_long (argc, argv, "-:s:C:h", longOptions, NULL))
    {
      // case h handled by parseConfigFromArgs
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
            /* Either arg for long opt or arg without an opt */
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
              EXIT_WITH_USAGE_ERROR ("flow-sinks", ARG_ERR (3));
            else if (ptr != end || ec != std::errc ())
              EXIT_WITH_USAGE_ERROR ("flow-sinks", ARG_ERR (2));
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
            break;
          }
            // case 4 handled by parseConfigFromArgs
            // case 5 handled by parseConfigFromArgs
            // case 6 handled by parseConfigFromArgs
        }
        break;
      }
      case -1:
      {
        /* No more opts or args */
        return;
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
        /* Unknown opt, or opt with too many args */
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

bool
genConfigFromSettings (const std::string& pathToFile,
                       const settings_t& settings)
{
  // build the yaml doc
  styml::Document root;
  root = styml::NodeType::MAP;

  root["version"] = CONFIG_VERSION;
  root["width"] = settings.width;
  root["height"] = settings.height;
  root["flow_srcs"] = settings.numFlowSrcs;
  root["flow_sinks"] = settings.numFlowSinks;
  root["start_cells"] = settings.numStartCells;
  root["seed"] = settings.seed;

  // write the doc to file
  std::string txt = root.asYaml ();

  std::ofstream fileWriter (pathToFile);
  if (!fileWriter.is_open ())
    return false;

  fileWriter
    << "# Automatically generated config file for Life\n"
    << "#\n"
    << "# The settings specified below will automatically be applied to the\n"
    << "#  simulation.\n"
    << "# To specify which config file to apply, use --config=<path> where\n"
    << "#  <path> is the path of the config to use.\n"
    << "# To regenerate the config file, use --gen-config.\n"
    << "# Specifying --config and --gen-config together will use the config\n"
    << "#  file from --config to generate the new config file.\n"
    << "\n"
    << txt << "\n";
  if (!fileWriter)
    return false;

  return true;
}