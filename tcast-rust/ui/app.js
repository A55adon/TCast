const appState = {
  snapshot: null,
  startupTab: "new",
  selectedProjectPath: "",
  modal: null,
  diagnostics: false,
  pending: new Map(),
  nextId: 1,
};

const PROJECTOR_ASPECTS = ["16:9", "16:10", "4:3"];
const PROJECTOR_ROTATIONS = [0, 90, 180, 270];

function invoke(command, payload = {}) {
  const id = String(appState.nextId++);
  const message = JSON.stringify({ id, command, payload });
  return new Promise((resolve, reject) => {
    appState.pending.set(id, { resolve, reject });
    window.ipc.postMessage(message);
  });
}

window.__TCast = {
  receive(response) {
    const pending = appState.pending.get(response.id);
    if (!pending) return;
    appState.pending.delete(response.id);
    if (response.ok) pending.resolve(response.data);
    else pending.reject(new Error(response.error || "Unknown error"));
  },
};

function h(value) {
  return String(value ?? "")
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#039;");
}

function pathToFileUrl(path) {
  if (!path) return "";
  if (/^https?:\/\//i.test(path)) return path;

  let normalized = path
      .replace(/^\\\\\?\\/, "")
      .replaceAll("\\", "/")
      .replace(/^\//, "");

  // Windows drive letter: C:/path → https://asset.c/path
  if (normalized[1] === ":") {
    const drive = normalized[0].toLowerCase();
    const rest = normalized.slice(2).replace(/^\//, "");
    return `https://asset.${drive}/${rest}`;
  }

  return `https://asset./${normalized}`;
}

function shortPath(path) {
  if (!path) return "";
  const normalized = path.replaceAll("\\", "/");
  const parts = normalized.split("/");
  return parts.slice(-3).join("/");
}

function appAsset(name) {
  return window.__TCastAssets?.[name] || "";
}

function projectorSettings(index) {
  const settings = appState.snapshot?.save_data?.projectorSettings?.[index]
      || appState.snapshot?.save_data?.projector_settings?.[index]
      || {};
  const aspect = PROJECTOR_ASPECTS.includes(settings.aspect) ? settings.aspect : "16:9";
  const rotation = PROJECTOR_ROTATIONS.includes(Number(settings.rotation)) ? Number(settings.rotation) : 0;
  return { aspect, rotation };
}

function aspectValue(aspect) {
  if (aspect === "16:10") return 16 / 10;
  if (aspect === "4:3") return 4 / 3;
  return 16 / 9;
}

function effectiveAspectValue(settings) {
  const base = aspectValue(settings.aspect);
  return settings.rotation % 180 === 90 ? 1 / base : base;
}

function effectiveAspectCss(settings) {
  const [w, h] = settings.aspect.split(":").map(Number);
  if (settings.rotation % 180 === 90) return `${h} / ${w}`;
  return `${w} / ${h}`;
}

function projectorRotatorStyle(settings) {
  const base = aspectValue(settings.aspect);
  const quarter = settings.rotation % 180 === 90;
  const width = quarter ? `${base * 100}%` : "100%";
  const height = quarter ? `${100 / base}%` : "100%";
  return `width:${width};height:${height};transform:translate(-50%, -50%) rotate(${settings.rotation}deg);`;
}

function showToast(message, type = "info") {
  const wrap = ensureToastWrap();
  const toast = document.createElement("button");
  toast.className = `toast ${type}`;
  toast.type = "button";
  toast.innerHTML = `
    <span class="toast-mark">${type === "error" ? "!" : "i"}</span>
    <span class="toast-body">
      <span class="toast-title">${type === "error" ? "Fehler" : "TCast"}</span>
      <span class="toast-message">${h(message)}</span>
    </span>
    <span class="toast-dismiss" aria-hidden="true">
      <svg xmlns="http://www.w3.org/2000/svg" width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.4" stroke-linecap="round" stroke-linejoin="round">
        <path d="M18 6 6 18"/>
        <path d="m6 6 12 12"/>
      </svg>
    </span>
  `;
  toast.addEventListener("click", () => toast.remove());
  wrap.appendChild(toast);
  setTimeout(() => toast.remove(), 7000);
}

function ensureToastWrap() {
  let wrap = document.querySelector(".toast-wrap");
  if (!wrap) {
    wrap = document.createElement("div");
    wrap.className = "toast-wrap";
    document.body.appendChild(wrap);
  }
  return wrap;
}

async function run(command, payload = {}, options = {}) {
  try {
    const data = await invoke(command, payload);
    const snapshot = data?.snapshot || data;
    if (snapshot?.default_saves_path !== undefined) {
      appState.snapshot = snapshot;
      render();
    }
    if (options.message) showToast(options.message);
    return data;
  } catch (error) {
    showToast(error.message, "error");
    throw error;
  }
}

function bindTitleBar() {
  // Window control buttons
  document.getElementById("minimize-window")?.addEventListener("click", () => {
    invoke("minimize_window");
  });

  document.getElementById("maximize-window")?.addEventListener("click", () => {
    invoke("maximize_window");
  });

  document.getElementById("close-window")?.addEventListener("click", () => {
    // Stop projection if running, then close
    invoke("stop_projection").finally(() => {
      invoke("close_window");
    });
  });

  window.addEventListener("resize", () => {
    updateMaximizeButton();
  });

  // Double-click on title bar to maximize
  document.querySelector(".title-bar")?.addEventListener("dblclick", (e) => {
    // Only maximize if double-clicking on the draggable area
    if (
        e.target.closest(".title-bar-center") ||
        (!e.target.closest("button") &&
            !e.target.closest(".menu-item") &&
            !e.target.closest(".dropdown"))
    ) {
      invoke("maximize_window");
    }
  });

  // Setup window dragging
  setupTitleBarDrag();

  // Bind menu items in title bar
  document.querySelectorAll("[data-menu]").forEach(button => {
    button.addEventListener("click", () => handleMenu(button.dataset.menu));
  });

  document.querySelectorAll("[data-diagnostic]").forEach(button => {
    button.addEventListener("click", () => {
      appState.diagnostics = button.dataset.diagnostic;
      render();
    });
  });
}

function render() {
  const root = document.getElementById("app");
  const snap = appState.snapshot;
  if (!snap?.current_project_path) {
    root.innerHTML = renderStartup();
    bindTitleBar();
    bindStartup();
    setTimeout(updateMaximizeButton, 100); // Short delay to ensure DOM is ready
  } else {
    root.innerHTML = renderWorkspace();
    bindWorkspace();
    bindTitleBar();
    setTimeout(updateMaximizeButton, 100);
  }
}

function renderStartup() {
  const snap = appState.snapshot || { projects: [], default_saves_path: "" };
  const projects = snap.projects || [];
  return `
    <div class="app-shell">
      ${renderTitleBar()}
      <main class="startup">
        <div class="startup-panel">
          <section class="brand-block">
            <img class="brand-logo" src="${h(appAsset("logo"))}" alt="TCast" />
            <div class="brand-subtitle">TCast ist eine moderne Theater- und Projektionsoftware zur Steuerung von Beamern, Szenen und Medieninhalten. Mehrere Projektoren können live synchron verwaltet und einzelnen Szenen zugewiesen werden. Bilder, Videos und Effekte lassen sich per Drag-and-drop organisieren und steuern.</div>
            <div class="brand-copyright">© Simon Wagner & Felix Eckinger</div>
          </section>
          <section class="startup-box">
            <div class="tabs">
              <button class="tab ${appState.startupTab === "new" ? "active" : ""}" data-tab="new">Neues Projekt</button>
              <button class="tab ${appState.startupTab === "load" ? "active" : ""}" data-tab="load">Projekt laden</button>
            </div>
            ${appState.startupTab === "new" ? renderNewProjectForm(snap) : renderLoadProjectForm(projects)}
          </section>
        </div>
      </main>
    </div>
  `;
}

function renderNewProjectForm(snap) {
  return `
    <form class="form" id="new-project-form">
      <div class="field">
        <label class="label-row" for="project-name"><span>Projekt Name</span></label>
        <input id="project-name" maxlength="60" autocomplete="off" />
      </div>
      <div class="field">
        <label class="label-row" for="project-count"><span>Anzahl Beamer</span><strong id="project-count-label">1</strong></label>
        <input id="project-count" type="range" min="1" max="6" step="1" value="1" />
      </div>
      <div class="field">
        <label class="label-row" for="project-description"><span>Beschreibung (max 180)</span></label>
        <textarea id="project-description" maxlength="180"></textarea>
      </div>
      <div class="field">
        <label class="label-row" for="project-dir"><span>Speicher Pfad</span></label>
        <div class="field-row">
          <input id="project-dir" value="${h(snap.default_saves_path)}" />
          <button type="button" id="browse-new-dir">Ändern</button>
        </div>
      </div>
      <button class="primary" type="submit">Projekt Erstellen</button>
    </form>
  `;
}

function renderLoadProjectForm(projects) {
  return `
    <form class="form" id="load-project-form">
      <div class="project-list">
        ${projects.length ? projects.map(renderProjectRow).join("") : `<div class="empty-state">Keine Projekte im Standardordner.</div>`}
      </div>
      <div class="field">
        <label class="label-row" for="load-path"><span>Pfad</span></label>
        <div class="field-row">
          <input id="load-path" value="${h(appState.selectedProjectPath)}" />
          <button type="button" id="browse-load-dir">Ändern</button>
        </div>
      </div>
      <button class="primary" type="submit">Laden</button>
      <button type="button" id="import-startup">Importieren (.tct)</button>
    </form>
  `;
}

function renderProjectRow(project) {
  return `
    <div class="project-row" data-path="${h(project.path)}">
      <button type="button" class="ghost project-pick">
        <div class="project-title">${h(project.name)}</div>
        <div class="project-meta">${project.projector_count} Beamer · ${h(project.version || "unknown")}</div>
      </button>
      <button type="button" class="project-load">Laden</button>
      <button type="button" class="danger project-delete">Löschen</button>
    </div>
  `;
}

function updateMaximizeButton() {
  const btn = document.getElementById("maximize-window");
  if (!btn) return;

  invoke("get_window_state").then(data => {
    const isMaximized = data?.maximized || false;
    btn.innerHTML = isMaximized
        ? `<svg width="12" height="12" viewBox="0 0 12 12">
           <rect x="1.5" y="3" width="7" height="7" stroke="currentColor" stroke-width="1.5" fill="none"/>
           <rect x="3.5" y="1" width="7" height="7" stroke="currentColor" stroke-width="1.5" fill="var(--bg)"/>
         </svg>`
        : `<svg width="12" height="12" viewBox="0 0 12 12">
           <rect x="1" y="1" width="10" height="10" stroke="currentColor" stroke-width="1.5" fill="none"/>
         </svg>`;
    btn.title = isMaximized ? "Restore" : "Maximize";
  });
}

function renderWorkspace() {
  const snap = appState.snapshot;

  return `
    ${renderTitleBar()}
      ${renderMenu()}

      <main class="workspace">
        ${renderScenes()}

        <div class="panel-resizer-x" id="scene-resizer"></div>

        <section class="main">
          ${renderTopbar()}
          ${renderProjectors()}
        
          <div class="panel-resizer-y" id="resources-resizer"></div>
        
          ${renderResources()}
        </section>
      </main>

      ${appState.modal ? renderModal() : ""}
      ${appState.diagnostics ? renderDiagnostics() : ""}
    </div>
  `;
}

function renderMenu() {
  return `
    <nav class="menu-bar">
      <div class="menu-item">Datei
        <div class="dropdown">
          <button data-menu="new">Projekt wechseln</button>
          <button data-menu="save">Speichern</button>
          <button data-menu="save-as">Speichern unter...</button>
          <button data-menu="export">Exportieren (.tct)</button>
          <button data-menu="close">Projekt schließen</button>
        </div>
      </div>

      <div class="menu-item">Debugging
        <div class="dropdown">
          <button data-diagnostic="overview">Overview</button>
          <button data-diagnostic="project">Project</button>
          <button data-diagnostic="projection">Projection</button>
          <button data-diagnostic="log">Logs</button>
          <button data-diagnostic="resources">Resources</button>
          <button data-diagnostic="scene">Scenes</button>
        </div>
      </div>
    </nav>
  `;
}
function renderScenes() {
  const snap = appState.snapshot;
  const scenes = snap.scenes || [];
  return `
    <aside class="scene-panel">
      <div class="section-head">
        <div class="section-title">Szenen</div>
        <button class="icon-button primary" id="add-scene" title="Szene hinzufügen">
          <svg
            xmlns="http://www.w3.org/2000/svg"
            width="18"
            height="18"
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2.4"
            stroke-linecap="round"
            stroke-linejoin="round"
          >
            <path d="M12 5v14"/>
            <path d="M5 12h14"/>
          </svg>
        </button>
      </div>
      <div class="scene-list">
        ${scenes.map((scene, index) => `
          <div class="scene-item ${index === snap.active_scene_index ? "active" : ""}">
            <button class="ghost scene-pick" data-index="${index}">
              <span class="scene-name">${h(scene.sceneName || scene.name)}</span>
            </button>
            <div class="scene-actions">
              <button class="icon-button scene-rename" data-index="${index}" title="Umbenennen">
                <svg
                  xmlns="http://www.w3.org/2000/svg"
                  width="16"
                  height="16"
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="2.2"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                >
                  <path d="M12 20h9"/>
                  <path d="M16.5 3.5a2.1 2.1 0 0 1 3 3L7 19l-4 1 1-4Z"/>
                </svg>
              </button>
              
              <button class="icon-button scene-copy" data-index="${index}" title="Klonen">
                <svg
                  xmlns="http://www.w3.org/2000/svg"
                  width="16"
                  height="16"
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="2.2"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                >
                  <rect x="9" y="9" width="13" height="13" rx="2"/>
                  <rect x="2" y="2" width="13" height="13" rx="2"/>
                </svg>
              </button>
              
              <button class="icon-button danger scene-delete" data-index="${index}" title="Löschen">
                <svg
                  xmlns="http://www.w3.org/2000/svg"
                  width="16"
                  height="16"
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="2.2"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                >
                  <path d="M3 6h18"/>
                  <path d="M8 6V4h8v2"/>
                  <path d="M19 6l-1 14H6L5 6"/>
                  <path d="M10 11v6"/>
                  <path d="M14 11v6"/>
                </svg>
              </button>
            </div>
          </div>
        `).join("")}
      </div>
      <div class="scene-footer">
        <div class="scene-actions-center">
          <button id="scene-up" title="Nach oben">
            <svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2">
              <path d="M18 15l-6-6-6 6"/>
            </svg>
          </button>
      
          <button id="scene-down" title="Nach unten">
            <svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2">
              <path d="M6 9l6 6 6-6"/>
            </svg>
          </button>
        </div>
      </div>
    </aside>
  `;
}

function renderTopbar() {
  const snap = appState.snapshot;
  const save = snap.save_data;
  return `
    <header class="topbar">
      <div class="project-heading">
        <h1>${h(save?.projectName || save?.name || "TCast")}</h1>
        <p>${h(save?.description || snap.current_project_path)}</p>
      </div>
      <div class="top-actions">
        <button class="primary icon-button" id="start-projection" title="Start">
          <svg
            xmlns="http://www.w3.org/2000/svg"
            width="18"
            height="18"
            viewBox="0 0 24 24"
            fill="currentColor"
          >
            <path d="M8 5v14l11-7z"/>
          </svg>
        </button>
        
        <button class="danger icon-button" id="stop-projection" title="Stop">
          <svg
            xmlns="http://www.w3.org/2000/svg"
            width="18"
            height="18"
            viewBox="0 0 24 24"
            fill="currentColor"
          >
            <rect x="6" y="6" width="12" height="12" rx="1"/>
          </svg>
        </button>
      </div>
    </header>
  `;
}

function computeProjectorViews() {
  const snap = appState.snapshot;
  const save = snap.save_data;
  const count = Math.max(1, Math.min(6, save?.projectorCount || save?.projector_amount || 1));
  const scene = snap.scenes?.[snap.active_scene_index] || {};
  const sources = Array.from({ length: count }, (_, i) => scene.sources?.[i] || "");
  const splitSources = Array.from({ length: count }, (_, i) => scene.splitSources?.[i] || scene.split_sources?.[i] || "");
  const connections = Array.from({ length: count }, (_, i) => Number(scene.connections?.[i] || 0));
  const views = [];
  let i = 0;
  while (i < count) {
    const groupStart = i;
    let groupLength = 1;
    while (groupStart + groupLength - 1 < count - 1 && connections[groupStart + groupLength - 1] === 1) {
      groupLength++;
    }
    const source = sources[groupStart] || "";
    for (let offset = 0; offset < groupLength; offset++) {
      const index = groupStart + offset;
      views[index] = {
        index,
        source,
        preview: splitSources[index] || source,
        connectedLeft: index > 0 && connections[index - 1] === 1,
        connectedRight: index < count - 1 && connections[index] === 1,
        isSplit: groupLength > 1,
        start: groupLength > 1 ? offset / groupLength : 0,
        end: groupLength > 1 ? (offset + 1) / groupLength : 1,
        settings: projectorSettings(index),
      };
    }
    i += groupLength;
  }
  return { count, views, connections };
}

function renderProjectors() {
  const { views, connections } = computeProjectorViews();
  return `
    <section class="stage-area">
      <div class="projector-strip">
        ${views.map((view, index) => `
          ${renderProjector(view)}
          ${index < views.length - 1 ? `<button class="connector ${connections[index] === 1 ? "connected" : ""}" data-index="${index}" title="Projektoren verbinden"></button>` : ""}
        `).join("")}
      </div>
    </section>
  `;
}

function renderProjector(view) {
  const resource = resourceForSource(view.source);
  const srcUrl = sourceToUrl(view.preview || view.source);
  const isVideo = resource?.is_video || /\.(mp4|avi|mov|mkv|flv|webm)$/i.test(view.source);
  const settings = view.settings || projectorSettings(view.index);
  const aspectOptions = PROJECTOR_ASPECTS.map(aspect => `
    <option value="${aspect}" ${settings.aspect === aspect ? "selected" : ""}>${aspect}</option>
  `).join("");
  let mediaHtml = '';
  if (srcUrl) {
    const span = view.isSplit && isVideo ? Math.max(0.0001, view.end - view.start) : 1;
    const width = 100 / span;
    const left = -view.start / span * 100;
    const sizing = `width:${width}%; left:${left}%;`;
    const media = isVideo
        ? `<video src="${h(srcUrl)}" muted loop playsinline style="${sizing}"></video>`
        : `<img src="${h(srcUrl)}" alt="" style="${sizing}" />`;
    mediaHtml = `<div class="projector-rotator" style="${projectorRotatorStyle(settings)}">${media}</div>`;
  } else {
    mediaHtml = `<div class="projector-empty">Kein Bild</div>`;
  }
  return `
    <div class="projector" data-projector="${view.index}" style="aspect-ratio:${effectiveAspectCss(settings)}">
      <button type="button" class="projector-media projector-select" data-projector="${view.index}" title="Ressource auswählen">
        ${mediaHtml}
      </button>
      <div class="projector-controls">
        <select class="projector-aspect" data-projector="${view.index}" title="Format">${aspectOptions}</select>
        <button type="button" class="projector-rotate" data-projector="${view.index}" title="Drehen">
          <svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M21 12a9 9 0 1 1-2.64-6.36"/>
            <path d="M21 3v6h-6"/>
          </svg>
          <span>${settings.rotation}°</span>
        </button>
      </div>
      <div class="projector-label">Beamer ${view.index + 1} · ${h(settings.aspect)}${settings.rotation ? ` · ${settings.rotation}°` : ""}${view.isSplit ? ` · ${Math.round(view.start * 100)}-${Math.round(view.end * 100)}%` : ""}</div>
      <div class="projector-source">${h(resource?.name || shortPath(view.source) || "Ressource auswählen")}</div>
    </div>
  `;
}

function renderResources() {
  const resources = appState.snapshot.resources || [];
  return `
    <section class="resource-panel">
      <div class="resource-header">
        <div class="section-title">Ressourcen</div>
        <div class="resource-actions">
          <button class="primary" id="add-resource">Hinzufügen</button>
        </div>
      </div>
      <div class="resource-list">
        ${resources.length ? resources.map(renderResource).join("") : `<div class="empty-state">Noch keine Ressourcen.</div>`}
      </div>
    </section>
  `;
}

function changeProjectorCount(count) {
  run("set_projector_count", { count }, { message: `Projektoranzahl auf ${count} gesetzt` });
}

function renderResource(resource) {
  return `
    <div class="resource-item ${appState.snapshot.active_resource_id === resource.id ? "active" : ""}" draggable="true" data-resource="${resource.id}">
      <div class="resource-thumb">
        <button class="resource-preview resource-select" data-resource="${resource.id}">
          ${resource.is_video
      ? `<video src="${h(resource.url)}" muted loop playsinline></video>`
      : `<img src="${h(resource.preview_url)}" alt="" />`}
        </button>
        <div class="resource-buttons">
          <button class="icon-button resource-rename" data-resource="${resource.id}" title="Umbenennen">
            <svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
              <path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7"/>
              <path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z"/>
            </svg>
          </button>
          <button class="icon-button danger resource-delete" data-resource="${resource.id}" title="Löschen">
            <svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
              <polyline points="3 6 5 6 21 6"/>
              <path d="M19 6l-1 14a2 2 0 0 1-2 2H8a2 2 0 0 1-2-2L5 6"/>
              <path d="M10 11v6M14 11v6"/>
              <path d="M9 6V4a1 1 0 0 1 1-1h4a1 1 0 0 1 1 1v2"/>
            </svg>
          </button>
        </div>
      </div>
      <div class="resource-name" title="${h(resource.name)}">${h(resource.name)}</div>
    </div>
  `;
}

function renderModal() {
  if (appState.modal?.type === "add-resource") return renderAddResourceModal();
  if (appState.modal?.type === "picker") return renderResourcePicker();
  return "";
}

function renderAddResourceModal() {
  return `
    <div class="modal-overlay">
      <form class="modal-box" id="resource-form">
        <h2 class="modal-title">Ressourcen hinzufügen</h2>
        <div class="field">
          <label class="label-row" for="resource-paths"><span>Dateien</span></label>
          <div class="field-row">
            <textarea id="resource-paths">${h((appState.modal.paths || []).join("\n"))}</textarea>
            <button type="button" id="browse-resource-files">Wählen</button>
          </div>
        </div>
        <div class="field">
          <label class="label-row" for="resource-names"><span>Namen</span></label>
          <textarea id="resource-names">${h((appState.modal.names || []).join("\n"))}</textarea>
        </div>
        <div class="modal-actions">
          <button type="button" class="modal-close">Abbrechen</button>
          <button class="primary" type="submit">Hinzufügen</button>
        </div>
      </form>
    </div>
  `;
}

function renderResourcePicker() {
  const resources = appState.snapshot.resources || [];
  return `
    <div class="modal-overlay">
      <div class="modal-box">
        <h2 class="modal-title">Ressource für Beamer ${appState.modal.projectorIndex + 1}</h2>
        <div class="picker-list">
          <button class="picker-row picker-none">
            <div class="resource-thumb"></div>
            <div>Kein Bild</div>
          </button>
          ${resources.map(resource => `
            <button class="picker-row picker-resource" data-resource="${resource.id}">
              <div class="resource-thumb">
                <div class="resource-preview">
                  ${resource.is_video ? `<video src="${h(resource.url)}" muted playsinline></video>` : `<img src="${h(resource.preview_url)}" alt="" />`}
                </div>
              </div>
              <div>
                <div class="resource-name">${h(resource.name)}</div>
                <div class="resource-type">${resource.is_video ? "Video" : "Bild"}</div>
              </div>
            </button>
          `).join("")}
        </div>
        <div class="modal-actions">
          <button class="modal-close">Schließen</button>
        </div>
      </div>
    </div>
  `;
}

function renderDiagnostics() {
  const snap = appState.snapshot;
  if (!snap) return '';

  const save = snap.save_data || {};
  const scenes = snap.scenes || [];
  const resources = snap.resources || [];
  const count = Math.max(1, Math.min(6, save.projectorCount || save.projector_amount || 1));
  const activeScene = scenes[snap.active_scene_index] || {};
  const activeSources = Array.from({ length: count }, (_, i) => activeScene.sources?.[i] || "");
  const settings = Array.from({ length: count }, (_, i) => projectorSettings(i));
  const tabs = [
    ["overview", "Overview"],
    ["project", "Project"],
    ["projection", "Projection"],
    ["log", "Logs"],
    ["resources", "Resources"],
    ["scene", "Scenes"],
  ];

  let body = '';
  switch (appState.diagnostics) {
    case 'overview':
      body = `
        <div class="diag-grid">
          <div class="stat-grid">
            <div class="stat-card"><span>Projectors</span><strong>${count}</strong></div>
            <div class="stat-card"><span>Scenes</span><strong>${scenes.length}</strong></div>
            <div class="stat-card"><span>Resources</span><strong>${resources.length}</strong></div>
            <div class="stat-card"><span>Assigned</span><strong>${activeSources.filter(Boolean).length}/${count}</strong></div>
          </div>
          <div class="diag-section">
            <h3>Current State</h3>
            <div class="diag-kv">
              <span>Project</span><strong>${h(save.projectName || save.name || "Untitled")}</strong>
              <span>Active scene</span><strong>${h(activeScene.sceneName || activeScene.name || "None")}</strong>
              <span>Projection</span><strong class="${snap.projection_active ? "status-on" : "status-off"}">${snap.projection_active ? "Running" : "Stopped"}</strong>
              <span>Path</span><strong title="${h(snap.current_project_path)}">${h(shortPath(snap.current_project_path))}</strong>
            </div>
          </div>
          <div class="diag-section">
            <h3>Projector Formats</h3>
            <div class="projector-debug-list">
              ${settings.map((item, index) => `
                <div class="projector-debug">
                  <strong>Beamer ${index + 1}</strong>
                  <span>${h(item.aspect)} · ${item.rotation}° · ${effectiveAspectValue(item).toFixed(3)}:1</span>
                  <small>${h(shortPath(activeSources[index]) || "No source")}</small>
                </div>
              `).join("")}
            </div>
          </div>
        </div>`;
      break;

    case 'project':
      body = `
        <div class="diag-section">
          <h3>Project Data</h3>
          <div class="diag-kv">
            <span>Name</span><strong>${h(save.projectName || save.name)}</strong>
            <span>Description</span><strong>${h(save.description || "")}</strong>
            <span>Version</span><strong>${h(save.version || "unknown")}</strong>
            <span>Folder</span><strong title="${h(snap.current_project_path)}">${h(shortPath(snap.current_project_path))}</strong>
          </div>
          <pre class="diag-code">${h(JSON.stringify({
        save_data: snap.save_data,
        current_project_path: snap.current_project_path,
        version: snap.save_data?.version
      }, null, 2))}</pre>
        </div>`;
      break;

    case 'projection':
      body = `
        <div class="diag-section">
          <h3>Projection</h3>
          ${snap.projection_specs && snap.projection_specs.length
          ? snap.projection_specs.map(spec => `
              <div class="proj-spec">
                <strong>${h(spec.label)}</strong>
                <div class="diag-kv compact">
                  <span>Resource</span><strong>${h(spec.resource_name)}</strong>
                  <span>Format</span><strong>${h(spec.aspect || "16:9")} · ${Number(spec.rotation || 0)}°</strong>
                  <span>Media</span><strong>${spec.is_video ? "Video" : "Image"}</strong>
                  <span>Split</span><strong>${spec.is_split ? `${Math.round(spec.start*100)}-${Math.round(spec.end*100)}%` : "No"}</strong>
                  <span>Source</span><strong title="${h(spec.source_path)}">${h(shortPath(spec.source_path))}</strong>
                </div>
              </div>`).join('')
          : '<div class="empty-state tight">No active projection. Start projection to inspect generated specs.</div>'}
        </div>`;
      break;

    case 'log':
      body = `
        <div class="diag-section">
          <h3>Recent Log</h3>
          <div class="logs">
            ${(snap.logs || []).slice().reverse().map(line => `
              <div class="log-line ${h(line.level)}">
                <span>${h(line.level)}</span>
                <strong>${h(line.message)}</strong>
              </div>`).join('')}
          </div>
        </div>`;
      break;

    case 'resources':
      body = `
        <div class="diag-section">
          <h3>Resources</h3>
          <table class="diag-table">
            <tr><th>ID</th><th>Name</th><th>Type</th><th>Thumb</th><th>Status</th></tr>
            ${resources.map(r => `
              <tr>
                <td>${r.id}</td>
                <td>${h(r.name)}</td>
                <td>${r.is_video ? 'video' : 'image'}</td>
                <td>${r.thumbnail_id >= 0 ? r.thumbnail_id : "source"}</td>
                <td>${r.missing ? 'Missing' : 'OK'}</td>
              </tr>`).join('')}
          </table>
          <pre class="diag-code">${h(JSON.stringify(resources.map(r => ({
            id: r.id,
            name: r.name,
            path: r.path,
            preview: r.preview_url,
            video: r.is_video,
            missing: r.missing,
          })), null, 2))}</pre>
        </div>`;
      break;

    case 'scene':
      body = `
        <div class="diag-section">
          <h3>Scenes</h3>
          ${scenes.map((scene, idx) => `
            <div class="scene-detail">
              <strong>${idx + 1}. ${h(scene.sceneName || scene.name)}</strong>
              <div class="diag-kv compact">
                <span>Sources</span><strong>${(scene.sources || []).filter(Boolean).length}/${count}</strong>
                <span>Connections</span><strong>${h(JSON.stringify(scene.connections || []))}</strong>
              </div>
              <pre class="diag-code">${h(JSON.stringify({
                sources: scene.sources,
                split_sources: scene.splitSources || scene.split_sources,
                connections: scene.connections,
              }, null, 2))}</pre>
            </div>`).join('')}
        </div>`;
      break;

    default:
      body = `<p>Unknown diagnostic.</p>`;
  }

  return `
    <aside class="drawer">
      <div class="resource-header">
        <div class="section-title">Debug</div>
        <button id="diagnostics-close">Schließen</button>
      </div>
      <div class="diag-tabs">
        ${tabs.map(([id, label]) => `
          <button data-diagnostic="${id}" class="${appState.diagnostics === id ? "active" : ""}">${label}</button>
        `).join("")}
      </div>
      <div class="drawer-body">
        ${body}
      </div>
    </aside>
  `;
}

function bindStartup() {
  document.querySelectorAll("[data-tab]").forEach(button => {
    button.addEventListener("click", () => {
      appState.startupTab = button.dataset.tab;
      render();
    });
  });

  const count = document.getElementById("project-count");
  if (count) {
    count.addEventListener("input", () => {
      document.getElementById("project-count-label").textContent = count.value;
    });
  }

  document.getElementById("browse-new-dir")?.addEventListener("click", async () => {
    const data = await run("browse_folder");
    if (data?.path) document.getElementById("project-dir").value = data.path;
  });

  document.getElementById("new-project-form")?.addEventListener("submit", event => {
    event.preventDefault();
    run("create_project", {
      name: document.getElementById("project-name").value.trim(),
      projectorCount: Number(document.getElementById("project-count").value),
      description: document.getElementById("project-description").value.trim(),
      path: document.getElementById("project-dir").value.trim(),
    }, { message: "Projekt erstellt" });
  });

  document.querySelectorAll(".project-row").forEach(row => {
    const path = row.dataset.path;
    row.querySelector(".project-pick")?.addEventListener("click", () => {
      appState.selectedProjectPath = path;
      document.getElementById("load-path").value = path;
    });
    row.querySelector(".project-load")?.addEventListener("click", () => {
      run("load_project", { path }, { message: "Projekt geladen" });
    });
    row.querySelector(".project-delete")?.addEventListener("click", () => {
      if (confirm("Projekt löschen?")) run("delete_project", { path });
    });
  });

  document.getElementById("browse-load-dir")?.addEventListener("click", async () => {
    const data = await run("browse_project_folder");
    if (data?.path) {
      appState.selectedProjectPath = data.path;
      document.getElementById("load-path").value = data.path;
    }
  });

  document.getElementById("load-project-form")?.addEventListener("submit", event => {
    event.preventDefault();
    const path = document.getElementById("load-path").value.trim();
    run("load_project", { path }, { message: "Projekt geladen" });
  });

  document.getElementById("import-startup")?.addEventListener("click", () => {
    run("import_project", {}, { message: "Projekt importiert" });
  });
}

function setupPanelResize() {
  const sceneResizer = document.getElementById("scene-resizer");
  const resourcesResizer = document.getElementById("resources-resizer");

  sceneResizer?.addEventListener("mousedown", event => {
    event.preventDefault();

    function move(e) {
      const width = Math.max(180, Math.min(500, e.clientX));
      document.documentElement.style.setProperty("--scene-width", width + "px");
    }

    function up() {
      window.removeEventListener("mousemove", move);
      window.removeEventListener("mouseup", up);
    }

    window.addEventListener("mousemove", move);
    window.addEventListener("mouseup", up);
  });

  resourcesResizer?.addEventListener("mousedown", event => {
    event.preventDefault();

    function move(e) {
      const height = Math.max(120, Math.min(500, window.innerHeight - e.clientY));
      document.documentElement.style.setProperty("--resources-height", height + "px");
    }

    function up() {
      window.removeEventListener("mousemove", move);
      window.removeEventListener("mouseup", up);
    }

    window.addEventListener("mousemove", move);
    window.addEventListener("mouseup", up);
  });
}

function setupTitleBarDrag() {
  // Remove ALL custom drag handling
  // The -webkit-app-region: drag in CSS will let Windows handle it natively

  // Keep double-click to maximize
  const titleBar = document.querySelector(".title-bar");

  if (titleBar) {
    titleBar.addEventListener("dblclick", (e) => {
      if (
          e.target.closest(".title-bar-center") ||
          (!e.target.closest("button") &&
              !e.target.closest(".menu-item") &&
              !e.target.closest(".dropdown"))
      ) {
        invoke("maximize_window");
      }
    });
  }
}

function renderTitleBar() {
  const hasProject = appState.snapshot?.current_project_path;

  return `
    <div class="title-bar">
      <div class="title-bar-left">
        ${hasProject ? `
          <div class="menu-item">Datei
            <div class="dropdown">
              <button data-menu="new">Projekt wechseln</button>
              <button data-menu="save">Speichern</button>
              <button data-menu="save-as">Speichern unter...</button>
              <button data-menu="export">Exportieren (.tct)</button>
              <button data-menu="close">Projekt schließen</button>
            </div>
          </div>
          <div class="menu-item">Debugging
            <div class="dropdown">
              <button data-diagnostic="overview">Overview</button>
              <button data-diagnostic="project">Project</button>
              <button data-diagnostic="projection">Projection</button>
              <button data-diagnostic="log">Logs</button>
              <button data-diagnostic="resources">Resources</button>
              <button data-diagnostic="scene">Scenes</button>
            </div>
          </div>
        ` : ''}
      </div>
      
      <div class="title-bar-center" title="Drag to move window">
        <img class="title-icon" src="${h(appAsset("favicon"))}" alt="" />
        <span class="title-text">TCast</span>
      </div>
      
      <div class="title-bar-right">
        <button class="window-control" id="minimize-window" title="Minimize">
          <svg width="12" height="12" viewBox="0 0 12 12">
            <rect x="0" y="5" width="12" height="1.5" fill="currentColor"/>
          </svg>
        </button>
        <button class="window-control" id="maximize-window" title="Maximize">
          <svg width="12" height="12" viewBox="0 0 12 12">
            <rect x="1" y="1" width="10" height="10" stroke="currentColor" stroke-width="1.5" fill="none"/>
          </svg>
        </button>
        <button class="window-control close" id="close-window" title="Close">
          <svg width="12" height="12" viewBox="0 0 12 12">
            <path d="M1 1L11 11M11 1L1 11" stroke="currentColor" stroke-width="1.5"/>
          </svg>
        </button>
      </div>
    </div>
  `;
}
function bindWorkspace() {

  document.querySelectorAll("[data-menu]").forEach(button => {
    button.addEventListener("click", () => handleMenu(button.dataset.menu));
  });

  document.getElementById("diagnostics-close")?.addEventListener("click", () => {
    appState.diagnostics = false;
    render();
  });

  document.querySelectorAll("[data-diagnostic]").forEach(button => {
    button.addEventListener("click", () => {
      appState.diagnostics = button.dataset.diagnostic;
      render();
    });
  });

  setupPanelResize();

  document.getElementById("add-scene")?.addEventListener("click", () => run("add_scene"));
  document.getElementById("scene-up")?.addEventListener("click", () => run("move_scene", { direction: -1 }));
  document.getElementById("scene-down")?.addEventListener("click", () => run("move_scene", { direction: 1 }));
  document.getElementById("scene-clear")?.addEventListener("click", () => {
    if (confirm("Alle Szenen löschen?")) run("clear_scenes");
  });
  document.getElementById("refresh-snapshot")?.addEventListener("click", () => run("snapshot"));

  document.querySelectorAll(".scene-pick").forEach(button => {
    button.addEventListener("click", () => run("select_scene", { index: Number(button.dataset.index) }));
  });
  document.querySelectorAll(".scene-rename").forEach(button => {
    button.addEventListener("click", () => {
      const index = Number(button.dataset.index);
      const scene = appState.snapshot.scenes[index];
      const name = prompt("Szenenname", scene.sceneName || scene.name || "");
      if (name) run("rename_scene", { index, name });
    });
  });
  document.querySelectorAll(".scene-copy").forEach(button => {
    button.addEventListener("click", () => run("duplicate_scene", { index: Number(button.dataset.index) }));
  });
  document.querySelectorAll(".scene-delete").forEach(button => {
    button.addEventListener("click", () => {
      if (confirm("Szene löschen?")) run("delete_scene", { index: Number(button.dataset.index) });
    });
  });

  document.getElementById("start-projection")?.addEventListener("click", () => run("start_projection", {}, { message: "Projektion gestartet" }));
  document.getElementById("stop-projection")?.addEventListener("click", () => run("stop_projection", {}, { message: "Projektion gestoppt" }));
  document.getElementById("save-now")?.addEventListener("click", () => run("save_project", {}, { message: "Gespeichert" }));

  document.querySelectorAll(".connector").forEach(button => {
    button.addEventListener("click", () => {
      const index = Number(button.dataset.index);
      run("set_connection", { index, connected: !button.classList.contains("connected") });
    });
  });

  document.querySelectorAll(".projector").forEach(button => {
    const projectorIndex = Number(button.dataset.projector);
    button.addEventListener("dragover", event => {
      event.preventDefault();
      button.classList.add("drag-over");
    });
    button.addEventListener("dragleave", () => button.classList.remove("drag-over"));
    button.addEventListener("drop", event => {
      event.preventDefault();
      button.classList.remove("drag-over");
      const resourceId = Number(event.dataTransfer.getData("text/resource-id"));
      if (!Number.isNaN(resourceId)) {
        run("assign_resource", { projectorIndex, resourceId });
      }
    });
  });

  document.querySelectorAll(".projector-select").forEach(button => {
    button.addEventListener("click", () => {
      appState.modal = { type: "picker", projectorIndex: Number(button.dataset.projector) };
      render();
    });
  });

  document.querySelectorAll(".projector-aspect").forEach(select => {
    select.addEventListener("change", () => {
      const projectorIndex = Number(select.dataset.projector);
      const current = projectorSettings(projectorIndex);
      run("set_projector_format", {
        projectorIndex,
        aspect: select.value,
        rotation: current.rotation,
      }, { message: `Beamer ${projectorIndex + 1}: ${select.value}` });
    });
  });

  document.querySelectorAll(".projector-rotate").forEach(button => {
    button.addEventListener("click", () => {
      const projectorIndex = Number(button.dataset.projector);
      const current = projectorSettings(projectorIndex);
      const nextRotation = (current.rotation + 90) % 360;
      run("set_projector_format", {
        projectorIndex,
        aspect: current.aspect,
        rotation: nextRotation,
      }, { message: `Beamer ${projectorIndex + 1}: ${nextRotation}°` });
    });
  });

  document.getElementById("add-resource")?.addEventListener("click", () => {
    appState.modal = { type: "add-resource", paths: [], names: [] };
    render();
  });

  bindResourceItems();
  bindModal();
  document.querySelectorAll("video").forEach(video => video.play?.().catch(() => {}));
}

function handleMenu(action) {
  if (action === "new") {
    appState.startupTab = "new";
    run("close_project");
  } else if (action === "save") {
    run("save_project", {}, { message: "Gespeichert" });
  } else if (action === "save-as") {
    run("save_as", {}, { message: "Kopie gespeichert" });
  } else if (action === "export") {
    run("export_project", {}, { message: "Projekt exportiert" });
  } else if (action === "close") {
    run("stop_projection").finally(() => run("close_project"));
  }
}

function bindResourceItems() {
  document.querySelectorAll(".resource-item").forEach(item => {
    item.addEventListener("dragstart", event => {
      event.dataTransfer.setData("text/resource-id", item.dataset.resource);
    });
  });
  document.querySelectorAll(".resource-select").forEach(button => {
    button.addEventListener("click", () => run("select_resource", { id: Number(button.dataset.resource) }));
  });
  document.querySelectorAll(".resource-rename").forEach(button => {
    button.addEventListener("click", () => {
      const id = Number(button.dataset.resource);
      const resource = appState.snapshot.resources.find(item => item.id === id);
      const name = prompt("Ressourcenname", resource?.name || "");
      if (name) run("rename_resource", { id, name });
    });
  });
  document.querySelectorAll(".resource-delete").forEach(button => {
    button.addEventListener("click", () => {
      const id = Number(button.dataset.resource);
      if (confirm("Ressource löschen?")) run("delete_resource", { id });
    });
  });
}

function bindModal() {
  document.querySelectorAll(".modal-close").forEach(button => {
    button.addEventListener("click", () => {
      appState.modal = null;
      render();
    });
  });

  document.getElementById("browse-resource-files")?.addEventListener("click", async () => {
    const data = await run("browse_media_files");
    const paths = data?.paths || [];
    appState.modal.paths = paths;
    appState.modal.names = paths.map(path => {
      const file = path.replaceAll("\\", "/").split("/").pop() || "Resource";
      return file.replace(/\.[^.]+$/, "");
    });
    render();
  });

  document.getElementById("resource-form")?.addEventListener("submit", event => {
    event.preventDefault();
    const paths = document.getElementById("resource-paths").value.split(/\r?\n/).map(v => v.trim()).filter(Boolean);
    const names = document.getElementById("resource-names").value.split(/\r?\n/).map(v => v.trim()).filter(Boolean);
    run("add_resources", { paths, names }, { message: "Ressourcen hinzugefügt" }).then(() => {
      appState.modal = null;
      render();
    });
  });

  document.querySelector(".picker-none")?.addEventListener("click", () => {
    run("assign_resource", { projectorIndex: appState.modal.projectorIndex, resourceId: null }).then(() => {
      appState.modal = null;
      render();
    });
  });
  document.querySelectorAll(".picker-resource").forEach(button => {
    button.addEventListener("click", () => {
      run("assign_resource", {
        projectorIndex: appState.modal.projectorIndex,
        resourceId: Number(button.dataset.resource),
      }).then(() => {
        appState.modal = null;
        render();
      });
    });
  });
}

function resourceForSource(source) {
  if (!source) return null;
  return (appState.snapshot.resources || []).find(resource => samePath(resource.path, source));
}

function sourceToUrl(source) {
  if (!source) return "";
  const resource = resourceForSource(source);
  return resource?.url || pathToFileUrl(source);
}

function samePath(a, b) {
  return String(a || "").replaceAll("\\", "/").toLowerCase() === String(b || "").replaceAll("\\", "/").toLowerCase();
}

document.addEventListener("DOMContentLoaded", async () => {
  try {
    await run("boot");
  } catch (error) {
    document.getElementById("app").innerHTML = `<main class="startup"><div class="startup-box form">TCast konnte nicht starten: ${h(error.message)}</div></main>`;
  }
});
