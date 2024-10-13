# c
this is a project/document/unittest management

## Development
### build/run
- `cmake -S. -B build -Denable_testing=ON -DCMAKE_BUILD_TYPE:STRING=Debug`
- `cmake --build build --config Debug`
- `cmake --install build --config Debug --prefix build/_c`
- `cd test/project2`
- linux: `LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PWD/../../build/_c/lib LIBRARY_PATH=$LD_LIBRARY_PATH:$PWD/../../build/_c/lib C_INCLUDE_PATH=$PWD/../../build/_c/include ../../build/_c/bin/c build`
- windows: `..\..\build\_c\bin\c.exe build`