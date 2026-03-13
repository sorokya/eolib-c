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
      [ "Getting the library", "getting_started.html#gs_download", [
        [ "Option 1 — Pre-built release (recommended)", "getting_started.html#gs_prebuilt", null ],
        [ "Option 2 — Build from source", "getting_started.html#gs_source", null ]
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
      [ "Prerequisites", "getting_started_lua.html#gsl_prereqs", [
        [ "Linux (Ubuntu / Debian)", "getting_started_lua.html#gsl_linux", null ],
        [ "macOS", "getting_started_lua.html#gsl_macos", null ],
        [ "Windows", "getting_started_lua.html#gsl_windows", null ]
      ] ],
      [ "Getting the bindings", "getting_started_lua.html#gsl_download", [
        [ "Option 1 — Pre-built drop-in (recommended)", "getting_started_lua.html#gsl_dropin", null ],
        [ "Option 2 — Build and install from source", "getting_started_lua.html#gsl_install", [
          [ "Troubleshooting: module 'eolib' not found", "getting_started_lua.html#gsl_troubleshoot", null ]
        ] ],
        [ "IDE support (lua-language-server)", "getting_started_lua.html#gsl_ide", null ]
      ] ],
      [ "Example 1 — Packet Inspector", "getting_started_lua.html#gsl_example1", null ],
      [ "Example 2 — EIF Pub File Reader", "getting_started_lua.html#gsl_example2", null ],
      [ "Memory Safety", "getting_started_lua.html#gsl_memory", null ],
      [ "Next Steps", "getting_started_lua.html#gsl_next", null ]
    ] ],
    [ "RNG (Random Number Generator)", "rng.html", [
      [ "Overview", "rng.html#rng_overview", null ],
      [ "Seeding", "rng.html#rng_seeding", [
        [ "Reproducing the original client behaviour", "rng.html#rng_client_seed", null ]
      ] ],
      [ "Algorithm", "rng.html#rng_algorithm", null ],
      [ "API", "rng.html#rng_api", null ]
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
    [ "WebAssembly (WASM)", "wasm.html", [
      [ "Use in JavaScript / TypeScript projects", "wasm.html#wasm_ts", null ]
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
        [ "Variables", "globals_vars.html", "globals_vars" ],
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
"globals_func_u.html",
"protocol_8c.html#a0e58dbfae3768ca3db799bac11017ab4",
"protocol_8c.html#a236548c7c1f643128b57300c53957ec0",
"protocol_8c.html#a36ff236bc5e9ed6e0fb620b3411ab43e",
"protocol_8c.html#a4aa257ee38f83488ab775d965c34eb4f",
"protocol_8c.html#a5f16f3b78112c6e9c08011666670bc68",
"protocol_8c.html#a6fbcc13c9af06b700d9aafef1319011f",
"protocol_8c.html#a82215a371622d32e1c7c1cfdab391675",
"protocol_8c.html#a95492d61a2a580a7cf0bcde1af416e8e",
"protocol_8c.html#aa8d098fcedfecee1c0d999086999dbf8",
"protocol_8c.html#abcd46585080901da1de50d7322aa8d93",
"protocol_8c.html#ad0b87ed6136daf71c35b956a6dcff0c3",
"protocol_8c.html#ae4375b33d002812a43e6435689b3ece0",
"protocol_8c.html#afb0ae8b8280f35673689352c375c95d1",
"protocol_8h.html#a142561f81038805cfc8786db4316b8b4",
"protocol_8h.html#a42f1d7b7d21b206d2f8dd68b20491c80",
"protocol_8h.html#a6b2feb855143e68470fc16141b77c3fd",
"protocol_8h.html#aa3fd9a9b720fca3f21f5ab868a561e02",
"protocol_8h.html#ad7da27bd8ffe5746919104f6345496c1",
"structAccountCreateClientPacket.html#ac791c42a3cea527bbae6521202679f78",
"structCharItem.html",
"structCoords.html#a16146bbbc7b8378f7a7b048297c3a0c0",
"structEquipmentCharacterSelect.html#a53cb118459e8ffdf6bb029091a74a9fe",
"structInitInitServerPacket.html#a346a8a0c3c6964c34bac2f2751af6cf9",
"structMapItem.html#aa1f0009e6de8c1dfd58d01cf5358e70d",
"structPartyRequestServerPacket.html#a3d04711c06d025fa8c0d6b1a0a199912",
"structShopSoldItem.html#a76dd251e4337ba89077ac0f664439864",
"structTalkReplyServerPacket.html"
];

var SYNCONMSG = 'click to disable panel synchronization';
var SYNCOFFMSG = 'click to enable panel synchronization';
var LISTOFALLMEMBERS = 'List of all members';