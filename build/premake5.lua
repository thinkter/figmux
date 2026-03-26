-- Export Compile Commands module for clangd support
require "ecc/ecc"

newoption
{
    trigger = "graphics",
    value = "OPENGL_VERSION",
    description = "version of OpenGL to build raylib against",
    allowed = {
        { "opengl11", "OpenGL 1.1" },
        { "opengl21", "OpenGL 2.1" },
        { "opengl33", "OpenGL 3.3" },
        { "opengl43", "OpenGL 4.3" },
        { "openges2", "OpenGL ES2" },
        { "openges3", "OpenGL ES3" },
        { "software", "OpenGL 1.1 Software Render" }
    },
    default = "opengl33"
}

newoption
{
    trigger = "backend",
    value = "BACKEND_TYPE",
    description = "Backend Platform to use",
    allowed = {
        { "glfw",  "GLFW" },
        { "rgfw",  "RGFW" },
        { "win32", "WIN32" },
    },
    default = "glfw"
}

newoption
{
    trigger = "wayland",
    value = "WAYLAND",
    description = "build for wayland",
    allowed = {
        { "off", "Off" },
        { "on",  "On" }
    },
    default = "off"
}

function download_progress(total, current)
    local ratio = current / total;
    ratio = math.min(math.max(ratio, 0), 1);
    local percent = math.floor(ratio * 100);
    print("Download progress (" .. percent .. "%/100%)")
end

function run_command(command)
    print(command)
    local result = os.execute(command)
    if result ~= true and result ~= 0 then
        error("Command failed: " .. command)
    end
end

function shared_library_exists(dir, host)
    if host == "windows" then
        return #os.matchfiles(path.join(dir, "ghostty-vt.dll")) > 0
    end

    if host == "macosx" then
        return #os.matchfiles(path.join(dir, "libghostty-vt*.dylib")) > 0
    end

    return #os.matchfiles(path.join(dir, "libghostty-vt.so*")) > 0
end

function check_raylib()
    os.chdir("external")
    if (os.isdir("raylib-master") == false) then
        if (not os.isfile("raylib-master.zip")) then
            print("Raylib not found, downloading from github")
            local result_str, response_code = http.download(
                "https://github.com/raysan5/raylib/archive/refs/heads/master.zip", "raylib-master.zip", {
                    progress = download_progress,
                    headers = { "From: Premake", "Referer: Premake" }
                })
        end
        print("Unzipping to " .. os.getcwd())
        zip.extract("raylib-master.zip", os.getcwd())
        os.remove("raylib-master.zip")
    end
    os.chdir("../")
end

function check_ghostty()
    os.chdir("external")
    if (os.isdir("ghostty-src") == false) then
        local archive_name = "ghostty-" .. ghostty_commit .. ".zip"
        local extracted_dir = "ghostty-" .. ghostty_commit
        if (not os.isfile(archive_name)) then
            print("Ghostty not found, downloading pinned source archive")
            http.download(
                "https://github.com/ghostty-org/ghostty/archive/" .. ghostty_commit .. ".zip",
                archive_name,
                {
                    progress = download_progress,
                    headers = { "From: Premake", "Referer: Premake" }
                }
            )
        end

        print("Unzipping Ghostty to " .. os.getcwd())
        zip.extract(archive_name, os.getcwd())
        if (os.isdir(extracted_dir) == false) then
            error("Expected extracted Ghostty directory not found: " .. extracted_dir)
        end

        if (os.isdir("ghostty-src")) then
            os.rmdir("ghostty-src")
        end
        os.rename(extracted_dir, "ghostty-src")
        os.remove(archive_name)
    end
    os.chdir("../")

    if (os.isdir(ghostty_build_dir) == false) then
        os.mkdir(ghostty_build_dir)
    end

    if (shared_library_exists(ghostty_lib_dir, os.host()) == false) then
        print("Building libghostty-vt with CMake/Zig")
        local configure = "cmake -S \"" .. ghostty_dir .. "\" -B \"" .. ghostty_build_dir .. "\""
        if (_ACTION == "vs2022") then
            configure = configure .. " -G \"Visual Studio 17 2022\" -A x64"
        end

        run_command(configure)
        run_command("cmake --build \"" .. ghostty_build_dir .. "\" --target zig_build_lib_vt")
    end
end

function build_externals()
    print("calling externals")
    check_raylib()
    check_ghostty()
