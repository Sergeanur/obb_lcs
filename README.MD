# obb_lcs
[![Join Discord](https://img.shields.io/badge/discord-join-7289DA.svg?logo=discord&longCache=true&style=flat)](https://discord.gg/EPjzhC6n) [![Support Author](https://img.shields.io/badge/-support%20author-red)](https://bit.ly/3sX2oMk)

This is a set of tools to work with OBB files from Android version of LCS.

## obbdec_lcs
This is a tool for unpacking OBB files. To use it simply drag n' drop OBB file onto obbdec_lcs.exe or provide a path to OBB in the commandline. The files would be extracted into a directory named after OBB file. For extracted files to be named correctly, dictionary.txt must be put in the same directory as either OBB file or obbdec_lcs.exe (dictionary.txt is supplied with obb_lcs).

## makeobb_lcs
This tool can make an OBB file. To use it simply drag n' drop a directory you want packaged into OBB or provide its path through the commandline. After OBB creation is done the tool would also create a txt with list of files inside created OBB. You can use that to make your own dictionary.txt in case your OBB has some custom files (if you'd ever need unpacking it, that is).