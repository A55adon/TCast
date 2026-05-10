use anyhow::{Context, Result, anyhow};
use image::{GenericImageView, ImageFormat, ImageReader, imageops::FilterType};
use std::fs;
use std::path::Path;

pub fn convert_image_to_png(src: &Path, dst: &Path) -> Result<()> {
    if let Some(parent) = dst.parent() {
        fs::create_dir_all(parent).with_context(|| format!("creating {}", parent.display()))?;
    }

    let image = ImageReader::open(src)
        .with_context(|| format!("opening image {}", src.display()))?
        .with_guessed_format()
        .with_context(|| format!("guessing image format {}", src.display()))?
        .decode()
        .with_context(|| format!("decoding image {}", src.display()))?;

    image
        .save_with_format(dst, ImageFormat::Png)
        .with_context(|| format!("writing png {}", dst.display()))?;

    Ok(())
}

pub fn crop_image_part(
    start: f32,
    end: f32,
    src: &Path,
    dst: &Path,
    target_aspect: f32,
) -> Result<()> {
    if !(0.0..1.0).contains(&start) && start != 0.0 {
        return Err(anyhow!("split start must be between 0 and 1"));
    }
    if end <= start || end > 1.0 {
        return Err(anyhow!(
            "split end must be greater than start and at most 1"
        ));
    }

    if let Some(parent) = dst.parent() {
        fs::create_dir_all(parent).with_context(|| format!("creating {}", parent.display()))?;
    }

    let image = ImageReader::open(src)
        .with_context(|| format!("opening image {}", src.display()))?
        .with_guessed_format()
        .with_context(|| format!("guessing image format {}", src.display()))?
        .decode()
        .with_context(|| format!("decoding image {}", src.display()))?;

    let (width, height) = image.dimensions();
    if width == 0 || height == 0 {
        return Err(anyhow!("image has no dimensions"));
    }

    let x0 = ((width as f32) * start)
        .floor()
        .clamp(0.0, width as f32 - 1.0) as u32;
    let x1 = ((width as f32) * end)
        .ceil()
        .clamp((x0 + 1) as f32, width as f32) as u32;
    let crop_width = x1.saturating_sub(x0);
    if crop_width == 0 {
        return Err(anyhow!("split crop width is zero"));
    }

    let target_aspect = target_aspect.clamp(0.1, 10.0);
    let target_height = ((crop_width as f32) / target_aspect).round().max(1.0) as u32;
    let crop_height = target_height.min(height);
    let y0 = height.saturating_sub(crop_height) / 2;

    let cropped = image.crop_imm(x0, y0, crop_width, crop_height);
    let output = if crop_height == target_height {
        cropped
    } else {
        cropped.resize_exact(crop_width, target_height, FilterType::Triangle)
    };

    output
        .save_with_format(dst, ImageFormat::Png)
        .with_context(|| format!("writing split png {}", dst.display()))?;

    Ok(())
}