end

function platform_defines()
    filter { "options:backend=glfw" }
    defines { "PLATFORM_DESKTOP" }

    filter { "options:backend=rgfw" }
    defines { "PLATFORM_DESKTOP_RGFW" }

    filter { "options:backend=win32" }
    defines { "PLATFORM_DESKTOP_WIN32" }

    filter { "options:graphics=opengl43" }
    defines { "GRAPHICS_API_OPENGL_43" }

    filter { "options:graphics=opengl33" }
    defines { "GRAPHICS_API_OPENGL_33" }

    filter { "options:graphics=opengl21" }
    defines { "GRAPHICS_API_OPENGL_21" }

    filter { "options:graphics=opengl11" }
    defines { "GRAPHICS_API_OPENGL_11" }

    filter { "options:graphics=openges3" }
    defines { "GRAPHICS_API_OPENGL_ES3" }

    filter { "options:graphics=openges2" }
    defines { "GRAPHICS_API_OPENGL_ES2" }

    filter { "options:graphics=software" }
    defines { "GRAPHICS_API_OPENGL_SOFTWARE" }

    filter { "system:macosx" }
    disablewarnings { "deprecated-declarations" }

    filter { "system:linux", "options:wayland=off" }
    defines { "_GLFW_X11" }

    filter { "system:linux", "options:wayland=on" }
    defines { "_GLFW_WAYLAND" }

    filter {}
end

-- if you don't want to download raylib, then set this to false, and set the raylib dir to where you want raylib to be pulled from, must be full sources.
downloadRaylib = true
raylib_dir = "external/raylib-master"
ghostty_commit = "bebca84668947bfc92b9a30ed58712e1c34eee1d"
ghostty_dir = "external/ghostty-src"
ghostty_build_dir = "external/ghostty-build"
ghostty_include_dir = ghostty_dir .. "/include"
ghostty_lib_dir = ghostty_dir .. "/zig-out/lib"

workspaceName = 'MyGame'
baseName = path.getbasename(path.getdirectory(os.getcwd()));

--if (baseName ~= 'raylib-quickstart') then
workspaceName = baseName
--end

if (os.isdir('build_files') == false) then
    os.mkdir('build_files')
end

if (os.isdir('external') == false) then
    os.mkdir('external')
end

workspace(workspaceName)
location "../"
configurations { "Debug", "Release" }
platforms { "x64", "x86", "ARM64" }

defaultplatform("x64")

filter "configurations:Debug"
defines { "DEBUG" }
symbols "On"

filter "configurations:Release"
defines { "NDEBUG" }
optimize "On"

filter { "configurations:Release", "action:vs*" }
linktimeoptimization "On"

filter { "platforms:x64" }
architecture "x86_64"

filter { "platforms:ARM64" }
architecture "ARM64"

filter {}

targetdir "bin/%{cfg.buildcfg}/"

if (downloadRaylib) then
    build_externals()
end

startproject(workspaceName)

project(workspaceName)
kind "ConsoleApp"
location "../"
targetdir "../bin/%{cfg.buildcfg}"

filter { "system:windows", "configurations:Release", "action:gmake*" }
kind "WindowedApp"
buildoptions { "-Wl,--subsystem,windows" }

filter { "system:windows", "configurations:Release", "action:vs*" }
kind "WindowedApp"
entrypoint "mainCRTStartup"

filter "action:vs*"
debugdir "$(SolutionDir)"

filter { "action:gmake*" } -- Uncoment if you need to force StaticLib
--          buildoptions { "-static" }
filter {}

vpaths
{
    ["Header Files/*"] = { "../include/**.h", "../include/**.hpp", "../src/**.h", "../src/**.hpp" },
    ["Source Files/*"] = { "../src/**.c", "src/**.cpp" },
    ["Windows Resource Files/*"] = { "../src/**.rc", "../src/**.ico" },
    ["Game Resource Files/*"] = { "../resources/**" },
}

files { "../src/**.c", "../src/**.cpp", "../src/**.h", "../src/**.hpp", "../include/**.h", "../include/**.hpp" }

filter { "system:windows", "action:vs*" }
files { "../src/*.rc", "../src/*.ico" }
files { "../resources/**" }

filter {}

includedirs { "../src" }
includedirs { "../include" }
includedirs { ghostty_include_dir }
libdirs { ghostty_lib_dir }

