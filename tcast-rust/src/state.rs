use crate::archive;
use crate::media;
use crate::model::{
    APP_VERSION, AppSnapshot, LogEntry, ProjectEntry, ProjectionSpec, Resource, ResourceManager,
    ResourceView, SaveData, SceneData, SceneManager, SplitInfo, file_url, is_image_path,
    is_video_path,
};
use anyhow::{Context, Result, anyhow, bail};
use serde::Deserialize;
use serde_json::{Value, json};
use std::cmp::{max, min};
use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;

#[derive(Debug, Deserialize)]
struct ProjectorCountPayload {
    count: usize,
}

pub struct AppState {
    repo_root: PathBuf,
    default_saves_dir: PathBuf,
    current_project_path: Option<PathBuf>,
    save_data: Option<SaveData>,
    scenes: Vec<SceneData>,
    resources: ResourceManager,
    active_scene_index: usize,
    active_resource_id: Option<i32>,
    logs: Vec<LogEntry>,
    projection_active: bool,

}

impl AppState {
    pub fn new() -> Result<Self> {
        let cwd = std::env::current_dir().context("reading current directory")?;
        let repo_root = if cwd.file_name().and_then(|value| value.to_str()) == Some("tcast-rust") {
            cwd.parent().unwrap_or(&cwd).to_path_buf()
        } else {
            cwd
        };
        let default_saves_dir = repo_root.join("saves").join("folderSaves");
        fs::create_dir_all(&default_saves_dir)
            .with_context(|| format!("creating {}", default_saves_dir.display()))?;

        let mut state = Self {
            repo_root,
            default_saves_dir,
            current_project_path: None,
            save_data: None,
            scenes: Vec::new(),
            resources: ResourceManager::default(),
            active_scene_index: 0,
            active_resource_id: None,
            logs: Vec::new(),
            projection_active: false,
        };

        if let Some(recent) = state.read_recent_path()? {
            if recent.exists() {
                if let Err(error) = state.load_project(&recent) {
                    state.log_error(format!("Could not load recent project: {error:#}"));
                }
            }
        }

        Ok(state)
    }

    fn set_projector_count(&mut self, count: usize) -> Result<()> {
        let count = count.clamp(1, 6);
        let Some(save_data) = &mut self.save_data else {
            bail!("no project loaded");
        };
        save_data.projector_amount = count;
        self.ensure_scene_lengths();
        self.save_project()?;
        self.log_info(format!("Changed projector count to {}", count));
        Ok(())
    }

    pub fn handle_command(&mut self, command: &str, payload: Value) -> Result<Value> {
        match command {
            "boot" | "snapshot" => Ok(json!(self.snapshot())),
            "list_projects" => Ok(json!(self.list_projects())),
            "browse_folder" => self.browse_folder(),
            "browse_project_folder" => self.browse_project_folder(),
            "browse_media_files" => self.browse_media_files(),
            "browse_tct_file" => self.browse_tct_file(),
            "create_project" => {
                let payload: CreateProjectPayload = serde_json::from_value(payload)?;
                self.create_project(payload)?;
                Ok(json!(self.snapshot()))
            }
            "load_project" => {
                let payload: PathPayload = serde_json::from_value(payload)?;
                self.load_project(Path::new(&payload.path))?;
                Ok(json!(self.snapshot()))
            }
            "delete_project" => {
                let payload: PathPayload = serde_json::from_value(payload)?;
                self.delete_project(Path::new(&payload.path))?;
                Ok(json!(self.snapshot()))
            }
            "save_project" => {
                self.save_project()?;
                Ok(json!(self.snapshot()))
            }
            "save_as" => {
                let payload: OptionalPathPayload = serde_json::from_value(payload)?;
                let saved_to = self.save_as(payload.path.as_deref().map(Path::new))?;
                Ok(json!({ "path": saved_to, "snapshot": self.snapshot() }))
            }
            "export_project" => {
                let payload: OptionalPathPayload = serde_json::from_value(payload)?;
                let exported_to = self.export_project(payload.path.as_deref().map(Path::new))?;
                Ok(json!({ "path": exported_to, "snapshot": self.snapshot() }))
            }
            "import_project" => {
                let payload: OptionalPathPayload = serde_json::from_value(payload)?;
                let imported = self.import_project(payload.path.as_deref().map(Path::new))?;
                self.load_project(&imported)?;
                Ok(json!(self.snapshot()))
            }
            "close_project" => {
                self.close_project();
                Ok(json!(self.snapshot()))
            }
            "add_scene" => {
                self.add_scene()?;
                Ok(json!(self.snapshot()))
            }
            "select_scene" => {
                let payload: IndexPayload = serde_json::from_value(payload)?;
                self.select_scene(payload.index)?;
                Ok(json!(self.snapshot()))
            }
            "rename_scene" => {
                let payload: RenameScenePayload = serde_json::from_value(payload)?;
                self.rename_scene(payload.index, payload.name)?;
                Ok(json!(self.snapshot()))
            }
            "delete_scene" => {
                let payload: IndexPayload = serde_json::from_value(payload)?;
                self.delete_scene(payload.index)?;
                Ok(json!(self.snapshot()))
            }
            "duplicate_scene" => {
                let payload: IndexPayload = serde_json::from_value(payload)?;
                self.duplicate_scene(payload.index)?;
                Ok(json!(self.snapshot()))
            }
            "move_scene" => {
                let payload: MoveScenePayload = serde_json::from_value(payload)?;
                self.move_scene(payload.direction)?;
                Ok(json!(self.snapshot()))
            }
            "clear_scenes" => {
                self.clear_scenes()?;
                Ok(json!(self.snapshot()))
            }
            "add_resources" => {
                let payload: AddResourcesPayload = serde_json::from_value(payload)?;
                self.add_resources(payload.paths, payload.names)?;
                Ok(json!(self.snapshot()))
            }
            "rename_resource" => {
                let payload: RenameResourcePayload = serde_json::from_value(payload)?;
                self.rename_resource(payload.id, payload.name)?;
                Ok(json!(self.snapshot()))
            }
            "delete_resource" => {
                let payload: IdPayload = serde_json::from_value(payload)?;
                self.delete_resource(payload.id)?;
                Ok(json!(self.snapshot()))
            }
            "select_resource" => {
                let payload: IdPayload = serde_json::from_value(payload)?;
                self.active_resource_id = Some(payload.id);
                Ok(json!(self.snapshot()))
            }
            "assign_resource" => {
                let payload: AssignResourcePayload = serde_json::from_value(payload)?;
                self.assign_resource(payload.projector_index, payload.resource_id)?;
                Ok(json!(self.snapshot()))
            }
            "set_connection" => {
                let payload: ConnectionPayload = serde_json::from_value(payload)?;
                self.set_connection(payload.index, payload.connected)?;
                Ok(json!(self.snapshot()))
            }
            "open_project_folder" => {
                self.open_project_folder()?;
                Ok(json!(self.snapshot()))
            }
            "set_projector_count" => {
                let payload: ProjectorCountPayload = serde_json::from_value(payload)?;
                self.set_projector_count(payload.count)?;
                Ok(json!(self.snapshot()))
            }
            other => bail!("unknown command: {other}"),
        }
    }

