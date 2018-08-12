# VCMPBrowser [![Build status](https://ci.appveyor.com/api/projects/status/m1d90tw3j3enmlrd?svg=true)](https://ci.appveyor.com/project/ysc3839/vcmpbrowser/build/artifacts)

A new VCMP browser.
# Features
* All functions in the official browser.
* Server filter. (Not implemented yet)
* Self update.
* Multi-language support.
* Lan server search.
* More......
# Build
## Build curl
This project uses [vcpkg](https://github.com/Microsoft/vcpkg) to build curl.

After installing vcpkg, add `set(CURL_USE_WINSSL ON)` to `<vcpkg_dir>\triplets\x86-windows-static.cmake`.

It will let curl use Windows' SSL library instead of OpenSSL.

Then run `vcpkg install curl --triplet x86-windows-static`.
