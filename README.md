This is a little toy Windows program I wrote over the course of several days that dumps all the Windows icons (.ico files and icon resources in executables and libraries) from all the files in a given folder into a winodow. Why? Back during the oft-spoke days of a long-lost computing legend now known only as "Windows 95", I was a kid who liked to explore the system, despite having absolutely no idea what I was looking at. And when I figured out how to edit file associations, I spent a lot of time exploring the icons in the C:\Windows\System folder. So you can consider this a nostalgia-bomb of sorts, I gues... (Also I had problems with Mac OS X which stalled package ui's development; I should get back to it now...)

This program also represents me learning to use the list view common control, which will be important to package ui's development later.

It can be built with both MinGW and Microsoft Visual C++'s command-line tools, and has been tested on 32-bit Windows XP, 64-bit Windows 7, and in wine. The GNUmakefile is for a MinGW-w64 cross-compiling environment, using the way Ubuntu distributes the mingw-w64 packages. The Makefile is for Microsoft nmake. Peek into these files to see how they work.

To run the program, simply double-click it. It will ask you to pick a folder, and then gather the icons from that folder. Once it does so, you will see a window with the icons from all files that have icons, grouped by filename. Other than closing the window, which closes the program, the only thing you can do is right-click, which switches between showing large icons and showing small icons.

On Windows Vista and up, groups can also be collapsed.

You can alternatively provide a directory name to scan on the command line.

This program will not scan recursively into directories; it will only show files within that directory. If running as a 32-bit process on 64-bit Windows, the automatic filesystem redirection provided by WOW64 is disabled during the file browsing and icon gathering, so you can peek through both 32-bit and 64-bit files with both 32-bit and 64-bit builds.

This program requires Common Controls version 6, so will only run on Windows XP and up. I don't know what the minimum service pack version of Windows XP is supported. Right now you need to use a manifest, which is provided in the source distribution.

Care has been taken to use as little memory as possible, since this is a heavy operation. There are some problem points discussed in the TODO section, but for the most part this should be enough.

As it stands, this program cannot be used to actually extract icons to files, change icons, etc.; it will only view the icons.

Enjoy!

TODO
- embed the manifest for comctl32.dll version 6
	- see how to do it in MinGW, if possible; otherwise, we're going to have to do it manually at runtime, like package ui does
- http://stackoverflow.com/questions/23429327/winapi-lvm-deleteitem-behaves-in-unusual-ways-with-grouped-list-views
- there are a bunch of TODOs scattered around the various files; those need to be done too
- there's one bit of global state left: the `currneticons[]` array used for swapping between large and small icons
- there are some memory hotspots
	- we can dump most of the LVGROUP memory and replace it in-place with pointers to the group names for the process of sorting, then dump that
	- we don't free the LVITEM set
	- large and small in `getIcons()` don't need to be repeatedly allocated and freed; they can be allocated once, grown as needed, and then freed at the end
- do I need to apply `controlfont` to the list view? it doesn't look like I do...
- add errno to panic()
	- get rid of the stray newline at the end of the GetLastError() message
	- apply the above two changes to the scratch Windows program in misctestprogs
- switch to embedding the manifest either as a resource (via FraGag) or internally (like package ui does)
- FraGag points out that the WOW64 redirection can screw with COM, ,which the Browse for Folders dialog uses...
- add /debug to the nmake Makefile's /link path? and is there another way to export type information too? or to use COFF symbol tables?
- change tthe GNUmakefile to also work with MinGW on Windows

NICE THINGS TO HAVE
- get the resource ID for each icon and print that
- type in the list view to navigate groups instead of icon labels
- long-filename aware in general; there's a TODO comment about it but still
- this might be the cause of our dummy item nonsense http://blogs.msdn.com/b/oldnewthing/archive/2003/12/12/56061.aspx#56064
	- but we always ZeroMemory() so huh???
