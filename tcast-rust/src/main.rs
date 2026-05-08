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

#[derive(Debug, Clone)]
enum UserEvent {
    Ipc(String),
}

struct ProjectionWindow {
    window: Window,
    _webview: WebView,
}

fn main() -> Result<()> {
    init_logging();

    let mut app_state = AppState::new().context("initializing TCast state")?;
    let event_loop = EventLoopBuilder::<UserEvent>::with_user_event().build();
    let proxy = event_loop.create_proxy();

    let control_window = WindowBuilder::new()
        .with_title("TCast Rust")
        .with_inner_size(LogicalSize::new(1280.0, 820.0))
        .with_min_inner_size(LogicalSize::new(960.0, 620.0))
        .build(&event_loop)
        .context("creating control window")?;
    let control_window_id = control_window.id();

    let control_webview = WebViewBuilder::new()
        .with_html(ui::main_html())
        .with_devtools(cfg!(debug_assertions))
        .with_hotkeys_zoom(true)
        .with_ipc_handler(move |request| {
            let _ = proxy.send_event(UserEvent::Ipc(request.body().clone()));
        })
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
            _ => {}
        }
    });
}

fn process_ipc(
    raw: String,
    target: &EventLoopWindowTarget<UserEvent>,
    app_state: &mut AppState,
    control_webview: &WebView,
    projection_windows: &mut Vec<ProjectionWindow>,
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
    let monitors = target.available_monitors().collect::<Vec<_>>();
    let mut windows = Vec::new();

    for spec in specs {
        let monitor = monitors
            .get(spec.index + 1)
            .or_else(|| monitors.get(spec.index))
            .cloned();
        let fullscreen = monitor.map(|monitor| Fullscreen::Borderless(Some(monitor)));

        let window = WindowBuilder::new()
            .with_title(format!("TCast {}", spec.label))
            .with_decorations(false)
            .with_resizable(false)
            .with_focused(false)
            .with_focusable(false)
            .with_inner_size(LogicalSize::new(1280.0, 720.0))
            .with_fullscreen(fullscreen)
            .build(target)
            .with_context(|| format!("creating {}", spec.label))?;

        let webview = WebViewBuilder::new()
            .with_html(ui::projection_html(spec))
            .with_autoplay(true)
            .with_devtools(cfg!(debug_assertions))
            .build(&window)
            .with_context(|| format!("creating webview for {}", spec.label))?;

        windows.push(ProjectionWindow {
            window,
            _webview: webview,
        });
    }

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
