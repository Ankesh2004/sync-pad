# sync-pad Directory Structure
```
sync-pad/
├── core/
│ ├── engine.hpp/.cpp # Doc model, op log, apply/serialize
│ ├── transport.hpp/.cpp # TCP client/server, framing, heartbeat
│ ├── discovery.hpp/.cpp # (opt) UDP broadcast
│ └── storage.hpp/.cpp # appData paths, doc/oplog persistence, CRC
├── ui_cli/
│ └── cli_main.cpp # Terminal editor/viewer + status line
├── ui_gui/
│ └── gui_main.cpp # FLTK/Qt main, notepad widget, status bar
└── common/
└── platform.hpp # POSIX/Win socket shims, termios helpers
```
### Description

- **core/**: Contains core syncing engine modules including document model, operation log, network transport, discovery, and storage management.
- **ui_cli/**: Command-line interface application for terminal-based editing and viewing.
- **ui_gui/**: Graphical user interface application using FLTK framework.
- **common/**: Platform-specific helpers for sockets and terminal controls supporting POSIX and Windows.

### Prerequsities
- FLTK
