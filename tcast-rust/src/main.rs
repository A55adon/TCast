mod archive;
mod media;
mod model;
mod state;
mod ui;

use anyhow::{Context, Result};
use model::{IpcMessage, IpcResponse, ProjectionSpec};
use serde_json::json;
use state::AppState;
use tao::dpi::LogicalSize;
use tao::event::{Event, WindowEvent};
use tao::event_loop::{ControlFlow, EventLoopBuilder, EventLoopWindowTarget};
use tao::window::{Fullscreen, Window, WindowBuilder};
use tracing_subscriber::EnvFilter;
use wry::{WebView, WebViewBuilder};
use percent_encoding::percent_decode_str;
use serde::Deserialize;

#[cfg(target_os = "windows")]
use wry::WebViewBuilderExtWindows;

#[cfg(target_os = "windows")]
use tao::platform::windows::WindowExtWindows;

#[cfg(target_os = "windows")]
mod windows_utils {
    use windows::Win32::UI::WindowsAndMessaging::{
        SetWindowLongPtrW, GetWindowLongPtrW, GWL_STYLE, GWL_EXSTYLE,
        WS_THICKFRAME, WS_CAPTION, WS_SYSMENU,
        WS_MAXIMIZEBOX, WS_MINIMIZEBOX,
        WS_EX_APPWINDOW, WS_EX_WINDOWEDGE,
        SetWindowPos, SWP_NOMOVE, SWP_NOSIZE, SWP_NOZORDER, SWP_FRAMECHANGED
    };
    use windows::Win32::Foundation::HWND;

    pub fn enable_snap(hwnd: isize) {
        unsafe {
            let hwnd = HWND(hwnd as *mut _);
            let mut style = GetWindowLongPtrW(hwnd, GWL_STYLE);
            let mut ex_style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);

            // Add styles needed for Aero Snap
            style |= (WS_THICKFRAME.0 as isize)
                | (WS_CAPTION.0 as isize)
                | (WS_SYSMENU.0 as isize)
                | (WS_MAXIMIZEBOX.0 as isize)
                | (WS_MINIMIZEBOX.0 as isize);

            // Add extended styles
            ex_style |= (WS_EX_APPWINDOW.0 as isize)
                | (WS_EX_WINDOWEDGE.0 as isize);

            SetWindowLongPtrW(hwnd, GWL_STYLE, style);
            SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex_style);

            // Force window to update its frame
            SetWindowPos(
                hwnd,
                None,
                0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED
            );
        }
    }
}

#[derive(Debug, Clone)]
enum UserEvent {
    Ipc(String),
}

struct ProjectionWindow {
    window: Window,
    _webview: WebView,
}

fn mime_for_path(path: &str) -> &'static str {
    let path = path.to_lowercase();
    if path.ends_with(".html") || path.ends_with(".htm") { return "text/html"; }
    if path.ends_with(".css")  { return "text/css"; }
    if path.ends_with(".js")   { return "text/javascript"; }
    if path.ends_with(".png")  { return "image/png"; }
    if path.ends_with(".jpg") || path.ends_with(".jpeg") { return "image/jpeg"; }
    if path.ends_with(".gif")  { return "image/gif"; }
    if path.ends_with(".webp") { return "image/webp"; }
    if path.ends_with(".mp4")  { return "video/mp4"; }
    if path.ends_with(".webm") { return "video/webm"; }
    if path.ends_with(".mov")  { return "video/quicktime"; }
    "application/octet-stream"
}

