# text-complex
The `text-complex` library aims to provide an API for interacting with
`sfnt`-format font files.

## Goals
This project aims to provide easy-to-use access to the font drawing
information in TrueType and similar files. The target API language
is C 89. In addition:

- The API should provide convenient access to blocks not directly
  supported by built-in features.

- The `text-complex` library should provide Graphite-based layout
  and kerning support.

- The interface should be simple and orthogonal.

- The source should not use more language features or libraries
  than necessary, and should not be larger (in lines of code)
  than needed.

- This library should support at least one form of emoji font encoding
  (likely COLR/CPAL).

## Build

This project will use CMake for building. Developers can obtain CMake from
the following URL:
[https://cmake.org/download/](https://cmake.org/download/)

To use CMake with this project, first make a directory to hold the build
results. Then run CMake in the directory with a path to the source code.
On UNIX, the commands would look like the following:
```
mkdir build
cd build
cmake ../text-complex
```

Running CMake should create a build project, which then can be processed
using other tools. Usually, the tool would be Makefile or a IDE project.
For Makefiles, use the following command to build the project:
```
make
```
For IDE projects, the IDE must be installed and ready to use. Open the
project within the IDE.

## License
This project uses the Unlicense, which makes the source effectively
public domain. Go to [http://unlicense.org/](http://unlicense.org/)
to learn more about the Unlicense.

Contributions to this project should likewise be provided under a
public domain dedication.
