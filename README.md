# pueoEvent

ROOT-based I/O for PUEO. 

As of now, this is primarily for simulation, until the DAQ is farther along. 

Dec. 19 2024 Update:
    The `CMakeLists.txt` makes sure that the `DEFAULT_PUEO_VERSION` comes from the `pueo-data` repo version.


# Instructions for building
This repo has a dependency on pueo-data. Be sure to build pueo-data with an install location that is used by this repo with some paths you know of.
Right now, the best option is to make a folder and env variable for `PUEO_UTIL_INSTALL_DIR` like:
```
export PUEO_UTIL_INSTALL_DIR = /path/to/pueo-install
```

And now clone pueo-data, then

```
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$PUEO_UTIL_INSTALL_DIR
make
make install
```

Now pueo-data is installed into the appropriate directory to access when building this repo- pueoEvent. Note: be sure to have `PUEO_UTIL_INSTALL_DIR` set to same location as before (if you restart your shell or whatever and didnt save this var)
Clone this repo and then, 

```
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$PUEO_UTIL_INSTALL_DIR
make
make install
```
Report issues on github or slack. 