/// Extracts the file path from a custom protocol URI.
fn extract_path_from_uri(uri: &str) -> String {
    let decoded = percent_decode_str(uri).decode_utf8_lossy().to_string();

    let path = if let Some(rest) = decoded.strip_prefix("asset://") {
        rest.to_string()
    } else if let Some(rest) = decoded.strip_prefix("https://").or_else(|| decoded.strip_prefix("http://")) {
        let slash = match rest.find('/') {
            Some(pos) => pos,
            None => return String::new(),
        };
        let authority = &rest[..slash];
        let path = &rest[slash + 1..];

        if cfg!(windows) {
            if let Some(sub) = authority.strip_prefix("asset.") {
                if sub.len() == 1 && sub.as_bytes()[0].is_ascii_alphabetic() {
                    let drive = (sub.as_bytes()[0] as char).to_ascii_uppercase();
                    return format!("{}:/{}", drive, path);
                }
            }
        }
        path.to_string()
    } else {
        return String::new();
    };

    if cfg!(windows)
        && path.len() >= 2
        && path.as_bytes()[1] == b'/'
        && path.as_bytes()[0].is_ascii_alphabetic()
    {
        let drive = (path.as_bytes()[0] as char).to_ascii_uppercase();
        return format!("{}:/{}", drive, &path[2..]);
    }

    path
}

fn main() -> Result<()> {
    init_logging();

    let mut app_state = AppState::new().context("initializing TCast state")?;
    let event_loop = EventLoopBuilder::<UserEvent>::with_user_event().build();
    let proxy = event_loop.create_proxy();

    let control_window = WindowBuilder::new()
        .with_title("TCast")
        .with_inner_size(LogicalSize::new(1280.0, 820.0))
        .with_min_inner_size(LogicalSize::new(960.0, 620.0))
        .with_decorations(false)
        .with_transparent(false)
        .build(&event_loop)
        .context("creating control window")?;
    let control_window_id = control_window.id();

    // Enable Aero Snap BEFORE creating the webview
    #[cfg(target_os = "windows")]
    {
        let hwnd = control_window.hwnd();
        windows_utils::enable_snap(hwnd);
    }

    let main_html_path = ui::main_html_path();
    let main_url = format!("asset://{}", main_html_path.display());

    let mut control_webview_builder = WebViewBuilder::new()
        .with_url(&main_url)
        .with_devtools(cfg!(debug_assertions))
        .with_hotkeys_zoom(true)
        .with_ipc_handler(move |request| {
            let _ = proxy.send_event(UserEvent::Ipc(request.body().clone()));
        })
        .with_custom_protocol("asset".into(), |_webview_id, request| {
            let uri = request.uri().to_string();
            let path_str = extract_path_from_uri(&uri);
            let path_str = match path_str.find('?') {
                Some(pos) => &path_str[..pos],
                None => &path_str,
            };

            if path_str.is_empty() {
                return wry::http::Response::builder()
                    .status(400)
                    .header("Content-Type", "text/plain")
                    .body(std::borrow::Cow::Borrowed(b"missing path".as_slice()))
                    .unwrap();
            }

            let path = std::path::Path::new(path_str);

            match std::fs::read(path) {
                Ok(bytes) => {
                    let mime = mime_for_path(path.to_str().unwrap_or(""));
                    wry::http::Response::builder()
                        .status(200)
                        .header("Content-Type", mime)
                        .header("Content-Length", bytes.len().to_string())
                        .header("Access-Control-Allow-Origin", "*")
                        .body(std::borrow::Cow::Owned(bytes))
                        .unwrap()
                }
                Err(e) => {
                    eprintln!("Failed to load asset {:?}: {:?}", path, e);
                    wry::http::Response::builder()
                        .status(404)
                        .header("Content-Type", "text/plain")
                        .body(std::borrow::Cow::Borrowed(b"File not found".as_slice()))
                        .unwrap()
                }
            }
        });

    #[cfg(target_os = "windows")]
    {
        control_webview_builder = control_webview_builder.with_https_scheme(true);
    }

    let control_webview = control_webview_builder
        .build(&control_window)
        .context("creating control webview")?;

    let mut projection_windows: Vec<ProjectionWindow> = Vec::new();

    event_loop.run(move |event, target, control_flow| {
        *control_flow = ControlFlow::Wait;

        match event {
            Event::UserEvent(UserEvent::Ipc(raw)) => {
                process_ipc(
                    raw,
                    target,
                    &mut app_state,
                    &control_webview,
                    &mut projection_windows,
                    &control_window,
                );
            }
            Event::WindowEvent {
                event: WindowEvent::CloseRequested,
                window_id,
                ..
            } => {
                if window_id == control_window_id {
                    projection_windows.clear();
                    *control_flow = ControlFlow::Exit;
                } else {
                    projection_windows.retain(|projection| projection.window.id() != window_id);
                    app_state.set_projection_active(!projection_windows.is_empty());
                }
            }
            Event::WindowEvent {
                event: WindowEvent::Resized(_),
                window_id,
                ..
            } => {
                if window_id == control_window_id {
                    let is_maximized = control_window.is_maximized();
                    let script = format!(
                        "document.querySelector('#maximize-window')?.classList.toggle('maximized', {});",
                        is_maximized
                    );
                    let _ = control_webview.evaluate_script(&script);
                }
            }
            _ => {}
        }
    });
}

