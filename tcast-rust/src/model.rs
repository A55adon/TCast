use serde::{Deserialize, Serialize};
use std::path::{Path, PathBuf};

pub const APP_VERSION: &str = "2.0.0-rust";

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct SaveData {
    #[serde(rename = "projectName")]
    pub name: String,
    #[serde(rename = "projectorCount")]
    pub projector_amount: usize,
    #[serde(rename = "projectorSettings", default)]
    pub projector_settings: Vec<ProjectorSettings>,
    pub description: String,
    pub path: PathBuf,
    pub version: String,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct ProjectorSettings {
    #[serde(default = "default_projector_aspect")]
    pub aspect: String,
    #[serde(default)]
    pub rotation: u16,
}

impl Default for ProjectorSettings {
    fn default() -> Self {
        Self {
            aspect: default_projector_aspect(),
            rotation: 0,
        }
    }
}

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct SceneData {
    #[serde(rename = "sceneName")]
    pub name: String,
    #[serde(rename = "sources", default)]
    pub sources: Vec<String>,
    #[serde(rename = "splitSources", default)]
    pub split_sources: Vec<String>,
    #[serde(default)]
    pub connections: Vec<i32>,
    #[serde(skip)]
    pub split_info: Vec<SplitInfo>,
}

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct SceneManager {
    #[serde(default)]
    pub scenes: Vec<SceneData>,
}

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct Resource {
    pub id: i32,
    pub path: PathBuf,
    pub name: String,
    #[serde(rename = "isVideo")]
    pub is_video: bool,
    pub thumbnail_id: i32,
}

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct ResourceManager {
    #[serde(rename = "maxId")]
    pub max_id: i32,
    #[serde(default)]
    pub resources: Vec<Resource>,
}

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct SplitInfo {
    #[serde(rename = "resourceId")]
    pub resource_id: i32,
    pub start: f32,
    pub end: f32,
    #[serde(rename = "isSplit")]
    pub is_split: bool,
}

#[derive(Clone, Debug, Serialize)]
pub struct ProjectEntry {
    pub name: String,
    pub path: String,
    pub description: String,
    pub projector_count: usize,
    pub version: String,
}

#[derive(Clone, Debug, Serialize)]
pub struct ResourceView {
    pub id: i32,
    pub path: String,
    pub url: String,
    pub name: String,
    pub is_video: bool,
    pub thumbnail_id: i32,
    pub preview_url: String,
    pub missing: bool,
}

#[derive(Clone, Debug, Serialize)]
pub struct LogEntry {
    pub level: String,
    pub message: String,
}

#[derive(Clone, Debug, Serialize)]
pub struct AppSnapshot {
    pub current_project_path: Option<String>,
    pub save_data: Option<SaveData>,
    pub scenes: Vec<SceneData>,
    pub resources: Vec<ResourceView>,
    pub projection_specs: Option<Vec<ProjectionSpec>>,
    pub active_scene_index: usize,
    pub active_resource_id: Option<i32>,
    pub default_saves_path: String,
    pub projects: Vec<ProjectEntry>,
    pub projection_active: bool,
    pub logs: Vec<LogEntry>,
}

#[derive(Clone, Debug, Serialize)]
pub struct ProjectionSpec {
    pub index: usize,
    pub label: String,
    pub resource_name: String,
    pub source_path: String,
    pub source_url: String,
    pub is_video: bool,
    pub is_split: bool,
    pub start: f32,
    pub end: f32,
    pub aspect: String,
    pub rotation: u16,
}

#[derive(Debug, Deserialize)]
pub struct IpcMessage {
    pub id: String,
    pub command: String,
    #[serde(default)]
    pub payload: serde_json::Value,
}

#[derive(Debug, Serialize)]
pub struct IpcResponse {
    pub id: String,
    pub ok: bool,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub data: Option<serde_json::Value>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub error: Option<String>,
}

pub fn file_url(path: &Path) -> String {
    let raw = path.to_string_lossy();
    let stripped = raw.strip_prefix(r"\\?\").unwrap_or(&raw);
    let s = stripped.replace('\\', "/");
    let s = s.trim_start_matches('/');

    // Windows: "C:/Users/..." → "https://asset.c/Users/..."
    // Matches the format wry uses for its own URL conversion
    if s.len() >= 2 && s.as_bytes()[1] == b':' && s.as_bytes()[0].is_ascii_alphabetic() {
        let drive = (s.as_bytes()[0] as char).to_ascii_lowercase();
        let rest = s[2..].trim_start_matches('/');
        return format!("https://asset.{}/{}", drive, rest);
    }

    format!("https://asset./{}", s)
}

pub fn is_image_path(path: &Path) -> bool {
    matches!(
        normalized_ext(path).as_str(),
        "png" | "jpg" | "jpeg" | "bmp" | "tga" | "gif" | "tif" | "tiff" | "webp"
    )
}

pub fn is_video_path(path: &Path) -> bool {
    matches!(
        normalized_ext(path).as_str(),
        "mp4" | "avi" | "mov" | "mkv" | "flv" | "webm"
    )
}

pub fn normalized_ext(path: &Path) -> String {
    path.extension()
        .and_then(|value| value.to_str())
        .unwrap_or_default()
        .to_ascii_lowercase()
}

pub fn default_projector_aspect() -> String {
    "16:9".to_string()
}

pub fn normalize_projector_aspect(aspect: &str) -> String {
    match aspect.trim() {
        "16:10" => "16:10".to_string(),
        "4:3" => "4:3".to_string(),
        _ => default_projector_aspect(),
    }
}

pub fn normalize_projector_rotation(rotation: u16) -> u16 {
    match rotation % 360 {
        45..=134 => 90,
        135..=224 => 180,
        225..=314 => 270,
        _ => 0,
    }
}

pub fn projector_aspect_value(aspect: &str) -> f32 {
    match normalize_projector_aspect(aspect).as_str() {
        "16:10" => 16.0 / 10.0,
        "4:3" => 4.0 / 3.0,
        _ => 16.0 / 9.0,
    }
}
