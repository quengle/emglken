cmake_minimum_required(VERSION 3.13)

project(Emglken
    VERSION 0.3.0
    LANGUAGES C CXX)

include(src/common.cmake)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
if (${CMAKE_BUILD_TYPE} STREQUAL Release)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    set(REMGLK_RS_LIB_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/target/wasm32-unknown-emscripten/release)
else()
    set(REMGLK_RS_LIB_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/target/wasm32-unknown-emscripten/debug)
endif()

add_executable(glulxe)
add_sources(glulxe "glulxe/"
    SRCS accel.c debugger.c exec.c files.c float.c funcs.c
    gestalt.c glkop.c heap.c main.c operand.c osdepend.c profile.c
    search.c serial.c string.c vm.c unixstrt.c unixautosave.c)
emglken_vm(glulxe)
target_compile_definitions(glulxe PRIVATE OS_UNIX)
target_compile_options(glulxe PRIVATE -Wall -Wmissing-prototypes -Wno-unused)
target_link_options(glulxe PRIVATE -sASYNCIFY_IGNORE_INDIRECT)

#[[
add_executable(glulxe-profiler-core)
add_sources(glulxe-profiler-core "glulxe/"
    SRCS accel.c debugger.c exec.c files.c float.c funcs.c
    gestalt.c glkop.c heap.c main.c operand.c osdepend.c profile.c
    search.c serial.c string.c vm.c unixstrt.c unixautosave.c)
emglken_vm(glulxe-profiler-core)
target_compile_definitions(glulxe-profiler-core PRIVATE OS_UNIX VM_PROFILING)
target_compile_options(glulxe-profiler-core PRIVATE -Wall -Wmissing-prototypes -Wno-unused)
]]

# Common setup for our two Bocfel builds
function(bocfel_common target)
    add_sources(${target} "garglk/terps/bocfel/"
        SRCS blorb.cpp branch.cpp dict.cpp glkautosave.cpp glkstart.cpp iff.cpp
        io.cpp mathop.cpp meta.cpp memory.cpp objects.cpp options.cpp osdep.cpp
        patches.cpp process.cpp random.cpp screen.cpp sound.cpp stack.cpp stash.cpp
        unicode.cpp util.cpp zterp.cpp)
    emglken_vm(${target})
    set_property(TARGET ${target} PROPERTY CXX_STANDARD 17)
    target_compile_definitions(${target} PRIVATE ZTERP_GLK ZTERP_GLK_BLORB
        ZTERP_GLK_NO_STDIO ZTERP_GLK_UNIX ZTERP_NO_OPTIONS
        "ZTERP_STATIC_PATCH_FILE=\"static-patches.h\"")
    target_compile_options(${target} PRIVATE -Wall)
    # Bocfel now uses exceptions, which takes a little bit to set up
    target_compile_options(${target} PRIVATE -fexceptions)
    target_link_options(${target} PRIVATE -sNO_DISABLE_EXCEPTION_CATCHING)
endfunction()

add_executable(bocfel-noz6)
bocfel_common(bocfel-noz6)
target_compile_definitions(bocfel-noz6 PRIVATE ZTERP_NO_V6)
target_link_options(bocfel-noz6 PRIVATE -sASYNCIFY_REMOVE=@${CMAKE_CURRENT_LIST_DIR}/src/asyncify-remove-list-bocfel-noz6.txt)

add_executable(bocfel)
bocfel_common(bocfel)
target_link_options(bocfel PRIVATE -sASYNCIFY_REMOVE=@${CMAKE_CURRENT_LIST_DIR}/src/asyncify-remove-list-bocfel.txt)

add_executable(git)
add_sources(git "git/"
    SRCS accel.c compiler.c gestalt.c git.c glkop.c heap.c
    memory.c opcodes.c operands.c peephole.c savefile.c saveundo.c
    search.c terp.c git_unix.c)
emglken_vm(git)
target_compile_definitions(git PRIVATE USE_DIRECT_THREADING USE_INLINE)
target_compile_options(git PRIVATE -Wall -Wno-int-conversion -Wno-pointer-sign -Wno-unused-but-set-variable)
target_link_options(git PRIVATE -sASYNCIFY_IGNORE_INDIRECT)

add_executable(hugo)
add_sources(hugo "hugo/source/"
    SRCS he.c heexpr.c hemisc.c heobject.c heparse.c heres.c herun.c heset.c stringfn.c)
# quengle: heglk.c with pictures (src/quengle-heglk.c includes it)
add_sources(hugo "hugo/heglk/"
    SRCS heglkunix.c)
