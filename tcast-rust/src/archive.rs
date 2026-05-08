use anyhow::{Context, Result, anyhow};
use std::fs::{self, File};
use std::io;
use std::path::{Path, PathBuf};
use walkdir::WalkDir;
use zip::write::FileOptions;
use zip::{CompressionMethod, ZipArchive, ZipWriter};

pub fn export_project(project_path: &Path, destination_dir: &Path) -> Result<PathBuf> {
    if !project_path.is_dir() {
        return Err(anyhow!(
            "project path is not a folder: {}",
            project_path.display()
        ));
    }
    fs::create_dir_all(destination_dir)
        .with_context(|| format!("creating {}", destination_dir.display()))?;

    let name = project_path
        .file_name()
        .and_then(|value| value.to_str())
        .ok_or_else(|| anyhow!("project folder has no valid name"))?;
    let output = destination_dir.join(format!("{name}.tct"));
    let file = File::create(&output).with_context(|| format!("creating {}", output.display()))?;
    let mut zip = ZipWriter::new(file);
    let options = FileOptions::default()
        .compression_method(CompressionMethod::Deflated)
        .unix_permissions(0o644);

    for entry in WalkDir::new(project_path) {
        let entry = entry?;
        let path = entry.path();
        let relative = path.strip_prefix(project_path)?;
        if relative.as_os_str().is_empty() {
            continue;
        }

        let archive_name = relative.to_string_lossy().replace('\\', "/");
        if path.is_dir() {
            zip.add_directory(format!("{archive_name}/"), options)?;
        } else {
            zip.start_file(archive_name, options)?;
            let mut input =
                File::open(path).with_context(|| format!("opening {}", path.display()))?;
            io::copy(&mut input, &mut zip)?;
        }
    }

    zip.finish()?;
    Ok(output)
}

pub fn import_project(source_file: &Path, saves_dir: &Path) -> Result<PathBuf> {
    if !source_file.is_file() {
        return Err(anyhow!("archive does not exist: {}", source_file.display()));
    }
    fs::create_dir_all(saves_dir).with_context(|| format!("creating {}", saves_dir.display()))?;

    let stem = source_file
        .file_stem()
        .and_then(|value| value.to_str())
        .ok_or_else(|| anyhow!("archive has no valid filename"))?;
    let extract_path = unique_project_path(saves_dir, stem);
    fs::create_dir_all(&extract_path)
        .with_context(|| format!("creating {}", extract_path.display()))?;

    let file =
        File::open(source_file).with_context(|| format!("opening {}", source_file.display()))?;
    let mut archive = ZipArchive::new(file)?;

    for i in 0..archive.len() {
        let mut file = archive.by_index(i)?;
        let Some(enclosed) = file.enclosed_name().map(|path| path.to_owned()) else {
            continue;
        };
        let out_path = extract_path.join(enclosed);

        if file.name().ends_with('/') {
            fs::create_dir_all(&out_path)?;
        } else {
            if let Some(parent) = out_path.parent() {
                fs::create_dir_all(parent)?;
            }
            let mut out_file = File::create(&out_path)
                .with_context(|| format!("creating {}", out_path.display()))?;
            io::copy(&mut file, &mut out_file)?;
        }
    }

    Ok(extract_path)
}

fn unique_project_path(base: &Path, stem: &str) -> PathBuf {
    let mut candidate = base.join(stem);
    let mut index = 2;
    while candidate.exists() {
        candidate = base.join(format!("{stem} ({index})"));
        index += 1;
    }
    candidate
}
