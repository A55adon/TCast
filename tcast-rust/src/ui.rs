use crate::model::ProjectionSpec;

const STYLE: &str = include_str!("../ui/styles.css");
const SCRIPT: &str = include_str!("../ui/app.js");

pub fn main_html() -> String {
    format!(
        r#"<!doctype html>
<html lang="de">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>TCast Rust</title>
  <style>{STYLE}</style>
</head>
<body>
  <div id="app"></div>
  <script>{SCRIPT}</script>
</body>
</html>"#
    )
}

pub fn projection_html(spec: &ProjectionSpec) -> String {
    let spec_json = serde_json::to_string(spec).expect("projection spec should serialize");
    format!(
        r#"<!doctype html>
<html lang="de">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>{}</title>
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
    .media {{
      position: absolute;
      top: 0;
      height: 100%;
      object-fit: fill;
      background: #000;
    }}
    .empty {{
      position: fixed;
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
    const span = Math.max(0.0001, spec.end - spec.start);
    const width = 100 / span;
    const left = -spec.start / span * 100;
    let media;
    if (spec.source_url) {{
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
      stage.appendChild(media);
    }} else {{
      const empty = document.createElement('div');
      empty.className = 'empty';
      empty.textContent = spec.label + ' has no source';
      stage.appendChild(empty);
    }}
    document.querySelector('.badge').textContent = spec.label + (spec.is_split ? ` · split ${{Math.round(spec.start*100)}}-${{Math.round(spec.end*100)}}%` : '');
  </script>
</body>
</html>"#,
        spec.label
    )
}
