top2csv
=======

Produce CSV (comma separated value) files from top log by selecting a single
column for a particular set of processes.

This repository contains the source code for top2csv.  The source is portable
between Windows and Linux.


Usage
-----

This version of top2csv is more advanced than the previous python script.  It
contains predefined presets and it is able to recursively find the CSV files to
generate starting from a root directory.

Generate memory CSV logs for all top.log(.[0-9])? files found under &lt;dir&gt;:

  $ top2csv.exe --find &lt;dir&gt; --mem --preset all

Generate cpu CSV logs for a give file CMS top log and print it on standard
ouput:

  $ top2csv.exe --cpu --preset cms -i mem/top.log

As you can see, the order of the arguments matters little. See:

  $ top2csv.exe --help

...for more information.


Compilation
-----------

Requirements:
- a recent version of G++ or compiler that supports C++11 standard.
- Boost::program_options
- Boost::filesystem
- Boost::system
- CMake
- Mingw32 (for cross-compilation) and the above libraries for the Mingw32
  environment.

On linux:

  $ mkdir build-g++
  $ cd build-g++
  $ cmake ..
  $ make

should be sufficient to generate top2csv.

Cross compiling for Windows on linux:

  $ mkdir build-mingw32
  $ cd build-mingw32
  $ i686-w64-mingw32-cmake ..
  $ make

Depending on your distribution, the cmake environment for Mingw32 might not be
available, and thus, you might have to write your own toolchain file.