    pub fn snapshot(&self) -> AppSnapshot {
        AppSnapshot {
            current_project_path: self
                .current_project_path
                .as_ref()
                .map(|path| path.to_string_lossy().to_string()),
            save_data: self.save_data.clone(),
            scenes: self.scenes.clone(),
            resources: self.resource_views(),
            active_scene_index: self.active_scene_index,
            active_resource_id: self.active_resource_id,
            default_saves_path: self.default_saves_dir.to_string_lossy().to_string(),
            projects: self.list_projects(),
            projection_active: self.projection_active,
            logs: self.logs.clone(),
        }
    }

    pub fn prepare_projection(&mut self) -> Result<Vec<ProjectionSpec>> {
        self.require_project()?;
        self.regenerate_split_sources()?;
        self.save_project()?;

        let scene = self.active_scene()?;
        let mut specs = Vec::new();
        for index in 0..self.projector_count() {
            let Some(split) = scene.split_info.get(index) else {
                continue;
            };
            if split.resource_id < 0 {
                continue;
            }
            let Some(resource) = self.resource_by_id(split.resource_id).cloned() else {
                continue;
            };
            specs.push(ProjectionSpec {
                index,
                label: format!("Projector {}", index + 1),
                resource_name: resource.name.clone(),
                source_path: resource.path.to_string_lossy().to_string(),
                source_url: file_url(&resource.path),
                is_video: resource.is_video,
                is_split: split.is_split,
                start: split.start,
                end: split.end,
            });
        }

        self.log_info(format!("Prepared {} projector outputs", specs.len()));
        Ok(specs)
    }

    pub fn set_projection_active(&mut self, active: bool) {
        self.projection_active = active;
    }

    pub fn log_info(&mut self, message: impl Into<String>) {
        self.push_log("info", message.into());
    }

    pub fn log_error(&mut self, message: impl Into<String>) {
        self.push_log("error", message.into());
    }

