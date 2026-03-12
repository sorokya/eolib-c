/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "eolib", "index.html", [
    [ "eolib-c", "index.html", null ],
    [ "Getting Started", "getting_started.html", [
      [ "Prerequisites &amp; Development Environment", "getting_started.html#gs_prereqs", [
        [ "Linux (Ubuntu / Debian)", "getting_started.html#gs_linux", null ],
        [ "macOS", "getting_started.html#gs_macos", null ],
        [ "Windows", "getting_started.html#gs_windows", null ]
      ] ],
      [ "Downloading a Release", "getting_started.html#gs_download", [
        [ "Using CMake (preferred)", "getting_started.html#gs_cmake", null ],
        [ "Manual compilation", "getting_started.html#gs_manual", null ]
      ] ],
      [ "Example 1 — Packet Inspector", "getting_started.html#gs_example1", null ],
      [ "Example 2 — EIF Pub File Reader", "getting_started.html#gs_example2", null ],
      [ "Memory Safety", "getting_started.html#gs_memory", [
        [ "EoReader — borrows its buffer", "getting_started.html#gs_memory_reader", null ],
        [ "EoWriter — owns a heap buffer", "getting_started.html#gs_memory_writer", null ],
        [ "Protocol structs — init, then free", "getting_started.html#gs_memory_structs", null ]
      ] ],
      [ "Next Steps", "getting_started.html#gs_next", null ]
    ] ],
    [ "Getting Started (Lua)", "getting_started_lua.html", [
      [ "Prerequisites &amp; Development Environment", "getting_started_lua.html#gsl_prereqs", [
        [ "Linux (Ubuntu / Debian)", "getting_started_lua.html#gsl_linux", null ],
        [ "macOS", "getting_started_lua.html#gsl_macos", null ],
        [ "Windows", "getting_started_lua.html#gsl_windows", null ]
      ] ],
      [ "Downloading a Release", "getting_started_lua.html#gsl_download", [
        [ "Installing the module", "getting_started_lua.html#gsl_install", null ],
        [ "IDE support", "getting_started_lua.html#gsl_ide", null ]
      ] ],
      [ "Example 1 — Packet Inspector", "getting_started_lua.html#gsl_example1", null ],
      [ "Example 2 — EIF Pub File Reader", "getting_started_lua.html#gsl_example2", null ],
      [ "Memory Safety", "getting_started_lua.html#gsl_memory", null ],
      [ "Next Steps", "getting_started_lua.html#gsl_next", null ]
    ] ],
    [ "Error Handling", "error_handling.html", null ],
    [ "Reading Data", "reading_data.html", null ],
    [ "Writing Data", "writing_data.html", null ],
    [ "Optional Fields", "optional_fields.html", null ],
    [ "Encryption", "encryption.html", null ],
    [ "Packet Sequencing", "sequencing.html", null ],
    [ "Protocol Struct Interface", "protocol_interface.html", [
      [ "Initialization", "protocol_interface.html#pi_init", null ],
      [ "Dispatch Functions", "protocol_interface.html#pi_dispatch", null ],
      [ "Packet-Specific Dispatch", "protocol_interface.html#pi_packets", null ],
      [ "Writing Generic Code", "protocol_interface.html#pi_generic", null ],
      [ "Memory Layout", "protocol_interface.html#pi_layout", null ]
    ] ],
    [ "Data Structures", "annotated.html", [
      [ "Data Structures", "annotated.html", "annotated_dup" ],
      [ "Data Structure Index", "classes.html", null ],
      [ "Data Fields", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Variables", "functions_vars.html", "functions_vars" ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "Globals", "globals.html", [
        [ "All", "globals.html", "globals_dup" ],
        [ "Functions", "globals_func.html", "globals_func" ],
        [ "Typedefs", "globals_type.html", "globals_type" ],
        [ "Enumerations", "globals_enum.html", null ],
        [ "Enumerator", "globals_eval.html", "globals_eval" ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"annotated.html",
"globals_t.html",
"protocol_8c.html#a8a0127158517854de2e1374dc14989cd",
"protocol_8h.html#a05bb5e533d1a93c1b8c55c7e3e2db1ddaa4546e8508624ebfddb29d3410c8b91e",
"protocol_8h.html#a2c794c5c13ab4dd7e65bad031dbe41c3",
"protocol_8h.html#a558fc5e077f4edede8cb1810e0c1702d",
"protocol_8h.html#a86699ad624fb843fce458a96c94f29ba",
"protocol_8h.html#ab77d2589b49b2e8bbebe2fdb68ec4ae3",
"protocol_8h.html#af499a869c750d326ef1b9b0f783fb053",
"structBankTakeClientPacket.html#ad241d0e2e07275883c5ce1c0c34441b9",
"structCharacterStatsReset.html#af557c9b116e316f1e543953b4959390e",
"structEifRecord.html#ad30573287014e7660122407935fb359b",
"structGuildCreateServerPacket.html#a89d169cfe2972141bfb8b826464fe07a",
"structJukeboxMsgClientPacket.html",
"structNpcSpecServerPacket.html#a63083bb8a3d68d49f3f249959956cb7e",
"structRangeRequestClientPacket.html#a884cfbe16f5a44dc2662ad826b40fec9",
"structSpellTargetSelfServerPacket.html#a831b896d60f0b546c19f3edfd10de80d",
"structWarpCreateServerPacket.html#a834f072277ac5be8262d394d0bc1176e"
];

var SYNCONMSG = 'click to disable panel synchronization';
var SYNCOFFMSG = 'click to enable panel synchronization';
var LISTOFALLMEMBERS = 'List of all members';