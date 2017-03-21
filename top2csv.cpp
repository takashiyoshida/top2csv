#include <iostream>
#include <iomanip>
#include <memory>
#include <fstream>
#include <string>
#include <regex>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

struct row_type
{
  int hour;
  int min;
  int sec;
  std::vector<float> columns;
};

const int VIRT_COL = 4;
const int CPU_COL = 8;

/**
 *  Parse a top log from std::cin and produces the CSV output on std::cout.
 *
 *  Since it's using the standard input and output for convenience, necessary
 *  piping needs to be done before the function is called by the program.
 *
 *  @param processes A list of process names to be analysed.
 *  @param top_column The column from the top log to be collected. Only 4 (VIRT)
 *                    and 8 (%CPU) are supported.
 *  @return 0 if everything went fine, 1 otherwise.
 */
int parse_and_print(std::vector<std::string> processes, int top_column)
{
  std::vector<row_type> rows;
  std::string line;
  while (std::getline(std::cin, line))
    {
      static const std::regex
        top{"^top - ([0-2][0-9]):([0-5][0-9]):([0-5][0-9])"};
      static const std::regex tokenizer{"[^ \t]+"};
      std::smatch subs;
      if (regex_search(line, subs, top))
        {
          // Converts to ints, it takes less space
          row_type row {std::stoi(subs[1]),
              std::stoi(subs[2]),
              std::stoi(subs[3]),
              std::vector<float>(processes.size(), 0.f)};
          rows.push_back(row);
        }
      else
        {
          if (rows.size() == 0)
            {
              std::cerr << "Malformed top log; logs must start by \"top - \".\n";
              return 1;
            }
          auto row = &rows.back();
          std::regex_token_iterator<std::string::iterator> it
          {line.begin(), line.end(), tokenizer};
          std::regex_token_iterator<std::string::iterator> end;
          int count = 0;
          std::string possibly_val, possibly_proc;
          while (it != end)
            {
              if (count == top_column) { possibly_val = *it; }
              if (count == 11) { possibly_proc = *it; }
              ++it;
              ++count;
            }
          if (count > 11)
            {
              auto found = std::find(processes.begin(), processes.end(),
                                     possibly_proc);
              if (found != processes.end())
                {
                  float val = std::stof(possibly_val);
                  if (*--possibly_val.end() == 'm') { val *= 1024.0; }
                  row->columns[found - processes.begin()] += val;
                }
            }
        }
    }

  std::cout << "Hour,Minute,Second";
  for (auto&& p : processes) { std::cout << "," << p; }
  std::cout << "\n";
  std::cout << std::fixed;
  if (top_column == VIRT_COL)
    { std::cout << std::setprecision(0); }
  else
    { std::cout << std::setprecision(1); }
  for (auto&& row : rows)
    {
      std::cout << row.hour << "," << row.min << "," << row.sec;
      for (auto&& col : row.columns) { std::cout << "," << col; }
      std::cout << "\n";
    }
  std::cout.flush();
  return 0;
}

/**
 *  Manages program options and calls parse_and_print as needed.
 *
 *  @return 0 is everything went fine, 1 otherwise.
 */
