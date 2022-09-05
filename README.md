# PremiseSys
Drivers for Premise SYS home automation software.

# Install and configure required components.
I have been building with Visual Studio 2019, although 2022 should work as well.

The include files and sysuuid.lib are needed from the old driver SDK that was available. You can get it from the Wayback Machine at https://web.archive.org/web/20060513190224/http://www.premisesystems.com/downloads/hsdkinstall.exe. Run the EXE and install the SDK somewhere. Then, copy the contents of the include and lib folders from the installation to the HSDK\include and HSDK\lib folders in this project. You will need to make one edit to a file to get things to build.

Open HSDK\include\driverutil.h and change line number 3531 from
		BYTE* pszLine;
to
		BYTE* pszLine = nullptr;

These projects compile with the /permissive command line switch due to a bunch of issues in the HSDK files. It is possible to fix all of them, but the runtime behavior stays the same, so it isn't really worth it.'

The Bond devices use a non-persistent HTTP mechanism, which the original Premise had no direct support for. So, the Bond driver uses libcurl (https://curl.se/libcurl/) to make the http requests. The easiest way to get this is to use vcpkg (https://vcpkg.io/en/index.html). Follow the directions here (https://vcpkg.io/en/getting-started.html) to install vcpkg and then run the following commands. The first command installs curl and the second will make any packages directly accessible from Visual Studio.

vcpkg install curl
vcpkg integrate install

You will need to copy the libcurl binaries that you built from vcpkg.
For debug builds, copy zlibd1.dll and libcurl-d.dll from vcpkg\installed\x86-windows\debug\bin to c:\program files(x86)\premise\sys\bin.
For release builds, copy zlibd1.dll and libcurl.dll from vcpkg\installed\x86-windows\bin to c:\program files(x86)\premise\sys\bin.

# Build
Load the solution in Visual Studio and build. You will see some warnings, but the projects should build.

# Install Bond and RadioRA2 drivers
To install the files, launch an administrator command prompt.
You can use the upd.bat file to kill SYS, copy the bond and radiora2 driver DLLs and PDBs, optionally reset the Premise data, and relaunch the SYS service and client.
For example, the following will copy the Debug binaries and reset all SYS data.
upd.bat Debug reset
You need to do this after building the drivers to copy them to the right location. You only need to add "reset" when you change one of the XML files. The reset argument makes sure the latext XML is loaded.