add_sources(hugo "src/"
    SRCS quengle-heglk.c)
emglken_vm(hugo)
# quengle: no NO_KEYPRESS_CURSOR, whose "*" stand-in cursor lands mid-line in grid windows; no COMPILE_V25:
# the 3.x engine has the video statement (quengle-heglk.c plays it) and runs older games too
target_compile_definitions(hugo PRIVATE GLK QUENGLE_RESOURCES HUGO_INLINE=static\ inline)
target_compile_options(hugo PRIVATE -Wall -Wno-unused-but-set-variable)
target_link_options(hugo PRIVATE -sASYNCIFY_IGNORE_INDIRECT)

# quengle: magnetic scrolls (GPL-2: only in builds with QUENGLE_MAGNETIC, see HANDOFF). autosave in emu.c
# (QUENGLE), stdio mapped onto glk streams by quengle-magstdio.h
add_executable(magnetic)
add_sources(magnetic "garglk/terps/magnetic/Generic/"
    SRCS emu.c)
add_sources(magnetic "garglk/terps/magnetic/Glk/"
    SRCS glk.c)
emglken_vm(magnetic)
target_compile_definitions(magnetic PRIVATE MAGNETIC_GLKUNIX QUENGLE)
target_compile_options(magnetic PRIVATE -include ${CMAKE_CURRENT_SOURCE_DIR}/src/quengle-magstdio.h -Wno-implicit-function-declaration)

# quengle: scottfree, spatterlight's (scott adams; GPL-2: only in builds with QUENGLE_SCOTT, see HANDOFF). autosave
# in scott_meta.c (QUENGLE), stdio over glk streams by quengle-stdio.h
set(GT "garglk/terps/")
add_executable(scott)
add_sources(scott "${GT}scott/"
    SRCS scott.c bsd.c detect_game.c parser.c restore_state.c scott_actions.c scott_display.c scott_game_info.c
    scott_meta.c title_image.c
    ai_uk/c64decrunch.c ai_uk/decompress_text.c ai_uk/game_specific.c ai_uk/gremlins.c ai_uk/hulk.c
    ai_uk/line_drawing.c ai_uk/ringbuffer.c ai_uk/robin_of_sherwood.c ai_uk/sagadraw.c ai_uk/seas_of_blood.c
    saga/apple2_vector_draw.c saga/apple2detect.c saga/atari8detect.c saga/atari_8bit_vector_draw.c saga/c64_small.c
    saga/c64a8scott.c saga/saga.c saga/sagagraphics.c saga/vector_common.c
    ti994a/load_ti99_4a.c ti994a/rtpi_graphics.c ti994a/ti99_4a_terp.c ti994a/ti99_disk.c ti994a/unzip_in_mem.c)
add_sources(scott "${GT}scott/saga/" SRCS)
add_sources(scott "${GT}scott/ai_uk/" SRCS)
add_sources(scott "${GT}scott/ti994a/" SRCS)
add_sources(scott "${GT}common_sagadraw/" SRCS gm_vector.c apple2draw.c c64a8draw.c gm_artifact.c pcdraw.c)
add_sources(scott "${GT}common_utils/"
    SRCS common_file_utils.c common_utils.c companion_search.c layouttext.c memory_allocation.c randomness.c)
add_sources(scott "${GT}readwoz/" SRCS ciderpress.c woz2nib.c)
add_sources(scott "${GT}aiukgraphics/" SRCS aiukpalette.c irmak.c taylordraw.c)
add_sources(scott "${GT}decompressz80/" SRCS alkatraz.c tapetzx.c z80snap.c zxscreen.c)
add_sources(scott "${GT}common_zxscreen/" SRCS zx_title.c)
add_sources(scott "${GT}c64diskimage/" SRCS c64diskimage.c)
add_sources(scott "${GT}unp64/"
    SRCS 6502_emu.cpp exo_util.cpp unp64.cpp scanners/abuze_crunch.cpp scanners/action_packer.cpp scanners/ar.cpp
    scanners/byte_boiler.cpp scanners/caution.cpp scanners/ccs.cpp scanners/cruel.cpp scanners/eca.cpp
    scanners/exomizer.cpp scanners/expert.cpp scanners/final_super_comp.cpp scanners/intros.cpp
    scanners/master_compressor.cpp scanners/megabyte.cpp scanners/mr_cross.cpp scanners/mr_z.cpp
    scanners/pu_crunch.cpp scanners/scanners.cpp scanners/section8.cpp scanners/tbc_multicomp.cpp
    scanners/tcs_crunch.cpp scanners/time_cruncher.cpp scanners/xtc.cpp)
