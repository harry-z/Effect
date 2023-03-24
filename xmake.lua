add_rules("mode.debug", "mode.release")
set_languages("c11", "c++17")
add_defines("UNICODE", "_UNICODE")

target("SimpleHLSL")
    set_kind("shared")
    add_defines("SIMPLE_HLSL_EXPORT_SYMBOL")
    add_files("Effects/SimpleHLSL/*.cpp")
    add_deps("EffectMain")
target_end()

target("EnvMap")
    set_kind("shared")
    add_defines("ENV_MAP_EXPORT_SYMBOL")
    add_files("Effects/EnvMap/*.cpp")
    add_deps("EffectMain")
target_end()

target("EffectMain")
    set_kind("shared")
    add_defines("EFFECT_EXPORT_SYMBOL")
    add_includedirs("Libs/jpeg")
    add_syslinks("user32", "d3d11", "dxgi", "dxguid", "d3dcompiler")
    add_files("EffectMain.cpp", "Framework.cpp", "Effect.cpp", "ResourceLoader.cpp", "Libs/jpeg/jpgd.cpp")
target_end()

target("Effect")
    set_kind("binary")
    add_files("main.cpp")
    add_deps("EffectMain")
    after_link(
        function (target)
            print("copying shaders ...")
            os.mkdir("$(buildir)/$(plat)/$(arch)/$(mode)/Shaders")
            os.cp("$(projectdir)/Shaders/*.*", "$(buildir)/$(plat)/$(arch)/$(mode)/Shaders")
            print("finish.")
        end
    )
target_end()

--
-- If you want to known more usage about xmake, please see https://xmake.io
--
-- ## FAQ
--
-- You can enter the project directory firstly before building project.
--
--   $ cd projectdir
--
-- 1. How to build project?
--
--   $ xmake
--
-- 2. How to configure project?
--
--   $ xmake f -p [macosx|linux|iphoneos ..] -a [x86_64|i386|arm64 ..] -m [debug|release]
--
-- 3. Where is the build output directory?
--
--   The default output directory is `./build` and you can configure the output directory.
--
--   $ xmake f -o outputdir
--   $ xmake
--
-- 4. How to run and debug target after building project?
--
--   $ xmake run [targetname]
--   $ xmake run -d [targetname]
--
-- 5. How to install target to the system directory or other output directory?
--
--   $ xmake install
--   $ xmake install -o installdir
--
-- 6. Add some frequently-used compilation flags in xmake.lua
--
-- @code
--    -- add debug and release modes
--    add_rules("mode.debug", "mode.release")
--
--    -- add macro defination
--    add_defines("NDEBUG", "_GNU_SOURCE=1")
--
--    -- set warning all as error
--    set_warnings("all", "error")
--
--    -- set language: c99, c++11
--    set_languages("c99", "c++11")
--
--    -- set optimization: none, faster, fastest, smallest
--    set_optimize("fastest")
--
--    -- add include search directories
--    add_includedirs("/usr/include", "/usr/local/include")
--
--    -- add link libraries and search directories
--    add_links("tbox")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add system link libraries
--    add_syslinks("z", "pthread")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--

