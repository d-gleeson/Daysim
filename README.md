# DAYSIM

This repository is a fork of the [RADIANCE mirror repository maintained by NREL](https://github.com/NREL/Radiance) and includes the source code for the [DAYSIM](http://daysim.ning.com/) suite. _Programs in this repository compile on Windows (VS2013) and Mac (XCode), but **testing is not complete**._ The following DAYSIM programs are part of this repository:

Programs maintained here (in src/daysim):
* ds_el_lighting
* ds_illum
* ds_shortterm
* gen_dc
* gen_dgp_profile
* gen_directsunlight
* gen_reindl
* gen_single_office
* gencumulativesky
* radfiles2daysim
* rotate_scene
* scale_dc

Programs maintained here that require manual intervention to compile (in src/rt):
* rtrace_dc
* rtrace_dc_2305

Programs maintained on the main RADIANCE repository and included here:
* epw2wea
* evalglare
* gendaylit
* _all the original RADIANCE programs_

## How to Compile Daysim

###Clone the repository to your own computer

1. Open cmd (Windows) or Terminal (Mac) and navigate to the folder you would like to add the source code to using the cd command. Mine is _D:/myname_ for Windows and _Users/myname_ for Mac.
2. Clone the repository to your local machine. The command is:
`git clone https://github.com/MITSustainableDesignLab/Daysim.git`
3. Navigate into the repository folder that was just created. The command is:
`cd Daysim`
4. Check out the combined branch of the repository to make it current. The command is:
`git checkout combined`
5. Add an upstream reference to Radiance. The command is:
`git remote add upstream https://github.com/NREL/Radiance.git`

###Create the Daysim project

1. Open CMake. You can download CMake from https://cmake.org/.
2. Enter the location of the Daysim project folder in the line �Where is the source code:�. In the example above, mine is _D:/myname/Daysim_ for Windows and _Users/myname/Daysim_ for Mac. 
3. Enter a **different** location in the line �Where to build the binaries:�. Using a different location prevents your built project (which is specific to your operating system) from being committed to the repository. The location you select could be a folder that doesn�t exist yet. Mine is _D:/myname/Daysim64_ for Windows because I compile 64-bit programs and _Users/myname/DS_Build_ for Mac.
4. Click _Configure_ and select the generator for the project. I use Visual Studio 12 2013 Win64. For Mac, I use XCode.
	- Optional: After the project configures, you may see some errors related to Qt5, which is used to build Radiance�s rvu program. If you have Qt5 installed, you can enter its location at the entry for Qt5Widgets_DIR, which currently says Qt5Widgets_DIR-NOTFOUND. For Windows, mine is _C:\Qt\5.5\msvc2013_64\lib\cmake\Qt5Widgets_. Then click _Configure_ again.
5. Click _Generate_ to build the project for your operating system.

###Compile Daysim

Windows:

1. Open the Visual Studio project file, which is located in your build folder. Mine is _D:/myname/Daysim64/daysim.sln_.
2. In the toolbar, you may choose either �Debug� or �Release� settings. Debug (the default) will take longer to compile and run, but will allow you to step through running code using the debugger. Use Release settings for creating distributable packages.
3. Choose Build > Build Solution from the menus.
    - Optional: You may see errors related to the normtiff and ra_tiff projects. To fix these, you need to build libtiff.lib. To do this:
        1. Navigate to _Start_ > _All Programs_ > _Visual Studio 2013_ > _Visual Studio Tools_ and run VS2013 x64 Native Tools Command Prompt (assuming you are creating a 64-bit build).
		2. Navigate to the libtiff directory, which downloaded when you built the daysim project. Mine is _D:\myname\Daysim64\Downloads\Source\radiance_support\src\px\tiff\libtiff_.
		3. Run this command:
`nmake /f makefile.vc all`
		4. In CMake, set the following variables:
			- TIFF_HEADER: `D:/myname/Daysim64/Downloads/Source/radiance_support/src/px/tiff/libtiff`
			- TIFF_LIBRARY: `D:/myname/Daysim64/Downloads/Source/radiance_support/src/px/tiff/libtiff/libtiff.lib`
		5. In CMake, click _Generate_.
		6. In Visual Studio, choose _Build_ > _Build Solution_.
4. Check that the executables have been built to your bin folder. Mine are in _D:\myname\Daysim64\bin\Debug_ or _D:\myname\Daysim64\bin\Release_.

Mac:

1. Open the XCode project generated by CMake, located in your build folder. Mine is _Users/myname/DS_Build/daysim.xcodeproj_.
2. In the top left corner, choose ALL_BUILD (default) to build all executables, or select certain programs from the drop-down menu. Press the play button to build the project according to specifications.
3. View any warnings or errors in the navigation panel on the left. Even if XCode indicates �Build Failed�, errors that caused the failure may not have occurred in crucial programs.
4. Check that the executables have been built to your bin folder. Mine are in _Users/myname/DS_Build/bin/Debug_.

###Pull Updates from Radiance

1. Download updates from the upstream Radiance branch. The command is:
`git fetch upstream`
2. Merge the changes into your working set. The command is:
`git merge upstream/combined`

You can do the same to pull updates from Daysim by replacing upstream with origin.