    fn create_project(&mut self, payload: CreateProjectPayload) -> Result<()> {
        validate_text(&payload.name, "project name")?;
        validate_text(&payload.description, "project description")?;

        let projector_amount = payload.projector_count.clamp(1, 6);
        let base_path = PathBuf::from(payload.path);
        fs::create_dir_all(&base_path)
            .with_context(|| format!("creating {}", base_path.display()))?;

        let full_path = base_path.join(&payload.name);
        if full_path.exists() {
            bail!("project already exists at {}", full_path.display());
        }
        fs::create_dir_all(full_path.join("resources").join("images"))?;
        fs::create_dir_all(
            full_path
                .join("resources")
                .join("videos")
                .join("thumbnails"),
        )?;
        fs::create_dir_all(full_path.join("resources").join("splits"))?;

        self.save_data = Some(SaveData {
            name: payload.name.clone(),
            projector_amount,
            description: payload.description,
            path: base_path,
            version: APP_VERSION.to_string(),
        });
        self.current_project_path = Some(full_path.clone());
        self.scenes = vec![SceneData {
            name: payload.name,
            sources: vec![String::new(); projector_amount],
            split_sources: vec![String::new(); projector_amount],
            connections: vec![0; projector_amount],
            split_info: vec![SplitInfo::default(); projector_amount],
        }];
        self.resources = ResourceManager::default();
        self.active_scene_index = 0;
        self.active_resource_id = None;
        self.ensure_scene_lengths();
        self.save_project()?;
        self.write_recent_path(&full_path)?;
        self.log_info(format!("Created project {}", full_path.display()));
        Ok(())
    }

    fn load_project(&mut self, path: &Path) -> Result<()> {
        let project_path = normalize_path(path);
        if !project_path.is_dir() {
            bail!("project folder does not exist: {}", project_path.display());
        }

        let save_path = project_path.join("saveData.json");
        let scenes_path = project_path.join("scenesData.json");
        let resource_path = project_path.join("resourceData.json");

        let mut save_data: SaveData = read_json(&save_path)?;
        if let Some(parent) = project_path.parent() {
            save_data.path = parent.to_path_buf();
        }

        let scene_manager: SceneManager = read_json(&scenes_path)?;
        let resources: ResourceManager =
            if resource_path.exists() && resource_path.metadata()?.len() > 0 {
                read_json(&resource_path)?
            } else {
                ResourceManager::default()
            };

        self.current_project_path = Some(project_path.clone());
        self.save_data = Some(save_data);
        self.scenes = scene_manager.scenes;
        self.resources = resources;
        self.active_scene_index = 0;
        self.active_resource_id = None;
        self.ensure_scene_lengths();
        self.remove_missing_resources()?;
        self.regenerate_split_sources()?;
        self.save_project()?;
        self.write_recent_path(&project_path)?;
        self.log_info(format!("Loaded project {}", project_path.display()));
        Ok(())
    }

    fn save_project(&mut self) -> Result<()> {
        let project_path = self.project_path()?;
        self.ensure_project_folders()?;
        self.ensure_scene_lengths();

        let save_data = self
            .save_data
            .as_ref()
            .ok_or_else(|| anyhow!("no project loaded"))?;
        write_json(&project_path.join("saveData.json"), save_data)?;
        write_json(
            &project_path.join("scenesData.json"),
            &SceneManager {
                scenes: self.scenes.clone(),
            },
        )?;
        write_json(&project_path.join("resourceData.json"), &self.resources)?;
        Ok(())
    }

    fn save_as(&mut self, destination: Option<&Path>) -> Result<String> {
        let project_path = self.project_path()?;
        let Some(save_data) = self.save_data.as_ref() else {
            bail!("no project loaded");
        };
        let destination = match destination {
            Some(path) => path.to_path_buf(),
            None => rfd::FileDialog::new()
                .set_title("Save project copy")
                .pick_folder()
                .ok_or_else(|| anyhow!("save-as cancelled"))?,
        };
        fs::create_dir_all(&destination)?;
        let target = destination.join(&save_data.name);
        if target.exists() {
            bail!("target already exists: {}", target.display());
        }
        copy_dir_all(&project_path, &target)?;
        self.log_info(format!("Saved project copy to {}", target.display()));
        Ok(target.to_string_lossy().to_string())
    }

    fn export_project(&mut self, destination: Option<&Path>) -> Result<String> {
        let project_path = self.project_path()?;
        self.save_project()?;
        let destination = match destination {
            Some(path) => path.to_path_buf(),
            None => rfd::FileDialog::new()
                .set_title("Export TCast project")
                .pick_folder()
                .ok_or_else(|| anyhow!("export cancelled"))?,
        };
        let output = archive::export_project(&project_path, &destination)?;
        self.log_info(format!("Exported {}", output.display()));
        Ok(output.to_string_lossy().to_string())
    }

    fn import_project(&mut self, source: Option<&Path>) -> Result<PathBuf> {
        let source = match source {
            Some(path) => path.to_path_buf(),
            None => rfd::FileDialog::new()
                .set_title("Import TCast project")
                .add_filter("TCast project", &["tct"])
                .pick_file()
                .ok_or_else(|| anyhow!("import cancelled"))?,
        };
        let imported = archive::import_project(&source, &self.default_saves_dir)?;
        self.log_info(format!("Imported {}", imported.display()));
        Ok(imported)
    }

