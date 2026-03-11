# EOLib

[![Build Status][actions-badge]][actions-url]
[![Docs][docs-badge]][docs-url]
[![License][mit-badge]][mit-url]

[docs-badge]: https://img.shields.io/github/actions/workflow/status/sorokya/eolib-c/docs.yml?label=docs
[docs-url]: https://sorokya.github.io/eolib-c/
[mit-badge]: https://img.shields.io/badge/license-MIT-blue.svg
[mit-url]: https://github.com/sorokya/eolib-c/blob/master/LICENSE
[actions-badge]: https://github.com/sorokya/eolib-c/actions/workflows/build.yml/badge.svg
[actions-url]: https://github.com/sorokya/eolib-c/actions/workflows/build.yml

A core C library for writing applications related to Endless Online

## Features

Read and write the following EO data structures:

- Client packets
- Server packets
- Endless Map Files (EMF)
- Endless Item Files (EIF)
- Endless NPC Files (ENF)
- Endless Spell Files (ESF)
- Endless Class Files (ECF)
- Endless Server Files (Inns, Drops, Shops, etc.)

Utilities:

- Data reader
- Data writer
- Number encoding
- String encoding
- Data encryption
- Packet sequencer