#[derive(Debug, Deserialize)]
struct MoveWindowPayload {
    #[serde(rename = "deltaX")]
    delta_x: i32,
    #[serde(rename = "deltaY")]
    delta_y: i32,
}

fn process_ipc(
    raw: String,
    target: &EventLoopWindowTarget<UserEvent>,
    app_state: &mut AppState,
    control_webview: &WebView,
    projection_windows: &mut Vec<ProjectionWindow>,
    control_window: &Window,
) {
    let parsed: Result<IpcMessage, _> = serde_json::from_str(&raw);
    let message = match parsed {
        Ok(message) => message,
        Err(error) => {
            app_state.log_error(format!("Invalid IPC message: {error}"));
            return;
        }
    };

    let result = match message.command.as_str() {
        "start_projection" => {
            projection_windows.clear();
            match app_state.prepare_projection() {
                Ok(specs) => match create_projection_windows(target, &specs) {
                    Ok(windows) => {
                        *projection_windows = windows;
                        app_state.set_projection_active(!projection_windows.is_empty());
                        Ok(json!(app_state.snapshot()))
                    }
                    Err(error) => Err(error),
                },
                Err(error) => Err(error),
            }
        }
        "stop_projection" => {
            projection_windows.clear();
            app_state.set_projection_active(false);
            app_state.log_info("Stopped projector outputs");
            Ok(json!(app_state.snapshot()))
        }
        "minimize_window" => {
            control_window.set_minimized(true);
            Ok(json!({ "action": "minimized" }))
        }
        "maximize_window" => {
            let is_maximized = control_window.is_maximized();
            control_window.set_maximized(!is_maximized);
            Ok(json!({
                "action": if !is_maximized { "maximized" } else { "restored" },
                "state": !is_maximized
            }))
        }
        "get_window_state" => {
            let is_maximized = control_window.is_maximized();
            Ok(json!({
                "maximized": is_maximized
            }))
        }
        "close_window" => {
            std::process::exit(0);
        }
        "move_window" => {
            control_window.drag_window();
            Ok(json!({ "action": "dragging" }))
        }
        command => {
            let result = app_state.handle_command(command, message.payload);
            if command == "close_project" {
                projection_windows.clear();
                app_state.set_projection_active(false);
            }
            result
        }
    };

    let response = match result {
        Ok(data) => IpcResponse {
            id: message.id,
            ok: true,
            data: Some(data),
            error: None,
        },
        Err(error) => {
            let error = format!("{error:#}");
            app_state.log_error(error.clone());
            IpcResponse {
                id: message.id,
                ok: false,
                data: None,
                error: Some(error),
            }
        }
    };

    if let Err(error) = send_response(control_webview, &response) {
        app_state.log_error(format!("Could not send IPC response: {error:#}"));
    }
}