    fn delete_project(&mut self, path: &Path) -> Result<()> {
        let path = normalize_path(path);
        //if !path.starts_with(&self.default_saves_dir) {
        //    bail!(
        //        "refusing to delete a project outside {}",
        //        self.default_saves_dir.display()
        //    );
        //}
        if path.is_dir() {
            fs::remove_dir_all(&path).with_context(|| format!("deleting {}", path.display()))?;
        }
        if self.current_project_path.as_deref() == Some(path.as_path()) {
            self.close_project();
        }
        self.log_info(format!("Deleted project {}", path.display()));
        Ok(())
    }

    fn close_project(&mut self) {
        self.current_project_path = None;
        self.save_data = None;
        self.scenes.clear();
        self.resources = ResourceManager::default();
        self.active_scene_index = 0;
        self.active_resource_id = None;
        self.projection_active = false;
        self.log_info("Closed project");
    }

    fn add_scene(&mut self) -> Result<()> {
        self.require_project()?;
        let count = self.projector_count();
        let name = format!("Szene {}", self.scenes.len() + 1);
        self.scenes.push(SceneData {
            name,
            sources: vec![String::new(); count],
            split_sources: vec![String::new(); count],
            connections: vec![0; count],
            split_info: vec![SplitInfo::default(); count],
        });
        self.active_scene_index = self.scenes.len() - 1;
        self.save_project()?;
        Ok(())
    }

    fn select_scene(&mut self, index: usize) -> Result<()> {
        if index >= self.scenes.len() {
            bail!("scene index out of range");
        }
        self.active_scene_index = index;
        self.regenerate_split_sources()?;
        Ok(())
    }

    fn rename_scene(&mut self, index: usize, name: String) -> Result<()> {
        validate_text(&name, "scene name")?;
        let scene = self
            .scenes
            .get_mut(index)
            .ok_or_else(|| anyhow!("scene index out of range"))?;
        scene.name = name;
        self.save_project()?;
        Ok(())
    }

    fn delete_scene(&mut self, index: usize) -> Result<()> {
        if index >= self.scenes.len() {
            bail!("scene index out of range");
        }
        self.scenes.remove(index);
        if self.scenes.is_empty() {
            let count = self.projector_count();
            self.scenes.push(SceneData {
                name: "default scene".to_string(),
                sources: vec![String::new(); count],
                split_sources: vec![String::new(); count],
                connections: vec![0; count],
                split_info: vec![SplitInfo::default(); count],
            });
        }
        self.active_scene_index = min(self.active_scene_index, self.scenes.len() - 1);
        self.regenerate_split_sources()?;
        self.save_project()?;
        Ok(())
    }

    fn duplicate_scene(&mut self, index: usize) -> Result<()> {
        let source = self
            .scenes
            .get(index)
            .cloned()
            .ok_or_else(|| anyhow!("scene index out of range"))?;
        let base = copy_base_name(&source.name);
        let mut candidate = format!("{base} (Copy)");
        let mut copy_index = 2;
        while self.scenes.iter().any(|scene| scene.name == candidate) {
            candidate = format!("{base} (Copy {copy_index})");
            copy_index += 1;
        }
        let mut duplicate = source;
        duplicate.name = candidate;
        self.scenes.insert(index + 1, duplicate);
        self.active_scene_index = index + 1;
        self.save_project()?;
        Ok(())
    }

    fn move_scene(&mut self, direction: i32) -> Result<()> {
        if self.scenes.is_empty() {
            return Ok(());
        }
        let current = self.active_scene_index;
        let target = if direction < 0 {
            current.saturating_sub(1)
        } else {
            min(current + 1, self.scenes.len() - 1)
        };
        if current != target {
            self.scenes.swap(current, target);
            self.active_scene_index = target;
            self.save_project()?;
        }
        Ok(())
    }

    fn clear_scenes(&mut self) -> Result<()> {
        let count = self.projector_count();
        self.scenes = vec![SceneData {
            name: "default scene".to_string(),
            sources: vec![String::new(); count],
            split_sources: vec![String::new(); count],
            connections: vec![0; count],
            split_info: vec![SplitInfo::default(); count],
        }];
        self.active_scene_index = 0;
        self.save_project()?;
        Ok(())
    }

    fn add_resources(&mut self, paths: Vec<String>, names: Vec<String>) -> Result<()> {
        self.require_project()?;
        if paths.is_empty() {
            bail!("no files selected");
        }

        for (index, raw_path) in paths.iter().enumerate() {
            let source = PathBuf::from(raw_path);
            if !source.is_file() {
                bail!("resource file does not exist: {}", source.display());
            }
            let name = names
                .get(index)
                .filter(|name| !name.trim().is_empty())
                .cloned()
                .unwrap_or_else(|| {
                    source
                        .file_stem()
                        .and_then(|value| value.to_str())
                        .unwrap_or("Resource")
                        .to_string()
                });
            validate_text(&name, "resource name")?;
            self.create_resource(&source, name)?;
        }

        self.save_project()?;
        Ok(())
    }

