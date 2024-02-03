# blackpill-node
## Build firmware
### Release ###
```
mkdir ./build
cd ./build
cmake ..
make silver
```
### Debug ###
```
mkdir ./build
cd ./build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make silver
```
## Flash
### Flash firmware ###
```
make flash
```
### Erase firmware ###
```
make erase
```
## Build tests
### Build ###
```
mkdir ./build
cd ./build
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON ..
make tests
```
### Run ###
```
make test
```
