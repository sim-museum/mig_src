# Mig Alley — native Linux (SDL2) port: CMake + Ninja build (INCREMENTAL).
#
# This is an ADDITIVE build path. `port/rebuild.sh` remains the canonical,
# always-working from-scratch builder and is NOT replaced by this file — if the
# CMake build ever misbehaves, fall back to it. Everything here is a faithful
# reproduction of rebuild.sh's semantics (same flag set, same five object groups
# with their different per-group include/prelude flags, same build-set lists in
# port/lists/, same nasm modules, same link line).
#
# Why: rebuild.sh recompiles all ~270 TUs on every change (~minutes). Ninja +
# CMake's -MD/-MF header dependency scanning rebuilds only what actually changed.
#
#   cmake -S . -B build -G Ninja && ninja -C build      -> build/wmig
#   cmake -S . -B build-asan -G Ninja -DMA_ASAN=ON && ninja -C build-asan
#
# NOTE on the "no incremental builds" warning in rebuild.sh's header: it exists
# because a *header* edit (notably SRC/compat/ddraw_legacy.h, which is inlined
# into many TUs) changes inline/weak bodies in every TU that included it, and
# `-Wl,--allow-multiple-definition` will happily keep whichever copy it saw
# first — so a partial rebuild could silently link a stale inline body. Ninja
# fixes exactly that: every TU that #includes the edited header is recompiled,
# because the compiler-emitted depfile lists it. See port/PORTING.md.
cmake_minimum_required(VERSION 3.16)

# Load-bearing: must be set BEFORE project() enables the ASM_NASM language,
# otherwise nasm is invoked without `-f elf32` and emits 64-bit objects.
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)

project(MigAlley CXX ASM_NASM)

# rebuild.sh passes NO -O flag, i.e. -O0. Do not let CMake inject -O3/-g behind
# our back: an empty build type adds no flags and keeps the two builds bit-for-bit
# comparable. (Setting CMAKE_BUILD_TYPE=Release WILL change codegen — don't,
# unless you are deliberately A/B-ing optimisation.)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "" CACHE STRING "Empty = faithful to port/rebuild.sh (-O0)" FORCE)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)

set(MA_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(MA_SRC  ${MA_ROOT}/SRC)

option(MA_ASAN "AddressSanitizer diagnostic build (mirrors ASAN=1 port/rebuild.sh)" OFF)

# ---------------------------------------------------------------------------
# The COMMON flag set — must stay identical to rebuild.sh's $COMMON.
# Every option is scoped to C/CXX with a generator expression so that NOTHING
# leaks onto the nasm command line (nasm would choke on -fpermissive etc).
# ---------------------------------------------------------------------------
add_library(ma_flags INTERFACE)

target_compile_options(ma_flags INTERFACE
    $<$<COMPILE_LANGUAGE:C,CXX>:-m32>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fno-pie>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fpermissive>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fno-strict-aliasing>
    # The engine uses the MSVC idiom of passing *(T*)NULL and testing `if (&ref)`
    # in the callee; GCC would delete those guards.
    $<$<COMPILE_LANGUAGE:C,CXX>:-fno-delete-null-pointer-checks>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fcommon>
    # LOAD-BEARING: matches MSVC /Zp1. Every packed binary game-file layout and
    # every inline-asm struct offset depends on it. Never drop this.
    $<$<COMPILE_LANGUAGE:C,CXX>:-fpack-struct=1>
    $<$<COMPILE_LANGUAGE:C,CXX>:-w>
)
target_compile_definitions(ma_flags INTERFACE
    $<$<COMPILE_LANGUAGE:C,CXX>:NDEBUG>
    $<$<COMPILE_LANGUAGE:C,CXX>:FF_LINUX>
    $<$<COMPILE_LANGUAGE:C,CXX>:MA_LINUX>
    $<$<COMPILE_LANGUAGE:C,CXX>:_LINUX>
    $<$<COMPILE_LANGUAGE:C,CXX>:stricmp=strcasecmp>
    $<$<COMPILE_LANGUAGE:C,CXX>:strnicmp=strncasecmp>
    $<$<COMPILE_LANGUAGE:C,CXX>:strcmpi=strcasecmp>
    # S57: source-tree root for the runtime RESOURCE.H / F_GRAFIX.G symbol tables
    # (IDS_* caption resolution + FIL_* art equates; ma_dlgtmpl.cpp). $MA_RC_DIR
    # overrides at runtime; absent both, resolution degrades to DLGINIT literals.
    $<$<COMPILE_LANGUAGE:C,CXX>:MA_SRC_DIR="${MA_SRC}">
)
# Order matters: compat, then H, then MFC — identical to $COMMON. The per-OCX
# -ISRC/R<xxx> directories are appended AFTER these on the individual OCX
# targets (see ma_ocx_target below), exactly as rebuild.sh appends $inc.
target_include_directories(ma_flags INTERFACE ${MA_SRC}/compat ${MA_SRC}/H ${MA_SRC}/MFC)

if(MA_ASAN)
    target_compile_options(ma_flags INTERFACE
        $<$<COMPILE_LANGUAGE:C,CXX>:-fsanitize=address>
        $<$<COMPILE_LANGUAGE:C,CXX>:-fsanitize-recover=address>
        $<$<COMPILE_LANGUAGE:C,CXX>:-g>
        $<$<COMPILE_LANGUAGE:C,CXX>:-fno-omit-frame-pointer>)
endif()

# ---------------------------------------------------------------------------
# Build-set lists (port/lists/, committed). Read at CONFIGURE time; touching a
# list re-runs cmake automatically thanks to CMAKE_CONFIGURE_DEPENDS below.
# ---------------------------------------------------------------------------
function(ma_read_list out file)
    set(path ${MA_ROOT}/port/lists/${file})
    if(NOT EXISTS ${path})
        message(FATAL_ERROR "missing build-set list: ${path} (regenerate with port/gen_buildset.sh)")
    endif()
    file(STRINGS ${path} lines)
    set(res "")
    foreach(l ${lines})
        string(STRIP "${l}" l)
        if(l)
            list(APPEND res "${l}")
        endif()
    endforeach()
    set(${out} ${res} PARENT_SCOPE)
endfunction()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${MA_ROOT}/port/lists/sa_ok.txt
    ${MA_ROOT}/port/lists/mfc_ok.txt
    ${MA_ROOT}/port/lists/mfc2_ok.txt)

