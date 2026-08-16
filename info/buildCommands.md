debug:

cmake -S . -B builds/debug -D CMAKE_BUILD_TYPE=Debug
cmake --build builds/debug -j

release:
cmake -S . -B builds/release -D CMAKE_BUILD_TYPE=Release -D CMAKE_CXX_FLAGS="-mbmi -mbmi2 -msse4.2"
cmake --build builds/release -j

prof:
cmake -S . -B builds/prof -D CMAKE_BUILD_TYPE=RelWithDebInfo -D CMAKE_CXX_FLAGS="-mbmi -mbmi2 -msse4.2"
cmake --build builds/prof -j

clean:
rm -rf builds/

