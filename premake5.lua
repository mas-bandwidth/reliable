
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
    staticruntime "On"
    floatingpoint "Fast"
    filter "configurations:Debug"
        symbols "On"
        defines { "RELIABLE_DEBUG" }
    filter "configurations:Release"
        symbols "Off"
        optimize "Speed"
        defines { "RELIABLE_RELEASE" }
        
project "test"
    files { "test.cpp" }

project "soak"
    files { "soak.c", "reliable.c" }

project "stats"
    files { "stats.c", "reliable.c" }

project "fuzz"
    files { "fuzz.c", "reliable.c" }

if os.ishost "windows" then

    -- Windows

    newaction
    {
        trigger     = "solution",
        description = "Create and open the reliable solution",
        execute = function ()
            os.execute "premake5 vs2019"
            os.execute "start reliable.sln"
        end
    }

else

    -- MacOSX and Linux.
    
    newaction
    {
        trigger     = "test",
        description = "Build and run all unit tests",
        execute = function ()
            os.execute "test ! -e Makefile && premake5 gmake"
            if os.execute "make -j32 test" then
                os.execute "./bin/test"
            end
        end
    }

    newaction
    {
        trigger     = "soak",
        description = "Build and run soak test",
        execute = function ()
            os.execute "test ! -e Makefile && premake5 gmake"
            if os.execute "make -j32 soak" then
                os.execute "./bin/soak"
            end
        end
    }

    newaction
    {
        trigger     = "stats",
        description = "Build and run stats example",
        execute = function ()
            os.execute "test ! -e Makefile && premake5 gmake"
            if os.execute "make -j32 stats" then
                os.execute "./bin/stats"
            end
        end
    }

    newaction
    {
        trigger     = "fuzz",
        description = "Build and run fuzz test",
        execute = function ()
            os.execute "test ! -e Makefile && premake5 gmake"
            if os.execute "make -j32 fuzz" then
                os.execute "./bin/fuzz"
            end
        end
    }

    newaction
    {
        trigger     = "loc",
        description = "Count lines of code",
        execute = function ()
            os.execute "wc -l *.h *.c *.cpp"
        end
    }

end

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