emglken_vm(scott)
set_property(TARGET scott PROPERTY CXX_STANDARD 17)
set_property(TARGET scott PROPERTY C_STANDARD 11)
target_compile_definitions(scott PRIVATE QUENGLE $<$<COMPILE_LANGUAGE:C>:_XOPEN_SOURCE=600>)
target_compile_options(scott PRIVATE $<$<COMPILE_LANGUAGE:C>:-include ${CMAKE_CURRENT_SOURCE_DIR}/src/quengle-stdio.h>
    --use-port=zlib -Wno-implicit-function-declaration)
target_link_options(scott PRIVATE --use-port=zlib)

# quengle: taylormade, spatterlight's (adventure soft uk: kayleth, rebel planet, temple of terror, blizzard pass…;
# GPL-2 as scott). stdio over glk streams by quengle-stdio.h
add_executable(taylor)
add_sources(taylor "${GT}taylor/"
    SRCS actions.c alkatraz_screen.c animations.c c64decrunch.c decrypttotloader.c extracommands.c extracttape.c
    gameinfo.c gameload.c graphics.c loading_screen.c parseinput.c player.c restorestate.c textoutput.c ui.c)
add_sources(taylor "${GT}common_utils/"
    SRCS common_file_utils.c common_utils.c companion_search.c layouttext.c memory_allocation.c randomness.c)
add_sources(taylor "${GT}aiukgraphics/" SRCS aiukpalette.c irmak.c taylordraw.c)
add_sources(taylor "${GT}decompressz80/" SRCS alkatraz.c tapetzx.c z80snap.c zxscreen.c)
add_sources(taylor "${GT}common_zxscreen/" SRCS zx_title.c)
add_sources(taylor "${GT}c64diskimage/" SRCS c64diskimage.c)
add_sources(taylor "${GT}unp64/"
    SRCS 6502_emu.cpp exo_util.cpp unp64.cpp scanners/abuze_crunch.cpp scanners/action_packer.cpp scanners/ar.cpp
    scanners/byte_boiler.cpp scanners/caution.cpp scanners/ccs.cpp scanners/cruel.cpp scanners/eca.cpp
    scanners/exomizer.cpp scanners/expert.cpp scanners/final_super_comp.cpp scanners/intros.cpp
    scanners/master_compressor.cpp scanners/megabyte.cpp scanners/mr_cross.cpp scanners/mr_z.cpp
    scanners/pu_crunch.cpp scanners/scanners.cpp scanners/section8.cpp scanners/tbc_multicomp.cpp
    scanners/tcs_crunch.cpp scanners/time_cruncher.cpp scanners/xtc.cpp)
emglken_vm(taylor)
set_property(TARGET taylor PROPERTY CXX_STANDARD 17)
set_property(TARGET taylor PROPERTY C_STANDARD 11)
target_compile_definitions(taylor PRIVATE QUENGLE $<$<COMPILE_LANGUAGE:C>:_XOPEN_SOURCE=600>)
target_compile_options(taylor PRIVATE $<$<COMPILE_LANGUAGE:C>:-include ${CMAKE_CURRENT_SOURCE_DIR}/src/quengle-stdio.h>
    -Wno-implicit-function-declaration)

# quengle: level 9 (GPL-2, as magnetic and scott). autosave in level9.c (QUENGLE), stdio over glk by quengle-stdio.h
add_executable(level9)
add_sources(level9 "${GT}level9/" SRCS level9.c bitmap.c)
add_sources(level9 "${GT}level9/Glk/" SRCS glk.c)
emglken_vm(level9)
target_compile_definitions(level9 PRIVATE QUENGLE BITMAP_DECODER NEED_STRICMP_PROTOTYPE
    stricmp=gln_strcasecmp strnicmp=gln_strncasecmp)
target_compile_options(level9 PRIVATE -include ${CMAKE_CURRENT_SOURCE_DIR}/src/quengle-stdio.h
    -Wno-implicit-function-declaration)