    fn create_resource(&mut self, source: &Path, name: String) -> Result<()> {
        let id = self.resources.max_id;
        self.resources.max_id += 1;
        let serial = self.resources.max_id;

        let mut resource = Resource {
            id,
            path: PathBuf::new(),
            name,
            is_video: false,
            thumbnail_id: -1,
        };

        if is_video_path(source) {
            resource.is_video = true;
            let ext = source
                .extension()
                .and_then(|value| value.to_str())
                .unwrap_or("mp4")
                .to_ascii_lowercase();
            let dst = self
                .video_dir()?
                .join(format!("video_{ext}_{serial}.{ext}"));
            fs::create_dir_all(dst.parent().unwrap())?;
            fs::copy(source, &dst).with_context(|| {
                format!("copying video {} to {}", source.display(), dst.display())
            })?;
            resource.path = dst;
        } else if is_image_path(source) {
            let dst = self.image_dir()?.join(format!("image_png_{serial}.png"));
            media::convert_image_to_png(source, &dst)?;
            resource.path = dst;
        } else {
            bail!("unsupported resource type: {}", source.display());
        }

        self.log_info(format!("Imported resource {}", resource.name));
        self.resources.resources.push(resource);
        Ok(())
    }

    fn rename_resource(&mut self, id: i32, name: String) -> Result<()> {
        validate_text(&name, "resource name")?;
        let resource = self
            .resources
            .resources
            .iter_mut()
            .find(|resource| resource.id == id)
            .ok_or_else(|| anyhow!("resource not found"))?;
        resource.name = name;
        self.save_project()?;
        Ok(())
    }

    fn delete_resource(&mut self, id: i32) -> Result<()> {
        let Some(resource) = self.resource_by_id(id).cloned() else {
            return Ok(());
        };
        remove_file_if_exists(&resource.path)?;
        if resource.thumbnail_id >= 0 {
            if let Some(thumb) = self.resource_by_id(resource.thumbnail_id).cloned() {
                remove_file_if_exists(&thumb.path)?;
            }
        }

        let thumbnail_id = resource.thumbnail_id;
        self.resources
            .resources
            .retain(|resource| resource.id != id && resource.id != thumbnail_id);

        let deleted_path = resource.path.to_string_lossy().to_string();
        for scene in &mut self.scenes {
            for source in &mut scene.sources {
                if *source == deleted_path {
                    source.clear();
                }
            }
            for split_source in &mut scene.split_sources {
                if *split_source == deleted_path {
                    split_source.clear();
                }
            }
            for connection in &mut scene.connections {
                *connection = 0;
            }
        }

        if self.active_resource_id == Some(id) {
            self.active_resource_id = None;
        }

        self.regenerate_split_sources()?;
        self.save_project()?;
        Ok(())
    }

    fn assign_resource(&mut self, projector_index: usize, resource_id: Option<i32>) -> Result<()> {
        self.require_project()?;
        let count = self.projector_count();
        if projector_index >= count {
            bail!("projector index out of range");
        }

        let path = match resource_id {
            Some(id) => self
                .resource_by_id(id)
                .ok_or_else(|| anyhow!("resource not found"))?
                .path
                .to_string_lossy()
                .to_string(),
            None => String::new(),
        };

        let scene = self.active_scene_mut()?;
        scene.sources.resize(count, String::new());
        scene.sources[projector_index] = path;
        if resource_id.is_none() {
            if projector_index < scene.connections.len() {
                scene.connections[projector_index] = 0;
            }
            if projector_index > 0 {
                scene.connections[projector_index - 1] = 0;
            }
        }

        self.regenerate_split_sources()?;
        self.save_project()?;
        Ok(())
    }

    fn set_connection(&mut self, index: usize, connected: bool) -> Result<()> {
        let count = self.projector_count();
        if index >= count.saturating_sub(1) {
            bail!("connection index out of range");
        }
        let scene = self.active_scene_mut()?;
        scene.connections.resize(count, 0);
        if connected {
            scene.sources.resize(count, String::new());
            if scene
                .sources
                .get(index)
                .is_some_and(|source| source.is_empty())
            {
                bail!("left projector needs a source before connecting");
            }
        }
        scene.connections[index] = i32::from(connected);
        self.regenerate_split_sources()?;
        self.save_project()?;
        Ok(())
    }

