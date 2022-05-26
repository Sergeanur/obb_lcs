workspace "obb_lcs"
	configurations
	{
		"Debug",
		"Release",
	}

	location "build"

project "obbdec_lcs"
	kind "ConsoleApp"
	language "C++"
	targetname "obbdec_lcs"
	targetdir "bin/%{cfg.buildcfg}"
	cppdialect "C++17"
	targetextension ".exe"

	includedirs { "src" }

	files { "src/obbdec_lcs.cpp" }
	files { "src/crc32.cpp" }
	files { "src/crc32.h" }
	files { "src/dictionary.cpp" }
	files { "src/dictionary.h" }
	files { "src/WadFile.cpp" }
	files { "src/WadFile.h" }
	files { "src/utils_win32.cpp" }
	files { "src/utils.h" }

	characterset ("Unicode")
	toolset ("v141_xp")

	links { "legacy_stdio_definitions" }

	filter "configurations:Debug"
		defines { "_DEBUG" }
		symbols "full"
		optimize "off"
		runtime "debug"
		editAndContinue "off"
		flags { "NoIncrementalLink" }
		staticruntime "on"

	filter "configurations:Release"
		defines { "NDEBUG" }
		symbols "on"
		optimize "speed"
		runtime "release"
		staticruntime "on"
		flags { "LinkTimeOptimization" }
		linkoptions { "/OPT:NOICF" }


project "makeobb_lcs"
	kind "ConsoleApp"
	language "C++"
	targetname "makeobb_lcs"
	targetdir "bin/%{cfg.buildcfg}"
	cppdialect "C++17"
	targetextension ".exe"

	includedirs { "src" }

	files { "src/makeobb_lcs.cpp" }
	files { "src/crc32.cpp" }
	files { "src/crc32.h" }
	files { "src/dictionary.cpp" }
	files { "src/dictionary.h" }
	files { "src/WadFile.cpp" }
	files { "src/WadFile.h" }
	files { "src/utils_win32.cpp" }
	files { "src/utils.h" }

	characterset ("Unicode")
	toolset ("v141_xp")
	
	links { "legacy_stdio_definitions" }

	filter "configurations:Debug"
		defines { "_DEBUG" }
		symbols "full"
		optimize "off"
		runtime "debug"
		editAndContinue "off"
		flags { "NoIncrementalLink" }
		staticruntime "on"

	filter "configurations:Release"
		defines { "NDEBUG" }
		symbols "on"
		optimize "speed"
		runtime "release"
		staticruntime "on"
		flags { "LinkTimeOptimization" }
		linkoptions { "/OPT:NOICF" }