fn create_projection_windows(
    target: &EventLoopWindowTarget<UserEvent>,
    specs: &[ProjectionSpec],
) -> Result<Vec<ProjectionWindow>> {
    let monitors: Vec<_> = target.available_monitors().collect();

    let mut ordered_monitors = Vec::with_capacity(monitors.len());
    let primary_index = monitors.iter().position(|m| {
        let pos = m.position();
        pos.x == 0 && pos.y == 0
    });

    if let Some(primary_idx) = primary_index {
        ordered_monitors.push(monitors[primary_idx].clone());
        for (i, monitor) in monitors.iter().enumerate() {
            if i != primary_idx {
                ordered_monitors.push(monitor.clone());
            }
        }
    } else {
        ordered_monitors = monitors.clone();
    }

    let mut windows = Vec::new();
    for (idx, spec) in specs.iter().enumerate() {
        let monitor_idx = idx + 1;
        if monitor_idx >= ordered_monitors.len() {
            println!("No monitor available for projector '{}' (only {} monitors)",
                     spec.label, ordered_monitors.len());
            continue;
        }
        let monitor = &ordered_monitors[monitor_idx];
        //println!("Assigning projector '{}' to monitor {}: {:?}", spec.label, idx, monitor.name());

        let fullscreen = Fullscreen::Borderless(Some(monitor.clone()));

        let window = WindowBuilder::new()
            .with_title(format!("TCast {}", spec.label))
            .with_decorations(false)
            .with_resizable(false)
            .with_focused(false)
            .with_focusable(false)
            .with_inner_size(LogicalSize::new(1280.0, 720.0))
            .with_fullscreen(Some(fullscreen))
            .build(target)
            .with_context(|| format!("creating {}", spec.label))?;

        //println!("✓ Window created for '{}'", spec.label);

        let proj_html = ui::projection_html_path(spec, idx);
        let proj_url = format!("asset://{}", proj_html.display());

        let mut webview_builder = WebViewBuilder::new()
            .with_url(&proj_url)
            .with_autoplay(true)
            .with_devtools(cfg!(debug_assertions))
            .with_custom_protocol("asset".into(), |_webview_id, request| {
                let uri = request.uri().to_string();
                let path_str = extract_path_from_uri(&uri);
                let path_str = match path_str.find('?') {
                    Some(pos) => &path_str[..pos],
                    None => &path_str,
                };

                if path_str.is_empty() {
                    return wry::http::Response::builder()
                        .status(400)
                        .header("Content-Type", "text/plain")
                        .body(std::borrow::Cow::Borrowed(b"missing path".as_slice()))
                        .unwrap();
                }

                let path = std::path::Path::new(path_str);
                match std::fs::read(path) {
                    Ok(bytes) => {
                        let mime = mime_for_path(path.to_str().unwrap_or(""));
                        wry::http::Response::builder()
                            .status(200)
                            .header("Content-Type", mime)
                            .header("Content-Length", bytes.len().to_string())
                            .header("Access-Control-Allow-Origin", "*")
                            .body(std::borrow::Cow::Owned(bytes))
                            .unwrap()
                    }
                    Err(e) => {
                        eprintln!("Failed to load asset {:?}: {:?}", path, e);
                        wry::http::Response::builder()
                            .status(404)
                            .header("Content-Type", "text/plain")
                            .body(std::borrow::Cow::Borrowed(b"File not found".as_slice()))
                            .unwrap()
                    }
                }
            });

        #[cfg(target_os = "windows")]
        {
            webview_builder = webview_builder.with_https_scheme(true);
        }

        let webview = webview_builder
            .build(&window)
            .with_context(|| format!("creating webview for {}", spec.label))?;

        windows.push(ProjectionWindow {
            window,
            _webview: webview,
        });
    }

    //println!("=== Created {} projection windows ===", windows.len());
    Ok(windows)
}

fn send_response(webview: &WebView, response: &IpcResponse) -> Result<()> {
    let response_json = serde_json::to_string(response)?;
    let script = format!("window.__TCast && window.__TCast.receive({response_json});");
    webview.evaluate_script(&script)?;
    Ok(())
}

fn init_logging() {
    let filter = EnvFilter::try_from_default_env().unwrap_or_else(|_| EnvFilter::new("info"));
    let _ = tracing_subscriber::fmt()
        .with_env_filter(filter)
        .with_target(false)
        .try_init();
}