
--======================================================================================
function make_library_project (name) 

    project.name = name
    project.bindir = "../../../bin"
    project.libdir = "../../../bin"

    project.configs = { "Debug", "Release", "Release32" }

    addoption ("disable-alsa",    "Force disable ALSA")
    addoption ("disable-jack",    "Force disable jack-audio-connection-kit")
    addoption ("disable-xshm",    "Force disable SHM support for Xorg")
    addoption ("enable-lash",     "Enable LASH support (default disabled)")
    addoption ("enable-xinerama", "Enable Xinerama support (default disabled)")
    addoption ("enable-opengl",   "Enable OpenGL support (default disabled)")

    package = newpackage()
    package.name = project.name
    package.target = project.name
    package.kind = "lib"
    package.language = "c++"
    package.linkflags = { "static-runtime" }
    package.objdir = project.bindir .. "/intermediate/"
    
    package.config["Debug"].target            = project.name .. "_debug"
    package.config["Debug"].objdir            = package.objdir .. "/" .. project.name .. "Debug"
    package.config["Debug"].defines           = { "DEBUG=1", "_DEBUG=1" }
    package.config["Debug"].buildoptions      = { "-O0 -g -Wall" }

    package.config["Release"].target          = project.name
    package.config["Release"].objdir          = package.objdir .. "/" .. project.name .. "Release"
    package.config["Release"].defines         = { "NDEBUG=1" }
    package.config["Release"].buildoptions    = { "-O2 -pipe -fvisibility=hidden -Wall" }
    package.config["Release"].buildflags      = { "no-symbols", "no-frame-pointer" }

    package.config["Release32"].target        = project.name .. "32"
    package.config["Release32"].objdir        = package.objdir .. "/" .. project.name .. "Release32"
    package.config["Release32"].defines       = { "NDEBUG=1" }
    package.config["Release32"].linkoptions   = { "-melf_i386" }
    package.config["Release32"].buildoptions  = { "-m32 -O2 -pipe -fvisibility=hidden -Wall" }
    package.config["Release32"].buildflags    = { "no-symbols", "no-frame-pointer" }

    package.defines = { "LINUX=1" }

    if (os.fileexists ("/usr/include/X11/extensions/XShm.h") and not options["disable-xshm"]) then
        table.insert (package.defines, "JUCE_USE_XSHM=1")
    end

    if (os.findlib ("Xinerama") and options["enable-xinerama"]) then
        table.insert (package.defines, "JUCE_USE_XINERAMA=1")
    end

    if (os.findlib ("GL") and os.findlib ("GLU") and options["enable-opengl"]) then
        table.insert (package.defines, "JUCE_OPENGL=1")
    end

    if (os.findlib ("jack") and not options["disable-jack"]) then
        table.insert (package.defines, "JUCE_JACK=1")
    end

    if (os.findlib ("asound") and not options["disable-alsa"]) then
        table.insert (package.defines, "JUCE_ALSA=1")
    end

    if (os.findlib ("lash") and options["enable-lash"]) then
        table.insert (package.defines, "JUCE_LASH=1")
        table.insert (package.includepaths, "/usr/include/lash-1.0")
    end

    package.includepaths = { 
        "/usr/include",
        "/usr/include/freetype2",
        "../../"
    }

    return package
end