# Resolve to absolute paths, drop anything that doesn't exist on disk (rebuild.sh
# does `[ -f "$f" ] && emit`), de-duplicate, and sort by basename so the final
# link order matches rebuild.sh's per-directory `*.o` glob as closely as possible
# (relevant because of -Wl,--allow-multiple-definition).
function(ma_resolve out)
    set(res "")
    foreach(f ${ARGN})
        if(EXISTS ${MA_ROOT}/${f})
            list(APPEND res ${MA_ROOT}/${f})
        elseif(EXISTS ${MA_SRC}/MFC/${f})
            # documented quirk: mfc2_ok.txt holds BARE filenames, resolved under SRC/MFC/
            list(APPEND res ${MA_SRC}/MFC/${f})
        else()
            message(WARNING "build set references a missing file, skipping: ${f}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES res)
    list(SORT res COMPARE FILE_BASENAME)
    set(${out} ${res} PARENT_SCOPE)
endfunction()

# ---------------------------------------------------------------------------
# obj/ — the 15 game-module unity aggregators + the compat runtime .cpps.
# Plain "game" mode: $COMMON only, no forced prelude.
# ---------------------------------------------------------------------------
ma_resolve(MA_OBJ_SRC
    SRC/3D/_3D.CPP SRC/AI/_AI.CPP SRC/AIRCRAFT/_AIRC.CPP SRC/BFIELDS/_BFIE.CPP
    SRC/COMMS/_COMM.CPP SRC/FILES/_FILE.CPP SRC/GENERAL/_GENE.CPP
    SRC/GRAPHICS/_GRAP.CPP SRC/HARDWARE/_HARD.CPP SRC/INPUT/_INPU.CPP
    SRC/MATH/_MATH.CPP SRC/MISSMAN/_MISS.CPP SRC/MODEL/_MODE.CPP
    SRC/MOVECODE/_MOVE.CPP SRC/TEXT/_TEXT.CPP
    SRC/compat/bob_main.cpp     SRC/compat/bob_resources.cpp SRC/compat/bob_stubs.cpp
    SRC/compat/bob_threads.cpp  SRC/compat/bob_video.cpp     SRC/compat/cstring_impl.cpp
    SRC/compat/ddraw_stubs.cpp  SRC/compat/miles_ail_stub.cpp SRC/compat/ma_openal.cpp
    SRC/compat/ma_music.cpp     SRC/compat/port_link_stubs.cpp SRC/compat/ma_dlgitem.cpp
    SRC/compat/ma_gdi.cpp
    SRC/compat/ma_acmi.cpp
    SRC/compat/ma_keybind.cpp       SRC/compat/ma_dlgtmpl.cpp     SRC/compat/ma_eventsink.cpp
    SRC/compat/ma_d3d_device.cpp SRC/compat/ma_d3d_exec.cpp SRC/compat/ma_help.cpp)
add_library(ma_obj OBJECT ${MA_OBJ_SRC})
target_link_libraries(ma_obj PRIVATE ma_flags)

# ---------------------------------------------------------------------------
# obj2/ — standalone game TUs not pulled into any unity ("game" mode), plus
# DEBRIEF which needs the MFC prelude for its unity header order.
# ---------------------------------------------------------------------------
ma_read_list(SA_OK sa_ok.txt)
ma_resolve(MA_OBJ2_SRC ${SA_OK}
    SRC/MODEL/ACMAI.CPP    SRC/MOVECODE/AUTOMOVE.CPP SRC/MODEL/CDATMOS.CPP
    SRC/MODEL/CDF94A.CPP   SRC/3D/IMAGEMAP.CPP
    SRC/INPUT/KEYSTUB.CPP  SRC/MISSMAN/MISSINIT.CPP  SRC/MISSMAN/NODEREV.CPP)
add_library(ma_obj2 OBJECT ${MA_OBJ2_SRC})
target_link_libraries(ma_obj2 PRIVATE ma_flags)

# ---------------------------------------------------------------------------
# The MFC prelude ("mfc" mode). SHELL: keeps the `-include <file>` pairs intact
# and stops CMake de-duplicating them; order is significant.
# ---------------------------------------------------------------------------
set(MA_MFC_PRELUDE "SHELL:-include stdafx.h" "SHELL:-include _mfc.h")

# DEBRIEF is compiled in mfc mode but linked with the obj2 group (rebuild.sh).
ma_resolve(MA_DEBRIEF_SRC SRC/MISSMAN/DEBRIEF.CPP)
add_library(ma_debrief OBJECT ${MA_DEBRIEF_SRC})
target_link_libraries(ma_debrief PRIVATE ma_flags)
target_compile_options(ma_debrief PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${MA_MFC_PRELUDE}>)

# objmfc/ — MFC fragments, batch 1 (per-fragment "approach B" compile).
ma_read_list(MFC_OK mfc_ok.txt)
ma_resolve(MA_MFC_SRC ${MFC_OK} SRC/MFC/FULLPANE.CPP)
add_library(ma_mfc OBJECT ${MA_MFC_SRC})
target_link_libraries(ma_mfc PRIVATE ma_flags)
target_compile_options(ma_mfc PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${MA_MFC_PRELUDE}>)

# objmfc2/ — MFC fragments, batch 2. NOTE the documented quirk: mfc2_ok.txt holds
# BARE filenames (RDIALLOG.CPP, TITLEBAR.CPP, ...) which must be resolved under
# SRC/MFC/ — ma_resolve() does that. Skipping them silently breaks the link with
# `RDialog::ChildDialClosed` undefined.
ma_read_list(MFC2_OK mfc2_ok.txt)
ma_resolve(MA_MFC2_SRC ${MFC2_OK} LISTBX.CPP MSCTLBR.CPP)
add_library(ma_mfc2 OBJECT ${MA_MFC2_SRC})
target_link_libraries(ma_mfc2 PRIVATE ma_flags)
target_compile_options(ma_mfc2 PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${MA_MFC_PRELUDE}>)

# ---------------------------------------------------------------------------
# objole/ — the hosted-OCX build modes. THE CRUX: each Rowan OCX project
# (RLISTBOX / RSTATIC / RBUTTON / RCOMBO / REDIT) ships its OWN copies of
# identically-named headers (stdafx.h, resource.h, ...). They MUST NOT be on a
# shared include path or they collide; so each mode gets its own -ISRC/R<xxx>
# APPENDED AFTER the common -I set, plus `-include afxctl.h` ahead of the normal
# MFC prelude. Seven modes total (ole, olestatic, olebutton, olecombo, oleedit,
# oleredtbt, rtabs - S60).
# ---------------------------------------------------------------------------
function(ma_ocx_target name incdir)
    ma_resolve(srcs ${ARGN})
    add_library(${name} OBJECT ${srcs})
    target_link_libraries(${name} PRIVATE ma_flags)
    # appended after ma_flags' compat/H/MFC, matching `g++ $COMMON $inc`
    target_include_directories(${name} PRIVATE ${MA_SRC}/${incdir})
    target_compile_options(${name} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:SHELL:-include afxctl.h>
        $<$<COMPILE_LANGUAGE:CXX>:${MA_MFC_PRELUDE}>)
endfunction()

ma_ocx_target(ma_ole       RLISTBOX SRC/compat/ma_olecontrol.cpp SRC/RLISTBOX/RLISTBXC.CPP
                                    SRC/RLISTBOX/RSCRLBAR.CPP    SRC/RLISTBOX/PICTURE.CPP)
ma_ocx_target(ma_olestatic RSTATIC  SRC/compat/ma_olestatic.cpp  SRC/RSTATIC/RSTATICC.CPP)
ma_ocx_target(ma_olebutton RBUTTON  SRC/compat/ma_olebutton.cpp  SRC/RBUTTON/RBUTTONC.CPP)
ma_ocx_target(ma_oleradio  RRADIO   SRC/compat/ma_oleradio.cpp   SRC/RRADIO/RRADIOC.CPP)   # S136 (PO-28)
ma_ocx_target(ma_olescroll RSCRLBAR SRC/compat/ma_olescroll.cpp  SRC/RSCRLBAR/RSCRLBRC.CPP) # S140
ma_ocx_target(ma_olespin   RSPINBUT SRC/compat/ma_olespin.cpp    SRC/RSPINBUT/RSPINBTC.CPP) # S170
ma_ocx_target(ma_olecombo  RCOMBO   SRC/compat/ma_olecombo.cpp   SRC/RCOMBO/RCOMBOC.CPP)
ma_ocx_target(ma_oleedit   REDIT    SRC/compat/ma_oleedit.cpp    SRC/REDIT/REDITCTL.CPP)
ma_ocx_target(ma_oleredtbt REDTBT   SRC/compat/ma_oleredtbt.cpp  SRC/REDTBT/REDTBTC.CPP)
ma_ocx_target(ma_oletabs   RTABS    SRC/compat/ma_oletabs.cpp    SRC/RTABS/RTABSCTL.CPP)

# ---------------------------------------------------------------------------
# wmig — the link. Object groups are listed in rebuild.sh's order
# (obj, obj2, objmfc, objmfc2, objole) because --allow-multiple-definition makes
# the FIRST definition seen win.
# ---------------------------------------------------------------------------
add_executable(wmig
    ${MA_SRC}/MATH/matrasm.nasm
    ${MA_SRC}/GRAPHICS/ma_xasm.nasm
    $<TARGET_OBJECTS:ma_obj>
    $<TARGET_OBJECTS:ma_debrief>
    $<TARGET_OBJECTS:ma_obj2>
    $<TARGET_OBJECTS:ma_mfc>
    $<TARGET_OBJECTS:ma_mfc2>
    $<TARGET_OBJECTS:ma_ole>
    $<TARGET_OBJECTS:ma_olestatic>
    $<TARGET_OBJECTS:ma_olebutton>
    $<TARGET_OBJECTS:ma_oleradio>
    $<TARGET_OBJECTS:ma_olescroll>
    $<TARGET_OBJECTS:ma_olespin>
    $<TARGET_OBJECTS:ma_olecombo>
    $<TARGET_OBJECTS:ma_oleedit>
    $<TARGET_OBJECTS:ma_oleredtbt>
    $<TARGET_OBJECTS:ma_oletabs>)
set_target_properties(wmig PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    LINKER_LANGUAGE CXX)
target_link_options(wmig PRIVATE -m32 -no-pie -Wl,--allow-multiple-definition)
if(MA_ASAN)
    target_link_options(wmig PRIVATE -fsanitize=address)
endif()
# S201: X11 for the non-fatal X error handler (bob_video.cpp). Xlib's default handler calls
# exit(), which ran the static destructors over a half-built 3D world and crashed there -- the
# "gun camera on -> 3D screen -> crash" the PO hit. SDL already pulls X11 in; we now name it.
target_link_libraries(wmig PRIVATE SDL2 GL openal fluidsynth pthread m X11)

# Convenience: `ninja install-wmig` drops the binary where rebuild.sh puts it,
# so the existing port/*.sh harnesses (asan_all.sh, stress_launch.sh, ...) which
# expect /tmp/wmig keep working against a Ninja-built binary.
add_custom_target(install-wmig
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:wmig> $<IF:$<BOOL:${MA_ASAN}>,/tmp/wmig-asan,/tmp/wmig>
    DEPENDS wmig
    COMMENT "Copying wmig to the path port/*.sh harnesses expect")

list(LENGTH MA_OBJ_SRC n1)
list(LENGTH MA_OBJ2_SRC n2)
list(LENGTH MA_MFC_SRC n3)
list(LENGTH MA_MFC2_SRC n4)
math(EXPR MA_TU_COUNT "${n1} + ${n2} + ${n3} + ${n4} + 1 + 12")
message(STATUS "MiG Alley: ${MA_TU_COUNT} C++ TUs + 2 nasm modules"
               " (obj=${n1} obj2=${n2}+DEBRIEF mfc=${n3} mfc2=${n4} ole=12)"
               " ASAN=${MA_ASAN}")