    fn regenerate_split_sources(&mut self) -> Result<()> {
        if self.scenes.is_empty() || self.save_data.is_none() {
            return Ok(());
        }
        self.ensure_scene_lengths();

        let count = self.projector_count();
        let active_index = self.active_scene_index;
        let split_dir = self.split_dir()?;
        fs::create_dir_all(&split_dir)?;
        clear_scene_splits(&split_dir, active_index)?;

        let mut scene = self.scenes[active_index].clone();
        scene.sources.resize(count, String::new());
        scene.split_sources = vec![String::new(); count];
        scene.split_info = vec![SplitInfo::default(); count];

        let mut projector = 0;
        while projector < count {
            let group_start = projector;
            let mut group_len = 1;
            while group_start + group_len - 1 < count.saturating_sub(1)
                && scene.connections[group_start + group_len - 1] == 1
            {
                group_len += 1;
            }

            let left_source = scene.sources[group_start].clone();
            for offset in 0..group_len {
                scene.sources[group_start + offset] = left_source.clone();
            }

            if left_source.is_empty() {
                scene.split_info[group_start] = SplitInfo {
                    resource_id: -1,
                    start: 0.0,
                    end: 1.0,
                    is_split: false,
                };
                projector += group_len;
                continue;
            }

            let source_path = PathBuf::from(&left_source);
            let resource_id = self.resource_id_for_source(&left_source).unwrap_or(-1);

            if group_len > 1 {
                for offset in 0..group_len {
                    let start = offset as f32 / group_len as f32;
                    let end = (offset + 1) as f32 / group_len as f32;
                    let target = group_start + offset;
                    let mut preview = left_source.clone();

                    if is_image_path(&source_path) && source_path.exists() {
                        let out = split_dir.join(format!(
                            "scene_{}_proj_{}_{}.png",
                            active_index, group_start, target
                        ));
                        if let Err(error) = media::crop_image_part(start, end, &source_path, &out) {
                            self.log_error(format!("Could not create split preview: {error:#}"));
                        } else {
                            preview = out.to_string_lossy().to_string();
                        }
                    }

                    scene.split_sources[target] = preview;
                    scene.split_info[target] = SplitInfo {
                        resource_id,
                        start,
                        end,
                        is_split: true,
                    };
                }
            } else {
                scene.split_sources[group_start] = left_source;
                scene.split_info[group_start] = SplitInfo {
                    resource_id,
                    start: 0.0,
                    end: 1.0,
                    is_split: false,
                };
            }

            projector += group_len;
        }

        self.scenes[active_index] = scene;
        Ok(())
    }

    fn resource_views(&self) -> Vec<ResourceView> {
        self.resources
            .resources
            .iter()
            .filter(|resource| !(resource.name.ends_with("_thumbnail") && !resource.is_video))
            .map(|resource| {
                let preview_path = if resource.is_video {
                    self.resource_by_id(resource.thumbnail_id)
                        .map(|thumb| thumb.path.clone())
                        .unwrap_or_else(|| resource.path.clone())
                } else {
                    resource.path.clone()
                };
                ResourceView {
                    id: resource.id,
                    path: resource.path.to_string_lossy().to_string(),
                    url: file_url(&resource.path),
                    name: resource.name.clone(),
                    is_video: resource.is_video,
                    thumbnail_id: resource.thumbnail_id,
                    preview_url: file_url(&preview_path),
                    missing: !resource.path.exists(),
                }
            })
            .collect()
    }

    fn list_projects(&self) -> Vec<ProjectEntry> {
        let mut projects = Vec::new();
        let Ok(entries) = fs::read_dir(&self.default_saves_dir) else {
            return projects;
        };

        for entry in entries.flatten() {
            let path = entry.path();
            if !path.is_dir() {
                continue;
            }
            let save_path = path.join("saveData.json");
            if !save_path.exists() {
                continue;
            }
            if let Ok(save_data) = read_json::<SaveData>(&save_path) {
                projects.push(ProjectEntry {
                    name: save_data.name,
                    path: path.to_string_lossy().to_string(),
                    description: save_data.description,
                    projector_count: save_data.projector_amount,
                    version: save_data.version,
                });
            }
        }
        projects.sort_by(|a, b| {
            a.name
                .to_ascii_lowercase()
                .cmp(&b.name.to_ascii_lowercase())
        });
        projects
    }

    fn remove_missing_resources(&mut self) -> Result<()> {
        let before = self.resources.resources.len();
        self.resources
            .resources
            .retain(|resource| resource.path.exists());
        let removed = before.saturating_sub(self.resources.resources.len());
        if removed > 0 {
            self.log_info(format!(
                "Removed {removed} missing resources from project data"
            ));
        }
        Ok(())
    }

    fn ensure_scene_lengths(&mut self) {
        let count = max(1, self.projector_count());
        if self.scenes.is_empty() && self.save_data.is_some() {
            self.scenes.push(SceneData {
                name: "default scene".to_string(),
                sources: vec![String::new(); count],
                split_sources: vec![String::new(); count],
                connections: vec![0; count],
                split_info: vec![SplitInfo::default(); count],
            });
        }

        for scene in &mut self.scenes {
            scene.sources.resize(count, String::new());
            scene.split_sources.resize(count, String::new());
            scene.connections.resize(count, 0);
            scene.split_info.resize(count, SplitInfo::default());
        }
        if !self.scenes.is_empty() {
            self.active_scene_index = min(self.active_scene_index, self.scenes.len() - 1);
        }
    }

    fn ensure_project_folders(&self) -> Result<()> {
        fs::create_dir_all(self.image_dir()?)?;
        fs::create_dir_all(self.video_dir()?)?;
        fs::create_dir_all(self.thumbnail_dir()?)?;
        fs::create_dir_all(self.split_dir()?)?;
        Ok(())
    }