# quengle: scarier, spatterlight's (adrift 3.9/4 + 5; GPL-2: only in builds with QUENGLE_SCARIER, see HANDOFF).
# autosave in glk/os_glk_autosave.cpp (QUENGLE)
add_executable(scarier)
add_sources(scarier "${GT}scarier/"
    SRCS
    adrift5/a5blorb.cpp adrift5/a5deobf.cpp adrift5/a5expr.cpp adrift5/a5map.cpp adrift5/a5model.cpp
    adrift5/a5parse.cpp adrift5/a5rand.cpp adrift5/a5restr.cpp adrift5/a5sb.cpp adrift5/a5sexpr.cpp
    adrift5/a5state.cpp adrift5/a5xml.cpp adrift5/run/a5run.cpp adrift5/run/a5run_act.cpp
    adrift5/run/a5run_action.cpp adrift5/run/a5run_conv.cpp adrift5/run/a5run_events.cpp adrift5/run/a5run_ref.cpp
    adrift5/run/a5run_resp.cpp adrift5/run/a5run_save.cpp adrift5/run/a5run_sort.cpp adrift5/run/a5run_words.cpp
    adrift5/text/a5text.cpp adrift5/text/a5text_func.cpp adrift5/text/a5text_name.cpp adrift5/text/a5text_render.cpp
    adrift5/text/a5text_view.cpp common_utils/randomness.c common_utils/sc_garglk.c core/scbattle.cpp
    core/scdebug.cpp core/scevents.cpp core/scexpr.cpp core/scgamest.cpp core/scinterf.cpp core/sclocale.cpp
    core/scmap.cpp core/scmemos.cpp core/scnpcs.cpp core/scobjcts.cpp core/scparser.cpp core/scparser_entity.cpp
    core/scparser_match.cpp core/scparser_pattern.cpp core/scparser_pronoun.cpp core/scprintf.cpp
    core/scprintf_alr.cpp core/scprintf_buffer.cpp core/scprintf_input.cpp core/scprintf_output.cpp core/scprops.cpp
    core/scresour.cpp core/screstrs.cpp core/scserial.cpp core/sctaffil.cpp core/sctafpar.cpp
    core/sctafpar_fixups.cpp core/sctafpar_patches.cpp core/sctasks.cpp core/sctasks_action.cpp
    core/sctasks_endgame.cpp core/sctasks_move.cpp core/sctasks_run.cpp core/scutils.cpp core/scvars.cpp
    glk/os_glk.cpp glk/os_glk_a5.cpp glk/os_glk_a5_display.cpp glk/os_glk_autosave.cpp glk/os_glk_commands.cpp
    glk/os_glk_input.cpp glk/os_glk_locale.cpp glk/os_glk_map.cpp glk/os_glk_output.cpp glk/os_glk_platform.cpp glk/os_glk_quengle.cpp
    glk/os_glk_resources.cpp glk/os_glk_status.cpp glk/os_glk_symbols.cpp library/sclibrar_battle.cpp
    library/sclibrar_disambig.cpp library/sclibrar_disambig_400.cpp library/sclibrar_disambig_scan.cpp
    library/sclibrar_dispatch.cpp library/sclibrar_drop.cpp library/sclibrar_examine.cpp library/sclibrar_go.cpp
    library/sclibrar_meta.cpp library/sclibrar_misc.cpp library/sclibrar_open.cpp library/sclibrar_print.cpp
    library/sclibrar_put.cpp library/sclibrar_put_multiple.cpp library/sclibrar_read.cpp library/sclibrar_refuse.cpp
    library/sclibrar_room.cpp library/sclibrar_sitstand.cpp library/sclibrar_take.cpp
    library/sclibrar_take_multiple.cpp library/sclibrar_takefrom.cpp library/sclibrar_talk.cpp
    library/sclibrar_topic.cpp library/sclibrar_verbobj.cpp library/sclibrar_verbs.cpp mapdraw.cpp
    runner/scrun_dispatch.cpp runner/scrun_input.cpp runner/scrun_match.cpp runner/scrun_match_refusal.cpp
    runner/scrun_match_run.cpp runner/scrun_respell.cpp runner/scrun_split.cpp runner/scrun_tables.cpp
    runner/scrunner.cpp)
emglken_vm(scarier)
set_property(TARGET scarier PROPERTY CXX_STANDARD 17)
target_compile_definitions(scarier PRIVATE QUENGLE)
target_compile_options(scarier PRIVATE --use-port=zlib)
# the adrift 5 engine recurses deep (xml, expressions, text functions): the default 64 KB stack overflowed into
# globals (the main window pointer). popups ask from inside a turn, so asyncify unwinds that depth too
target_link_options(scarier PRIVATE --use-port=zlib -sSTACK_SIZE=4MB -sASYNCIFY_STACK_SIZE=262144)

add_executable(scare)
add_sources(scare "garglk/terps/scare/"
    SRCS os_glk.c scdebug.c scevents.c scexpr.c scgamest.c scinterf.c sclibrar.c
    sclocale.c scmemos.c scnpcs.c scobjcts.c scparser.c scprintf.c scprops.c
    scresour.c screstrs.c scrunner.c scserial.c sctaffil.c sctafpar.c sctasks.c
    scutils.c scvars.c)
