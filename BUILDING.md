How to build reliable.io
========================

## Building on Windows

Download [premake 5](https://premake.github.io/download.html) and copy the **premake5** executable somewhere in your path. Please make sure you have at least premake5 alpha 13.

You need Visual Studio to build the source code. If you don't have Visual Studio 2015 you can [download the community edition for free](https://www.visualstudio.com/en-us/downloads/download-visual-studio-vs.aspx).

Once you have Visual Studio installed, go to the command line under the netcode.io/c directory and type:

    premake5 solution

This creates reliable.sln and opens it in Visual Studio for you.

Now you can build the library and run individual test programs as you would for any other Visual Studio solution.

## Building on MacOS and Linux

First, download and install [premake 5](https://premake.github.io/download.html). Please make sure you have at least premake5 alpha 13.

Now go to the command line under the reliable.io directory and enter:

    premake5 gmake

Which creates makefiles which you can use to build the source via:

    make all

Alternatively, you can use the following shortcuts to build and run test programs directly:

    premake5 test           // build and run unit tests

    premake5 soak           // run the soak test that exercises all library functionality

    premake5 fuzz           // run the fuzz test that tests the library is able to correctly handle random data
   
If you have questions please create an issue at https://github.com/networkprotocol/reliable.io and I'll do my best to help you out.

cheers

 - Glenn