links { "raylib" }
links { "ghostty-vt" }

cdialect "C17"
cppdialect "C++17"

includedirs { raylib_dir .. "/src" }

flags { "ShadowedVariables" }
platform_defines()

filter "action:vs*"
defines { "_WINSOCK_DEPRECATED_NO_WARNINGS", "_CRT_SECURE_NO_WARNINGS" }
dependson { "raylib" }
links { "raylib.lib" }
characterset("Unicode")
buildoptions { "/Zc:__cplusplus" }

filter "system:windows"
defines { "_WIN32" }
links { "winmm", "gdi32", "opengl32" }
libdirs { "../bin/%{cfg.buildcfg}" }
postbuildcommands {
    "{COPYFILE} " .. path.translate(ghostty_lib_dir .. "/ghostty-vt.dll") .. " " .. path.translate("../bin/%{cfg.buildcfg}/ghostty-vt.dll")
}

filter "system:linux"
links { "pthread", "m", "dl", "rt", "util" }
linkoptions { "-Wl,-rpath,'$$ORIGIN/../../build/" .. ghostty_lib_dir .. "'" }

filter { "system:linux", "options:wayland=off" }
links { "X11" }

filter { "system:linux", "options:wayland=on" }
links { "wayland-client", "wayland-cursor", "wayland-egl", "xkbcommon" }

filter "system:macosx"
links { "OpenGL.framework", "Cocoa.framework", "IOKit.framework", "CoreFoundation.framework", "CoreAudio.framework", "CoreVideo.framework", "AudioToolbox.framework" }
linkoptions { "-Wl,-rpath,@loader_path/../../build/" .. ghostty_lib_dir }

filter {}


project "raylib"
kind "StaticLib"

platform_defines()

location "../"

language "C"
targetdir "../bin/%{cfg.buildcfg}"


filter { "options:wayland=on" }
defines { "GLFW_LINUX_ENABLE_WAYLAND=TRUE" }

filter { "options:wayland=on", "system:linux" }
prebuildcommands {
    "@echo Generating Wayland protocols...",
    -- Core Wayland & Shell
    "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/wayland.xml ../" .. raylib_dir .. "/src/wayland-client-protocol.h",
    "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/xdg-shell.xml ../" .. raylib_dir .. "/src/xdg-shell-client-protocol.h",
    "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/xdg-decoration-unstable-v1.xml ../" .. raylib_dir .. "/src/xdg-decoration-unstable-v1-client-protocol.h",

    -- Viewporter
    "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/viewporter.xml ../" .. raylib_dir .. "/src/viewporter-client-protocol.h",

    -- Relative Pointer
    "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/relative-pointer-unstable-v1.xml ../" .. raylib_dir .. "/src/relative-pointer-unstable-v1-client-protocol.h",
    -- Pointer Constraints
    "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/pointer-constraints-unstable-v1.xml ../" .. raylib_dir .. "/src/pointer-constraints-unstable-v1-client-protocol.h",

    -- Fractional Scale
    "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/fractional-scale-v1.xml ../" .. raylib_dir .. "/src/fractional-scale-v1-client-protocol.h",

    -- XDG Activation
    "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/xdg-activation-v1.xml ../" .. raylib_dir .. "/src/xdg-activation-v1-client-protocol.h",
    -- Idle Inhibit
    "@wayland-scanner client-header ../" .. raylib_dir .. "/src/external/glfw/deps/wayland/idle-inhibit-unstable-v1.xml ../" .. raylib_dir .. "/src/idle-inhibit-unstable-v1-client-protocol.h",
}
filter {}

filter "action:vs*"
defines { "_WINSOCK_DEPRECATED_NO_WARNINGS", "_CRT_SECURE_NO_WARNINGS" }
characterset("Unicode")
buildoptions { "/Zc:__cplusplus" }
filter {}

includedirs { raylib_dir .. "/src", raylib_dir .. "/src/external/glfw/include" }
vpaths
{
    ["Header Files"] = { raylib_dir .. "/src/**.h" },
    ["Source Files/*"] = { raylib_dir .. "/src/**.c" },
}
files { raylib_dir .. "/src/*.h", raylib_dir .. "/src/*.c" }

removefiles { raylib_dir .. "/src/rcore_*.c" }

filter { "system:macosx", "files:" .. raylib_dir .. "/src/rglfw.c" }
compileas "Objective-C"

filter {}
