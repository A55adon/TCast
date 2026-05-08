# TCast Requirements And Capability Summary

TCast is a theatre projection control program for school productions. It manages a show project made of scenes, media resources, and one to six projector outputs. The original implementation is C++ with GLFW, OpenGL, FFmpeg, RmlUi, ImGui, and hand-managed JSON/filesystem state. The Rust rewrite must preserve the behavior below while replacing the low-level UI stack with a higher-level HTML/CSS style interface.

## Core Domain

- A project represents one theatre/show setup.
- Each project has a name, description, app version, save location, and projector count from 1 to 6.
- A project contains scenes.
- A scene contains one source assignment per projector.
- A scene contains connection flags between adjacent projectors.
- Connected adjacent projectors share one source and display horizontal slices of that source.
- A project contains resources, which are imported media files copied into the project folder.

## Project Storage

Projects are stored as folders, usually under `saves/folderSaves`.

Each project folder contains:

- `saveData.json`: project metadata.
- `scenesData.json`: scene list, projector source paths, split preview paths, and connections.
- `resourceData.json`: imported resource list and resource id counter.
- `resources/images`: imported image files converted/copied as PNG.
- `resources/videos`: imported video files.
- `resources/videos/thumbnails`: generated thumbnails for videos when available.
- `resources/splits`: generated preview images for connected projector splits.

The existing JSON keys are part of the compatibility surface:

- `saveData.json`: `projectName`, `projectorCount`, `description`, `path`, `version`.
- `scenesData.json`: root key `scenes`; each scene has `sceneName`, `sources`, `splitSources`, `connections`.
- `resourceData.json`: `maxId`, `resources`; each resource has `id`, `path`, `name`, `isVideo`, `thumbnail_id`.

## Startup And Project Flow

- On startup, the app can open the most recent project from `saves/recent.path` if that file exists and points to a valid project.
- If no recent project is available, the app opens a startup screen.
- The startup screen has a "new project" tab and a "load project" tab.
- New project creation collects project name, projector count, description, and save directory.
- New projects create the project folder and default scene.
- Loading a project reads `saveData.json`, `scenesData.json`, and `resourceData.json`.
- Existing project folders can be listed from the default saves folder.
- Existing project folders can be selected, loaded, and deleted from the startup screen.

## File Menu

- Create a new project.
- Open/load another project.
- Save the current project.
- Save As by copying the current project folder to another directory.
- Export the current project as a `.tct` archive.
- Import a `.tct` archive into the local saves folder.
- Close the current project and return to the startup screen.
- Exit the application.

## Scenes

- Show all scenes in a left-side scene list.
- Select the active scene.
- Add a scene with an automatic name such as `Szene N`.
- Rename a scene with inline editing.
- Delete a scene.
- Duplicate a scene, including its assignments and connections, with copy-number naming.
- Move the active scene up or down.
- Delete all scenes, then keep or recreate at least one valid scene before saving.
- Persist scene changes immediately to the project folder.

## Resources

- Show resources in a bottom resource panel.
- Import one or multiple media files.
- Supported image inputs include PNG, JPG/JPEG, BMP, TGA, GIF, TIFF, and WebP where the decoder supports them.
- Supported video input is MP4 in the original UI; projector playback also recognizes several common video extensions.
- Imported images are written into the project as PNG resources.
- Imported videos are copied into the project video folder.
- Video imports should have a thumbnail or a clear video preview/placeholder.
- Resource names can be edited.
- Resources can be deleted.
- Missing resource files are detected and removed from the resource list.
- Resource selection is reflected visually.
- Resources can be dragged onto a projector preview.

## Projector Assignment And Splitting

- The main workspace shows one projector preview for each configured projector.
- Projector previews keep a 16:9 aspect ratio.
- A resource can be assigned to a projector from a resource picker.
- A resource can be assigned by dragging from the resource panel onto a projector preview.
- A projector source can be cleared.
- Adjacent projector connection buttons toggle whether two neighboring projectors are connected.
- When projectors are connected, the leftmost projector's source becomes the shared source for the connected group.
- The app calculates split ranges for each projector in the group.
- Split ranges are horizontal percentages, for example two connected projectors use `0.0..0.5` and `0.5..1.0`.
- UI previews show either the assigned full source or the generated split preview.
- Projection output uses the same split logic for image and video playback.

## Projection Output

- Start projection opens output windows for the current active scene.
- The original app maps projector `0` to monitor index `1`, projector `1` to monitor index `2`, and so on, leaving the control window on the primary monitor.
- Each output window is borderless, fullscreen, non-resizable, and should not steal focus if the platform allows it.
- Image resources are displayed fullscreen.
- Video resources are decoded and looped.
- Split video playback loops within the split time/texture region.
- Stop projection closes all active output windows cleanly.
- Starting projection again first stops existing output windows.

## Settings And Debugging

- A settings overlay exists in the original UI with controls for updates, last-project loading, project rename/delete/open/repair, VSync, FPS limit, fullscreen, debug mode, cleanup, and automatic cleanup. Many of these controls are visual placeholders in the current implementation and can become real features in the rewrite.
- The original debug build has an ImGui debug window toggled by `F1`.
- The original debug build toggles the RmlUi debugger with `F8`.
- The rewrite should include visible diagnostics for project state, scenes, resources, projector assignments, active windows, errors, and logs.
- Errors should be shown as dismissible toast messages and logged for debugging.

## Rust Rewrite Direction

- The rewrite should be a new Rust app, not a direct port of the low-level C++ architecture.
- The UI should be based on HTML/CSS concepts and assets, with ordinary DOM layout and CSS styling.
- The backend should expose structured commands rather than UI code directly mutating global state.
- State should be serializable, testable, and inspectable.
- Project loading/saving should preserve the existing project folder and JSON format where practical.
- The app should prefer high-level libraries for UI, dialogs, image conversion, archive import/export, logging, and error handling.
- The code should keep platform-specific work isolated.
- The app should be debuggable through logs, a diagnostics panel, and small backend modules with focused responsibilities.

## Acceptable Enhancements

- Cleaner design and layout.
- Better error messages.
- A visible project health/diagnostics panel.
- Real settings implementation.
- Safer import/export that does not shell out to PowerShell.
- Better resource validation.
- Better video preview behavior.
- Theme variables in CSS.
- More robust projector monitor selection.
- More explicit save status.

## Non-Negotiable Compatibility

- Do not remove project creation, loading, saving, import, or export.
- Do not remove scene management.
- Do not remove media resource import, rename, delete, or assignment.
- Do not remove support for 1 to 6 projectors.
- Do not remove connected-projector split behavior.
- Do not remove projection start/stop.
- Do not silently discard existing project JSON fields.