emglken_vm(scare)
target_compile_options(scare PRIVATE --use-port=zlib)
target_link_options(scare PRIVATE --use-port=zlib)

add_executable(tads)
add_sources(tads "tads/glk/"
    SRCS memicmp.c osbuffer.c osemglken.c osextra.c osglk.c osglkban.c osmisc.c osparse.c
    osportable.cc t23run.cpp t2askf.c t2indlg.c t3askf.cpp t3indlg.cpp vmuni_cs.cpp)
add_sources(tads "tads/tads2/"
    SRCS argize.c bif.c bifgdum.c cmap.c cmd.c dat.c dbgtr.c errmsg.c execmd.c
    fio.c fioxor.c getstr.c ler.c linfdum.c lst.c mch.c mcm.c mcs.c obj.c oem.c
    os0.c oserr.c osifc.c osnoui.c osrestad.c osstzprs.c ostzposix.c out.c output.c
    ply.c qas.c regex.c run.c runstat.c suprun.c trd.c voc.c vocab.c)
add_sources(tads "tads/tads3/"
    SRCS charmap.cpp md5.cpp resldexe.cpp resload.cpp sha2.cpp std.cpp tcerr.cpp
    tcerrmsg.cpp tcgen.cpp tcglob.cpp tcmain.cpp tcprs.cpp tcprs_rt.cpp tcprsnf.cpp
    tcprsnl.cpp tcprsstm.cpp tcsrc.cpp tct3.cpp tct3_d.cpp tct3nl.cpp tct3stm.cpp
    tct3unas.cpp tctok.cpp utf8.cpp vmanonfn.cpp vmbif.cpp vmbifl.cpp vmbifreg.cpp
    vmbift3.cpp vmbiftad.cpp vmbiftio.cpp vmbignum.cpp vmbignumlib.cpp vmbt3_nd.cpp
    vmbytarr.cpp vmcfgmem.cpp vmcoll.cpp vmconhmp.cpp vmconsol.cpp vmcrc.cpp
    vmcset.cpp vmdate.cpp vmdict.cpp vmdynfunc.cpp vmerr.cpp vmerrmsg.cpp
    vmfile.cpp vmfilnam.cpp vmfilobj.cpp vmfref.cpp vmfunc.cpp vmglob.cpp
    vmgram.cpp vmhash.cpp vmhostsi.cpp vmhosttx.cpp vmimage.cpp vmimg_nd.cpp
    vmini_nd.cpp vminit.cpp vminitim.cpp vmintcls.cpp vmisaac.cpp vmiter.cpp
    vmlog.cpp vmlookup.cpp vmlst.cpp vmmain.cpp vmmcreg.cpp vmmeta.cpp
    vmnetfillcl.cpp vmobj.cpp vmop.cpp vmpack.cpp vmpat.cpp vmpool.cpp
    vmpoolim.cpp vmregex.cpp vmrun.cpp vmrunsym.cpp vmsa.cpp vmsave.cpp vmsort.cpp
    vmsortv.cpp vmsrcf.cpp vmstack.cpp vmstr.cpp vmstrbuf.cpp vmstrcmp.cpp
    vmtmpfil.cpp vmtobj.cpp vmtype.cpp vmtypedh.cpp vmtz.cpp vmtzobj.cpp
    vmundo.cpp vmvec.cpp vmconnom.cpp)
emglken_vm(tads)
set_property(TARGET tads PROPERTY CXX_STANDARD 11)
target_compile_definitions(tads PRIVATE EMGLKEN GLK GLK_TIMERS GLK_UNICODE NO_T2_COPYRIGHT_NOTICE
    OSNOUI_OMIT_OS_FPRINT OSNOUI_OMIT_OS_FPRINTZ RUNTIME TC_TARGET_T3 VMGLOB_STRUCT)
target_compile_options(tads PRIVATE -Wno-deprecated-register -Wno-logical-not-parentheses -Wno-pointer-sign -Wno-string-concatenation)
#target_link_options(tads PRIVATE -sASYNCIFY_ADVISE)
#target_link_options(tads PRIVATE -sASYNCIFY_REMOVE=@${CMAKE_CURRENT_LIST_DIR}/src/asyncify-remove-list-tads.txt)
if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
    # TADS needs to be compiled in at least -O2, or else it will use 4GB+ of RAM and be very slow
    target_compile_options(tads PRIVATE -O2)
    target_link_options(tads PRIVATE -O2)
endif()