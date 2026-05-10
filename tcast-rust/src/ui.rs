use crate::model::ProjectionSpec;
use std::path::PathBuf;

const STYLE: &str = include_str!("../ui/styles.css");
const SCRIPT: &str = include_str!("../ui/app.js");
const LOGO_PNG: &[u8] = include_bytes!("../../assets/t-cast.png");
const FAVICON_PNG: &[u8] = include_bytes!("../../assets/t-cast-favicon.png");

pub fn main_html_path() -> PathBuf {
    let temp_dir = std::env::temp_dir().join("tcast");
    std::fs::create_dir_all(&temp_dir).ok();
    let path = temp_dir.join("main.html");
    let logo_uri = png_data_uri(LOGO_PNG);
    let favicon_uri = png_data_uri(FAVICON_PNG);

    let html = format!(
        r#"<!doctype html>
<html lang="de">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>TCast Rust</title>
  <link rel="icon" type="image/png" href="{favicon_uri}" />
  <style>{STYLE}</style>
</head>
<body>
  <div id="app"></div>
  <script>window.__TCastAssets = {{ logo: "{logo_uri}", favicon: "{favicon_uri}" }};</script>
  <script>{SCRIPT}</script>
</body>
</html>"#
    );

    let write_file = match std::fs::read_to_string(&path) {
        Ok(existing) => existing != html,
        Err(_) => true, // Datei existiert nicht oder kann nicht gelesen werden
    };

    if write_file {
        std::fs::write(&path, &html).expect("Failed to write main.html");
    }
    path
}

pub fn projection_html_path(spec: &ProjectionSpec, index: usize) -> PathBuf {
    let temp_dir = std::env::temp_dir().join("tcast");
    std::fs::create_dir_all(&temp_dir).ok();
    let path = temp_dir.join(format!("projection_{}.html", index));

    let spec_json = serde_json::to_string(spec).expect("projection spec should serialize");
    let favicon_uri = png_data_uri(FAVICON_PNG);
    let html = format!(
        r#"<!doctype html>
<html lang="de">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>{}</title>
  <link rel="icon" type="image/png" href="{favicon_uri}" />
  <style>
    html, body {{
      width: 100%;
      height: 100%;
      margin: 0;
      background: #000;
      overflow: hidden;
      cursor: none;
      font-family: system-ui, sans-serif;
    }}
    #stage {{
      position: fixed;
      inset: 0;
      overflow: hidden;
      background: #000;
    }}
    #frame {{
      position: absolute;
      inset: 0;
      margin: auto;
      overflow: hidden;
      background: #000;
    }}
    .rotator {{
      position: absolute;
      left: 50%;
      top: 50%;
      transform-origin: center;
      overflow: hidden;
    }}
    .media {{
      position: absolute;
      top: 0;
      height: 100%;
      object-fit: fill;
      background: #000;
    }}
    .empty {{
      position: absolute;
      inset: 0;
      display: grid;
      place-items: center;
      color: #6b7280;
      font-size: 22px;
      letter-spacing: 0;
      background: #050505;
    }}
    .badge {{
      position: fixed;
      right: 12px;
      bottom: 10px;
      color: rgba(255,255,255,.28);
      font-size: 12px;
    }}
  </style>
</head>
<body>
  <div id="stage"></div>
  <div class="badge"></div>
  <script>
    const spec = {spec_json};
    const stage = document.getElementById('stage');
    const frame = document.createElement('div');
    frame.id = 'frame';
    stage.appendChild(frame);

    const [aspectW, aspectH] = String(spec.aspect || '16:9').split(':').map(Number);
    const baseAspect = aspectW > 0 && aspectH > 0 ? aspectW / aspectH : 16 / 9;
    const rotation = ((Number(spec.rotation) || 0) % 360 + 360) % 360;
    const isQuarterTurn = rotation === 90 || rotation === 270;
    const effectiveAspect = isQuarterTurn ? 1 / baseAspect : baseAspect;

    function layoutFrame() {{
      const viewportAspect = window.innerWidth / Math.max(1, window.innerHeight);
      if (viewportAspect > effectiveAspect) {{
        frame.style.height = '100vh';
        frame.style.width = (window.innerHeight * effectiveAspect) + 'px';
      }} else {{
        frame.style.width = '100vw';
        frame.style.height = (window.innerWidth / effectiveAspect) + 'px';
      }}
    }}
    window.addEventListener('resize', layoutFrame);
    layoutFrame();

    const span = Math.max(0.0001, spec.end - spec.start);
    const width = 100 / span;
    const left = -spec.start / span * 100;
    let media;
    if (spec.source_url) {{
      const rotator = document.createElement('div');
      rotator.className = 'rotator';
      rotator.style.width = isQuarterTurn ? (baseAspect * 100) + '%' : '100%';
      rotator.style.height = isQuarterTurn ? (100 / baseAspect) + '%' : '100%';
      rotator.style.transform = `translate(-50%, -50%) rotate(${{rotation}}deg)`;
      frame.appendChild(rotator);

      media = document.createElement(spec.is_video ? 'video' : 'img');
      media.className = 'media';
      media.src = spec.source_url;
      media.style.width = width + '%';
      media.style.left = left + '%';
      if (spec.is_video) {{
        media.autoplay = true;
        media.loop = true;
        media.muted = true;
        media.playsInline = true;
        media.addEventListener('canplay', () => media.play().catch(() => {{}}));
      }}
      rotator.appendChild(media);
    }} else {{
      const empty = document.createElement('div');
      empty.className = 'empty';
      empty.textContent = spec.label + ' has no source';
      frame.appendChild(empty);
    }}
    document.querySelector('.badge').textContent = spec.label + ` · ${{spec.aspect || '16:9'}}` + (rotation ? ` · ${{rotation}}°` : '') + (spec.is_split ? ` · split ${{Math.round(spec.start*100)}}-${{Math.round(spec.end*100)}}%` : '');
  </script>
</body>
</html>"#,
        spec.label
    );

    std::fs::write(&path, &html).expect("Failed to write projection HTML");
    path
}

fn png_data_uri(bytes: &[u8]) -> String {
    format!("data:image/png;base64,{}", base64_encode(bytes))
}

fn base64_encode(bytes: &[u8]) -> String {
    const TABLE: &[u8; 64] = b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    let mut out = String::with_capacity(bytes.len().div_ceil(3) * 4);
    for chunk in bytes.chunks(3) {
        let b0 = chunk[0];
        let b1 = *chunk.get(1).unwrap_or(&0);
        let b2 = *chunk.get(2).unwrap_or(&0);

        out.push(TABLE[(b0 >> 2) as usize] as char);
        out.push(TABLE[(((b0 & 0b0000_0011) << 4) | (b1 >> 4)) as usize] as char);
        if chunk.len() > 1 {
            out.push(TABLE[(((b1 & 0b0000_1111) << 2) | (b2 >> 6)) as usize] as char);
        } else {
            out.push('=');
        }
        if chunk.len() > 2 {
            out.push(TABLE[(b2 & 0b0011_1111) as usize] as char);
        } else {
            out.push('=');
        }
    }
    out
}