    fn browse_folder(&self) -> Result<Value> {
        let path = rfd::FileDialog::new()
            .set_title("Choose folder")
            .pick_folder()
            .map(|path| path.to_string_lossy().to_string());
        Ok(json!({ "path": path }))
    }

    fn browse_project_folder(&self) -> Result<Value> {
        let path = rfd::FileDialog::new()
            .set_title("Choose project folder")
            .set_directory(&self.default_saves_dir)
            .pick_folder()
            .map(|path| path.to_string_lossy().to_string());
        Ok(json!({ "path": path }))
    }

    fn browse_media_files(&self) -> Result<Value> {
        let files = rfd::FileDialog::new()
            .set_title("Choose images or videos")
            .add_filter(
                "Media",
                &[
                    "png", "jpg", "jpeg", "bmp", "tga", "gif", "tif", "tiff", "webp", "mp4", "avi",
                    "mov", "mkv", "flv", "webm",
                ],
            )
            .pick_files()
            .unwrap_or_default()
            .into_iter()
            .map(|path| path.to_string_lossy().to_string())
            .collect::<Vec<_>>();
        Ok(json!({ "paths": files }))
    }

    fn browse_tct_file(&self) -> Result<Value> {
        let path = rfd::FileDialog::new()
            .set_title("Choose TCast project")
            .add_filter("TCast project", &["tct"])
            .pick_file()
            .map(|path| path.to_string_lossy().to_string());
        Ok(json!({ "path": path }))
    }

    fn open_project_folder(&self) -> Result<()> {
        let path = self.project_path()?;
        #[cfg(target_os = "windows")]
        {
            Command::new("explorer")
                .arg(path)
                .spawn()
                .context("opening project folder in Explorer")?;
        }
        #[cfg(target_os = "macos")]
        {
            Command::new("open")
                .arg(path)
                .spawn()
                .context("opening project folder")?;
        }
        #[cfg(all(unix, not(target_os = "macos")))]
        {
            Command::new("xdg-open")
                .arg(path)
                .spawn()
                .context("opening project folder")?;
        }
        Ok(())
    }

    fn project_path(&self) -> Result<PathBuf> {
        self.current_project_path
            .clone()
            .ok_or_else(|| anyhow!("no project loaded"))
    }

    fn require_project(&self) -> Result<()> {
        if self.current_project_path.is_none() || self.save_data.is_none() {
            bail!("no project loaded");
        }
        Ok(())
    }

    fn projector_count(&self) -> usize {
        self.save_data
            .as_ref()
            .map(|data| data.projector_amount.clamp(1, 6))
            .unwrap_or(1)
    }

    fn image_dir(&self) -> Result<PathBuf> {
        Ok(self.project_path()?.join("resources").join("images"))
    }

    fn video_dir(&self) -> Result<PathBuf> {
        Ok(self.project_path()?.join("resources").join("videos"))
    }

    fn thumbnail_dir(&self) -> Result<PathBuf> {
        Ok(self.video_dir()?.join("thumbnails"))
    }

    fn split_dir(&self) -> Result<PathBuf> {
        Ok(self.project_path()?.join("resources").join("splits"))
    }

    fn active_scene(&self) -> Result<&SceneData> {
        self.scenes
            .get(self.active_scene_index)
            .ok_or_else(|| anyhow!("active scene missing"))
    }

    fn active_scene_mut(&mut self) -> Result<&mut SceneData> {
        self.scenes
            .get_mut(self.active_scene_index)
            .ok_or_else(|| anyhow!("active scene missing"))
    }

    fn resource_by_id(&self, id: i32) -> Option<&Resource> {
        self.resources
            .resources
            .iter()
            .find(|resource| resource.id == id)
    }

    fn resource_id_for_source(&self, source: &str) -> Option<i32> {
        let source_path = PathBuf::from(source);
        if let Some(resource) = self
            .resources
            .resources
            .iter()
            .find(|resource| paths_equal(&resource.path, &source_path))
        {
            return Some(resource.id);
        }

        self.resources.resources.iter().find_map(|resource| {
            let thumb = self.resource_by_id(resource.thumbnail_id)?;
            paths_equal(&thumb.path, &source_path).then_some(resource.id)
        })
    }

    fn read_recent_path(&self) -> Result<Option<PathBuf>> {
        let path = self.recent_path_file();
        if !path.exists() {
            return Ok(None);
        }
        let value = fs::read_to_string(path)?;
        let trimmed = value.trim();
        if trimmed.is_empty() {
            Ok(None)
        } else {
            Ok(Some(PathBuf::from(trimmed)))
        }
    }

    fn write_recent_path(&self, project_path: &Path) -> Result<()> {
        let recent_path = self.recent_path_file();
        if let Some(parent) = recent_path.parent() {
            fs::create_dir_all(parent)?;
        }
        fs::write(recent_path, project_path.to_string_lossy().as_bytes())?;
        Ok(())
    }

