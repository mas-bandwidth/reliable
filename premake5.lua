
solution "reliable"
    kind "ConsoleApp"
    language "C"
    configurations { "Debug", "Release" }
    if not os.istarget "windows" then
        includedirs { ".", "/usr/local/include" }       -- for clang scan-build only. for some reason it needs this to work =p
        targetdir "bin/"  
    end
    rtti "Off"
    warnings "Extra"
    flags { "FatalWarnings" }
    staticruntime "On"
    floatingpoint "Fast"
    filter "configurations:Debug"
        symbols "On"
        defines { "RELIABLE_DEBUG", "RELIABLE_ENABLE_TESTS" }
    filter "configurations:Release"
        symbols "Off"
        optimize "Speed"
        defines { "RELIABLE_RELEASE", "RELIABLE_ENABLE_TESTS" }
        
project "test"
    files { "test.cpp", "reliable.c"  }

project "example"
    files { "example.c", "reliable.c" }

project "soak"
    files { "soak.c", "reliable.c" }

project "stats"
    files { "stats.c", "reliable.c" }

project "fuzz"
    files { "fuzz.c", "reliable.c" }

newaction
{
    trigger     = "clean",

    description = "Clean all build files and output",

    execute = function ()

        files_to_delete = 
        {
            "Makefile",
            "*.make",
            "*.txt",
            "*.zip",
            "*.tar.gz",
            "*.db",
            "*.opendb",
            "*.vcproj",
            "*.vcxproj",
            "*.vcxproj.user",
            "*.vcxproj.filters",
            "*.sln",
            "*.xcodeproj",
            "*.xcworkspace"
        }

        directories_to_delete = 
        {
            "obj",
            "ipch",
            "bin",
            ".vs",
            "Debug",
            "Release",
            "release",
            "cov-int",
            "docs",
            "xml"
        }

        for i,v in ipairs( directories_to_delete ) do
          os.rmdir( v )
        end

        if not os.ishost "windows" then
            os.execute "find . -name .DS_Store -delete"
            for i,v in ipairs( files_to_delete ) do
              os.execute( "rm -f " .. v )
            end
        else
            for i,v in ipairs( files_to_delete ) do
              os.execute( "del /F /Q  " .. v )
            end
        end

    end
}
