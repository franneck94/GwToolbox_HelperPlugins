# GWToolbox_AllowAllDialogs
GWToolbox plugin. Removes blocker in GWToolbox to allow any dialog to be immediately sent. 

Change `GWTOOLBOXPP_SOURCE_DIR` in `CMakeLists.txt`, `cmake/gwca.cmake` and `cmake/minhook.cmake`

`cmake build -B build -A Win32`

NOTE: Sending dialogs that aren't available automatically flags the sendng character's uuid for investigation, so only use this if you don't care about that
