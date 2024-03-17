# Notepad-- Roadmap
The items in this list are not in any particular order. This list will be updated occasionally as development progresses.

## General
- [x] Initial project setup (build process)
- [ ] Platform layers
  - [ ] Windows
  - [ ] Linux
  - [ ] macOS
- [ ] Windowing
  - [ ] Multi-window support
- [ ] Keyboard input
- [ ] Mouse input
- [ ] System font
  - [ ] Font loading
  - [ ] Font system
  - [ ] Font rendering
- [ ] Event/message system
- [ ] Buffer (represents text content, as well as edits, undo stack, file watching, etc.)
  - [ ] Change detection/notification
  - [ ] Read-only/writeable
  - [ ] Get/set content
  - [ ] Caret and navigation
    - [ ] Go to position
    - [ ] Go to line number
    - [ ] Go to column/line


## UI
- [ ] UI system/manager
- [ ] UI Controls
  - [ ] ui_window (container to hold controls, optional standard minimize/maximize/close buttons)
  - [ ] ui_bufferdisplay (scrollable text container used to display a buffer)
    - [ ] Text rendering, line by line and potentially tokenized.
    - [ ] Line number display
      - [ ] Standard line numbers
      - [ ] Relative line numbers
    - [ ] Set/change buffer

## Renderer
- [ ] Renderer frontend
  - [ ] Shape rendering (lines, boxes, etc.)
- [ ] Renderer backend (Vulkan)

### High-level design
Notepad-- was concieved as a joke on stream chat as a competitor to the editors out there with a "less is more" approach. The 
primary goal is to have an editor that is fast, responsive, relatively lightweight and portable. It will be GPU driven and 
will be designed with a keyboard-centric layout (although the mouse will be usable as well).

### Main components
Notepad-- will make extensive use of "buffers", which hold text content, may be editable or read-only, and manage things such as 
undo/redo stack, etc.. It is important to note that buffers are _not_ connected to displays directly, 
as that is a UI feature. The reason for this is because there can be more than one "view" of a buffer open at once. For example,
The editor might have the same code file displayed twice, side by side. Each of these displays point to the same buffer, but each 
view has its own caret, position, selection, scroll position, etc. When buffers change, they should send out a notification of 
the change so that anything watching/displaying them gets updated. As an eventual optimization, the change diff should be included
as part of this notification to avoid re-tokenizing the entire document (although this may be required anyway at some point).

Read-only buffers can be used for on-screen displays (such as status line, file names, file tree view, etc.).

Other uses for editable buffers might include a "command" buffer to enter editor commands into.

Buffers will each have a unique numeric id instead of a pointer reference.
Buffers will each have a name and a filename separately.

Buffers are either "attached" or "detached" from a file. Attached means that the buffer is directly associated with an already-existing file.
Detached means it is not attached to a file. When a buffer is created, a filename can optionally be passed. If the file exists,
the buffer is attached to it automatically. If the file does not exist but a name is provided, the file is created when the buffer 
is saved, and the buffer is then attached to the file. If no filename is provided, the buffer is always detached.

Buffers that are read-only cannot be edited or saved.

Buffers can also be setup to not allow file attaching, and thus can be edited/written to but cannot be saved (i.e the "command" buffer).

During creation, if a buffer already exists that contains the same filename, create simply returns the id of the existing buffer. If 
the buffer can be created otherwise using the flags passed to it, then the newly-created buffer's id is returned. If creation fails,
INVALID_ID is returned.

All visible controls contain a transform of thier own.

ui_windows each always contain a transform and have a list of child windows. This is a list of ids.
ui_windows each maintain lists of various control types they contain (such as ui_bufferdisplays). This is a list of ids.
If a ui_window is hidden, so too are its children and attached controls.