    fn recent_path_file(&self) -> PathBuf {
        self.repo_root.join("saves").join("recent.path")
    }

    fn push_log(&mut self, level: &str, message: String) {
        if level == "error" {
            tracing::error!("{message}");
        } else {
            tracing::info!("{message}");
        }
        self.logs.push(LogEntry {
            level: level.to_string(),
            message,
        });
        if self.logs.len() > 200 {
            self.logs.remove(0);
        }
    }
}

#[derive(Debug, Deserialize)]
struct CreateProjectPayload {
    name: String,
    #[serde(rename = "projectorCount")]
    projector_count: usize,
    description: String,
    path: String,
}

#[derive(Debug, Deserialize)]
struct PathPayload {
    path: String,
}

#[derive(Debug, Deserialize)]
struct OptionalPathPayload {
    path: Option<String>,
}

#[derive(Debug, Deserialize)]
struct IndexPayload {
    index: usize,
}

#[derive(Debug, Deserialize)]
struct IdPayload {
    id: i32,
}

#[derive(Debug, Deserialize)]
struct RenameScenePayload {
    index: usize,
    name: String,
}

#[derive(Debug, Deserialize)]
struct MoveScenePayload {
    direction: i32,
}

#[derive(Debug, Deserialize)]
struct AddResourcesPayload {
    paths: Vec<String>,
    #[serde(default)]
    names: Vec<String>,
}

#[derive(Debug, Deserialize)]
struct RenameResourcePayload {
    id: i32,
    name: String,
}

#[derive(Debug, Deserialize)]
struct AssignResourcePayload {
    #[serde(rename = "projectorIndex")]
    projector_index: usize,
    #[serde(rename = "resourceId")]
    resource_id: Option<i32>,
}

#[derive(Debug, Deserialize)]
struct ConnectionPayload {
    index: usize,
    connected: bool,
}

fn read_json<T: for<'de> serde::Deserialize<'de>>(path: &Path) -> Result<T> {
    let text = fs::read_to_string(path).with_context(|| format!("reading {}", path.display()))?;
    serde_json::from_str(&text).with_context(|| format!("parsing {}", path.display()))
}

fn write_json<T: serde::Serialize>(path: &Path, data: &T) -> Result<()> {
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent)?;
    }
    let text = serde_json::to_string_pretty(data)?;
    fs::write(path, text).with_context(|| format!("writing {}", path.display()))
}

fn validate_text(value: &str, field: &str) -> Result<()> {
    if value.trim().is_empty() {
        bail!("{field} cannot be empty");
    }
    if value
        .chars()
        .any(|c| matches!(c, '<' | '>' | ':' | '"' | '/' | '\\' | '|' | '?' | '*'))
    {
        bail!("{field} contains characters that are unsafe for project files");
    }
    Ok(())
}

fn normalize_path(path: &Path) -> PathBuf {
    match path.canonicalize() {
        Ok(canon) => {
            let s = canon.to_string_lossy();
            PathBuf::from(s.strip_prefix(r"\\?\").unwrap_or(&s).to_string())
        }
        Err(_) => path.to_path_buf(),
    }
}

fn paths_equal(left: &Path, right: &Path) -> bool {
    normalize_path(left) == normalize_path(right)
}

fn remove_file_if_exists(path: &Path) -> Result<()> {
    if path.exists() && path.is_file() {
        fs::remove_file(path).with_context(|| format!("deleting {}", path.display()))?;
    }
    Ok(())
}

fn clear_scene_splits(dir: &Path, scene_index: usize) -> Result<()> {
    if !dir.exists() {
        return Ok(());
    }
    let marker = format!("scene_{scene_index}_");
    for entry in fs::read_dir(dir)? {
        let entry = entry?;
        let path = entry.path();
        if path
            .file_name()
            .and_then(|value| value.to_str())
            .is_some_and(|name| name.contains(&marker))
        {
            if path.is_file() {
                fs::remove_file(path)?;
            }
        }
    }
    Ok(())
}

fn copy_base_name(name: &str) -> String {
    if let Some(stripped) = name.strip_suffix(" (Copy)") {
        return stripped.to_string();
    }
    if let Some(start) = name.rfind(" (Copy ") {
        if name.ends_with(')') {
            let number = &name[start + 7..name.len() - 1];
            if number.chars().all(|ch| ch.is_ascii_digit()) {
                return name[..start].to_string();
            }
        }
    }
    name.to_string()
}

fn copy_dir_all(src: &Path, dst: &Path) -> Result<()> {
    fs::create_dir_all(dst)?;
    for entry in fs::read_dir(src)? {
        let entry = entry?;
        let ty = entry.file_type()?;
        let target = dst.join(entry.file_name());
        if ty.is_dir() {
            copy_dir_all(&entry.path(), &target)?;
        } else {
            fs::copy(entry.path(), target)?;
        }
    }
    Ok(())
}