int main (int argc, char **argv)
{
  std::cout.sync_with_stdio(false);
  std::cin.sync_with_stdio(false);
  std::cin.tie(nullptr);

  std::string input_path;
  std::string output_path;
  std::string find_path;

  po::options_description desc{"Allowed options"};
  desc.add_options()
    ("help,h", "Print this help")
    ("cpu,c", "Gather CPU usage for each process."
     "  One of --cpu or --mem must be specified, only.")
    ("mem,m", "Gather memory usage for each process."
     "  One of --cpu or --mem must be specified, only.")
    ("find,f", po::value< std::string >(&find_path),
     "Search for all top.log[.*] files and generate outputs at the locations "
     "where the files have been found.  When --find is used, --input-file and "
     "--output-file are ignored.")
    ("input-file,i", po::value< std::string >(&input_path),
     "Input file to read from, instead of stdin.")
    ("output-file,o", po::value< std::string >(&output_path),
     "Output file to write to, instead of stdout.")
    ("preset,p", po::value<std::string>(),
     "Preset is one of 'all', 'ats', 'cms', 'dcs', 'ecs', or 'sms'.  "
     "When --preset is used, any processes specified are added to the preset.")
    ("processes", po::value< std::vector<std::string> >(),
     "List of processes used to generate information."
     "  At least one process must be specified.  The option --processes can be "
     "omitted if the list of processes is specified at the end of the command.")
    ;
  po::positional_options_description p;
  p.add("processes", -1);
  po::variables_map vm;

  try
    {
      po::store(po::command_line_parser(argc, argv)
                .options(desc).positional(p).run(), vm);
      po::notify(vm);
    }
  catch (const po::unknown_option& e)
    {
      std::cerr << "Error: " << e.what() << std::endl;
      return 1;
    }

  if (vm.count("help"))
  {
    std::cout << desc << "\n";
    return 0;
  }

  int top_column = VIRT_COL;
  if (vm.count("cpu") + vm.count("mem") != 1)
    {
      std::cerr << "Error: only one of --cpu or --mem must be specified."
                << std::endl;
      return 1;
    }
  if (vm.count("cpu"))
    { top_column = CPU_COL; }

  std::vector<std::string> processes;
  if (vm.count("preset") == 1)
    {
      std::string preset = vm["preset"].as<std::string>();
      if (preset == "all")
        processes = {"ascmanager", "BmfCol", "BmfExcReceiver", "BmfExcSender",
                     "CctCtl", "ctlkcmdpro", "daccompms", "daccomrss",
                     "daccontrol", "dbpoller", "dbserver", "dpckeqpmgr",
                     "dpckvarmgr", "EcsSmc", "EcsSys", "ftsserver", "HdvServer",
                     "historyserver", "inputmgr", "LoginServer", "opmserver",
                     "PasCtl", "PisCtl", "RadCom", "RadCtl", "RadPgr",
                     "ReaPrgServer", "scsalarmserver", "scsctlgrcserver",
                     "SigCtlServer", "SigDpc", "SigLdt", "SigLoc",
                     "taonameserv", "TelSvr", "tmcpex", "tmcsup" };
      if (preset == "ats")
        processes = {"ascmanager", "BmfCol", "ctlkcmdpro",
                     "daccompms", "daccomrss", "daccontrol", "dbpoller",
                     "dbserver", "dpckeqpmgr", "dpckvarmgr", "ftsserver",
                     "HdvServer", "inputmgr", "ReaPrgServer", "scsalarmserver",
                     "SigCtlServer", "SigDpc", "SigLdt", "SigLoc",
                     "taonameserv", "tmcpex", "tmcsup" };
      if (preset == "cms")
        processes = {"ascmanager", "BmfCol", "BmfExcReceiver", "BmfExcSender",
                     "CctCtl", "ctlkcmdpro", "daccompms", "daccontrol",
                     "dbpoller", "dbserver", "dpckeqpmgr", "dpckvarmgr",
                     "ftsserver", "HdvServer", "historyserver", "inputmgr",
                     "LoginServer", "opmserver", "PasCtl", "PisCtl", "RadCom",
                     "RadCtl", "ReaPrgServer", "scsalarmserver",
                     "scsctlgrcserver", "taonameserv", "TelSvr"};
      if (preset == "sms")
        processes = {"ascmanager", "BmfCol",
                     "CctCtl", "ctlkcmdpro", "daccompms", "daccomrss",
                     "daccontrol", "dbpoller", "dbserver", "dpckeqpmgr",
                     "dpckvarmgr", "EcsSmc", "EcsSys", "ftsserver", "HdvServer",
                     "historyserver", "inputmgr", "LoginServer", "PasCtl",
                     "PisCtl", "RadCom", "RadCtl", "RadPgr", "ReaPrgServer",
                     "scsalarmserver", "scsctlgrcserver", "SigCtlServer",
                     "SigDpc", "SigLdt", "SigLoc", "taonameserv", "TelSvr"};
      if (preset == "dcs")
        processes = {"ascmanager", "BmfCol",
                     "CctCtl", "ctlkcmdpro", "daccompms", "daccomrss",
                     "daccontrol", "dbpoller", "dbserver", "dpckeqpmgr",
                     "dpckvarmgr", "EcsSmc", "EcsSys", "ftsserver", "HdvServer",
                     "historyserver", "inputmgr", "LoginServer", "PasCtl",
                     "PisCtl", "RadCom", "RadCtl", "RadPgr", "ReaPrgServer",
                     "scsalarmserver", "scsctlgrcserver", "SigCtlServer",
                     "SigDpc", "SigLdt", "SigLoc", "taonameserv", "TelSvr",
                     "tmcsup"};
      if (preset == "ecs")
        processes = {"ascmanager", "BmfCol", "daccompms", "daccomrss",
                     "daccontrol", "dbpoller", "dbserver", "dpckeqpmgr",
                     "dpckvarmgr", "EcsSmc", "EcsSys", "HdvServer", "inputmgr",
                     "ReaPrgServer", "scsalarmserver", "scsctlgrcserver",
                     "taonameserv" };
      if (processes.size() == 0)
        {
          std::cerr << "Error: unknown preset '" << preset << "'" << std::endl;
          return 1;
        }
    }
  if (vm.count("processes"))
    {
      for (auto&& p : vm["processes"].as<std::vector<std::string> >())
        {
          if (find(processes.begin(), processes.end(), p) == processes.end())
            processes.push_back(p);
        }
    }
  else if (processes.size() == 0)
    {
      std::cerr << "Error: at least one process must be specified."
                << std::endl;
      return 1;
    }

  // The setup is done! Can start doing some actual processing...

  std::streambuf* rdin = std::cin.rdbuf();
  std::streambuf* rdout = std::cout.rdbuf();
  int ret_val = 0;
  if (vm.count("find")) // find all possible files, and parse them
    {
      fs::path root(vm["find"].as<std::string>());
      try
        {
          if (!exists(root))
            {
              std::cerr << "Error accessing path: " << root << std::endl;
              return 1;
            }
          if (!is_directory(root))
            {
              std::cerr << "Error: " << root << " is not a directory" << std::endl;
              return 1;
            }
          static const std::regex pattern{"top\\.log(\\.[0-9])?"};
          for (auto&& entry : fs::recursive_directory_iterator(root))
            {
              if (is_regular_file(entry.path())
                  && std::regex_match(entry.path().filename().string(), pattern))
                {
                  std::cout << "Found: " << entry.path().string() << std::endl;
                  std::ifstream ifs(entry.path().string());
                  if (ifs) // if file cannot be opened, silently skip.
                    {
                      output_path = entry.path().string();
                      if (top_column == VIRT_COL) { output_path += "-mem.csv"; }
                      else { output_path += "-cpu.csv"; }
                      std::ofstream ofs(output_path);
                      if (ofs)
                        {
                          std::cout << "Writing: " << output_path << std::endl;
                          std::cout.flush();
                          std::cin.rdbuf(ifs.rdbuf());
                          std::cout.rdbuf(ofs.rdbuf());
                          // Silently ignore errors here.
                          parse_and_print(processes, top_column);
                          std::cin.rdbuf(rdin);
                          std::cout.rdbuf(rdout);
                          ofs.close();
                        }
                      ifs.close();
                    }
                }
            }
        }
      catch (const fs::filesystem_error& e)
        {
          std::cerr << "Error: " << e.what();
          return 1;
        }
    }
  else
    {
      std::ifstream input_file;
      std::ofstream output_file;
      if (vm.count("input-file"))
        {
          input_file.open(input_path.c_str());
          if (!input_file)
            {
              std::cerr << "Error opening file: " << input_path << std::endl;
              return 1;
            }
          rdin = std::cin.rdbuf(input_file.rdbuf());
        }
      if (vm.count("output-file"))
        {
          output_file.open(output_path.c_str());
          if (!output_file)
            {
              std::cerr << "Error opening file: " << output_path << std::endl;
              return 1;
            }
          rdout = std::cout.rdbuf(output_file.rdbuf());
        }
      ret_val = parse_and_print(processes, top_column);
    }

  std::cin.rdbuf(rdin);
  std::cout.rdbuf(rdout);
  return ret_val;
}