--======================================================================================
function make_plugin_project (name, kind, do32bit, doAmalgama) 

    overrideAmalgama = true
    override32bit = true
    finalRelease = true

    if (overrideAmalgama) then
        doAmalgama = true
    end
    if (override32bit) then
        do32bit = false
    end

    libpath = "../../../../bin"
    if (doAmalgama and finalRelease) then
        libpath = "../../bin"
    end

    addoption ("disable-alsa",    "Force disable ALSA")
    addoption ("disable-jack",    "Force disable jack-audio-connection-kit")
    addoption ("disable-xshm",    "Force disable SHM support for Xorg")
    addoption ("enable-lash",     "Enable LASH support (default disabled)")
    addoption ("enable-xinerama", "Enable Xinerama support (default disabled)")
    addoption ("enable-opengl",   "Enable OpenGL support (default disabled)")

    project.name = name
    project.bindir = libpath
    project.libdir = libpath

    if (do32bit) then
        project.configs = { "Debug", "Release", "Release32" }
    else
        project.configs = { "Debug", "Release" }
    end

    package = newpackage()
    package.name = project.name
    package.target = package.name
    package.kind = kind
    package.language = "c++"
    package.linkflags = { "static-runtime" }
    package.objdir = project.bindir .. "/intermediate"

    if (package.kind == "dll") then
        package.targetprefix = ""
        package.targetextension = "so"
    end

    package.config["Debug"].target            = package.name .. "_debug"
    package.config["Debug"].objdir            = package.objdir .. "/" .. package.name .. "Debug"
    package.config["Debug"].defines           = { "DEBUG=1", "_DEBUG=1" }
    package.config["Debug"].buildoptions      = { "-O0 -g -Wall" }
    package.config["Debug"].libpaths          = { "/usr/X11R6/lib/", "/usr/lib/" }
    if (not doAmalgama) then
        package.config["Debug"].links         = { "juce_debug" }
    end

    package.config["Release"].target          = package.name
    package.config["Release"].objdir          = package.objdir .. "/" .. package.name .. "Release"
    package.config["Release"].defines         = { "NDEBUG=1" }
    package.config["Release"].buildoptions    = { "-O2 -pipe -fvisibility=hidden -Wall" }
    package.config["Release"].buildflags      = { "no-symbols", "no-frame-pointer" }
    package.config["Release"].libpaths        = { "/usr/X11R6/lib/", "/usr/lib/" }
    if (not doAmalgama) then
        package.config["Release"].links       = { "juce" }
    end

    if (do32bit) then
        package.config["Release32"].target        = package.name
        package.config["Release32"].objdir        = package.objdir .. "/" .. package.name .. "Release32"
        package.config["Release32"].defines       = { "NDEBUG=1" }
        package.config["Release32"].buildoptions  = { "-m32 -O2 -pipe -fvisibility=hidden -Wall" }
        package.config["Release32"].buildflags    = { "no-symbols", "no-frame-pointer" }
        package.config["Release32"].libpaths      = { "/usr/X11R6/lib32/", "/usr/lib32/" }
        package.config["Release32"].linkoptions   = { "-melf_i386" }
        if (not doAmalgama) then
            package.config["Release32"].links     = { "juce32" }
        end
    end

    package.defines = { "LINUX=1" }
    if (doAmalgama) then
        table.insert (package.defines, "JUCETICE_USE_AMALGAMA=1")
    end

    package.links = { "freetype", "pthread", "rt", "X11" }

    if (os.fileexists ("/usr/include/X11/extensions/XShm.h") and not options["disable-xshm"]) then
        table.insert (package.defines, "JUCE_USE_XSHM=1")
        table.insert (package.links, "Xext")
    end

    if (os.findlib ("GL") and os.findlib ("GLU") and options["enable-opengl"]) then
        table.insert (package.defines, "JUCE_OPENGL=1")
        table.insert (package.links, "GL")
        table.insert (package.links, "GLU")
    end

    if (os.findlib ("Xinerama") and options["enable-xinerama"]) then
        table.insert (package.defines, "JUCE_USE_XINERAMA=1")
        table.insert (package.links, "Xinerama")
        table.insert (package.links, "Xext")
    end

    if (os.findlib ("asound") and not options["disable-alsa"]) then
        table.insert (package.defines, "JUCE_ALSA=1")
        table.insert (package.links, "asound")
    end

    if (os.findlib ("jack") and not options["disable-jack"]) then
        table.insert (package.defines, "JUCE_JACK=1")
        table.insert (package.links, "jack")
    end

    if (os.findlib ("lash") and options["enable-lash"]) then
        table.insert (package.defines, "JUCE_LASH=1")
        table.insert (package.links, "lash")
        table.insert (package.includepaths, "/usr/include/lash-1.0")
    end

    package.libpaths = { libpath }

    package.includepaths = {
        "/usr/include",
        "/usr/include/freetype2",
        "../../../../juce",
        "../../../../juce/src",
        "../../../../wrapper",
        "../../wrapper",
        "../../../../vst/vstsdk2.3",
        "../../../../vst/vstsdk2.3/source/common",
        "../../../../vstsdk2.3",
        "../../../../vstsdk2.3/source/common",
        "../../vst/vstsdk2.3",
        "../../vst/vstsdk2.3/source/common",
        "../../vstsdk2.3",
        "../../vstsdk2.3/source/common",
        "/usr/include/vstsdk2.3",
        "/usr/include/vst",
        "../../src"
    }

    if (doAmalgama and finalRelease) then
        package.excludes = {
            "../../src/juce_amalgamated.cpp",
            "../../src/juce_amalgamated.h"
        }
    end

    return package
end

--======================================================================================
function configure_jost_libraries (package, standalone)

    addoption ("vst-path",        "Specify where your 'vstsdk2.3' directory resides")
    addoption ("disable-vst",     "Force disable VST (2.3) support")
    addoption ("disable-ladspa",  "Force disable LADSPA support")
    addoption ("disable-dssi",    "Force disable DSSI support")

    if (os.fileexists ("/usr/include/vst/audioeffectx.h") and not options["disable-vst"]) then
        table.insert (package.defines, "JOST_USE_VST=1")
        if (options["vst-path"]) then
            table.insert (package.includepaths, options["vst-path"]);
        end
    else
        table.insert (package.defines, "JOST_USE_VST=0")
    end

    if (os.fileexists ("/usr/include/ladspa.h") and not options["disable-ladspa"]) then
        table.insert (package.defines, "JOST_USE_LADSPA=1")
    else
        table.insert (package.defines, "JOST_USE_LADSPA=0")
    end

    if (os.fileexists ("/usr/include/dssi.h") and not options["disable-dssi"]) then
        table.insert (package.defines, "JOST_USE_DSSI=1")
    else
        table.insert (package.defines, "JOST_USE_DSSI=0")
    end

    if (not standalone) then
        table.insert (package.defines, "JOST_VST_PLUGIN=1")
    end

    return package
end

