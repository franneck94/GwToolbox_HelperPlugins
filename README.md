# [TAS] Clown Plugins for GWToolbox

Refer to the [documentation](https://franneck94.github.io/GwToolbox_HelperPlugins/).

Features are:

- Smart Chat Commands
- Oldschool Dialog Window
- UW Helper: LT, EMO and Ranger Terra
- Auto Follow
- Smart Hero Commands

## Developement

- CMake 3.21+, Git
- MSVC 2022 with C++23
- Optional: Python 3.8+ for the Documenation

```bash
git clone --recursive [GwToolbox_HelperPlugins](https://github.com/franneck94/GwToolbox_HelperPlugins)
```

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A Win32 -B .
```

Then open it in VSCode or VS2022.
