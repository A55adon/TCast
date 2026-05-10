# TCast Rust

This is the Rust/WebView rewrite of TCast.

Run it from this folder:

```powershell
cargo run
```

The app uses:

- `wry` + `tao` for desktop windows and WebView rendering.
- Plain HTML/CSS/JavaScript for the control UI.
- Rust modules for project state, JSON compatibility, media import, archive import/export, and projector-window creation.
- `rfd` for native file dialogs.
- `image` for image conversion and split-preview generation.
- `zip` for `.tct` import/export without shelling out to PowerShell.

## Layout

- `src/main.rs`: app entry point, event loop, IPC bridge, and projection window lifecycle.
- `src/state.rs`: project, scene, resource, save/load, and command handling.
- `src/model.rs`: JSON-compatible data structures and frontend snapshot types.
- `src/media.rs`: image conversion and split-preview creation.
- `src/archive.rs`: `.tct` archive export/import.
- `src/ui.rs`: embeds the HTML/CSS/JS and creates projection-window HTML.
- `ui/styles.css`: app design system and layout.
- `ui/app.js`: DOM UI, drag/drop, dialogs, and IPC commands.

## Current Notes

- Existing project JSON keys are preserved where practical.
- Image imports are converted to PNG.
- Video imports are copied and projected through the WebView video element.
- Video thumbnail extraction is not implemented yet; video resources show video previews/placeholders instead.
- Projector outputs are borderless fullscreen WebView windows. The first output targets monitor index `1` when available, matching the old control-screen-plus-projectors assumption.
- The diagnostics drawer shows current project state and logs.
