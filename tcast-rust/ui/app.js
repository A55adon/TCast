const appState = {
  snapshot: null,
  startupTab: "new",
  selectedProjectPath: "",
  modal: null,
  diagnostics: false,
  pending: new Map(),
  nextId: 1,
};

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
  if (/^(file|https?):\/\//i.test(path)) return path;
  let normalized = path.replaceAll("\\", "/");
  if (/^[A-Za-z]:/.test(normalized)) normalized = "/" + normalized;
  return encodeURI("file://" + normalized).replaceAll("#", "%23");
}

function shortPath(path) {
  if (!path) return "";
  const normalized = path.replaceAll("\\", "/");
  const parts = normalized.split("/");
  return parts.slice(-3).join("/");
}

function showToast(message, type = "info") {
  const wrap = ensureToastWrap();
  const toast = document.createElement("button");
  toast.className = `toast ${type}`;
  toast.textContent = message;
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

function render() {
  const root = document.getElementById("app");
  const snap = appState.snapshot;
  if (!snap?.current_project_path) {
    root.innerHTML = renderStartup();
    bindStartup();
  } else {
    root.innerHTML = renderWorkspace();
    bindWorkspace();
  }
}

function renderStartup() {
  const snap = appState.snapshot || { projects: [], default_saves_path: "" };
  const projects = snap.projects || [];
  return `
    <main class="startup">
      <div class="startup-panel">
        <section class="brand-block">
          <div class="brand-name">TCast</div>
          <div class="brand-subtitle">Theatre scene control for one to six projector outputs, rewritten as a Rust app with a regular HTML and CSS interface.</div>
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
        <label class="label-row" for="project-description"><span>Beschreibung</span></label>
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

function renderWorkspace() {
  const snap = appState.snapshot;
  return `
    <div class="app-shell">
      ${renderMenu()}
      <main class="workspace">
        ${renderScenes()}
        <section class="main">
          ${renderTopbar()}
          ${renderProjectors()}
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
          <button data-menu="new">Neues Projekt</button>
          <button data-menu="load">Projekt öffnen</button>
          <button data-menu="save">Speichern</button>
          <button data-menu="save-as">Speichern unter...</button>
          <button data-menu="import">Importieren (.tct)</button>
          <button data-menu="export">Exportieren (.tct)</button>
          <button data-menu="close">Projekt Schließen</button>
        </div>
      </div>
      <div class="menu-item"><button class="ghost" id="diagnostics-toggle">Diagnose</button></div>
      <div class="menu-item"><button class="ghost" id="open-folder">Ordner</button></div>
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
        <button class="icon-button primary" id="add-scene" title="Szene hinzufügen">+</button>
      </div>
      <div class="scene-list">
        ${scenes.map((scene, index) => `
          <div class="scene-item ${index === snap.active_scene_index ? "active" : ""}">
            <button class="ghost scene-pick" data-index="${index}">
              <span class="scene-name">${h(scene.sceneName || scene.name)}</span>
            </button>
            <div class="scene-actions">
              <button class="icon-button scene-rename" data-index="${index}" title="Umbenennen">R</button>
              <button class="icon-button scene-copy" data-index="${index}" title="Klonen">C</button>
              <button class="icon-button danger scene-delete" data-index="${index}" title="Löschen">X</button>
            </div>
          </div>
        `).join("")}
      </div>
      <div class="scene-footer">
        <button id="scene-up">Hoch</button>
        <button id="scene-down">Runter</button>
        <button class="danger" id="scene-clear">Alle löschen</button>
        <button id="refresh-snapshot">Aktualisieren</button>
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
        <button class="primary" id="start-projection">Start</button>
        <button class="danger" id="stop-projection">Stop</button>
        <button id="save-now">Speichern</button>
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
  const previewUrl = sourceToUrl(view.preview || view.source);
  const isVideo = resource?.is_video || /\.(mp4|avi|mov|mkv|flv|webm)$/i.test(view.source);
  const span = Math.max(0.0001, view.end - view.start);
  const width = 100 / span;
  const left = -view.start / span * 100;
  return `
    <button class="projector" data-projector="${view.index}">
      <div class="projector-media">
        ${previewUrl ? (
          isVideo
            ? `<video src="${h(previewUrl)}" style="width:${width}%;left:${left}%;" muted loop playsinline></video>`
            : `<img src="${h(previewUrl)}" style="width:${width}%;left:${left}%;" alt="" />`
        ) : `<div class="projector-empty">Kein Bild</div>`}
      </div>
      <div class="projector-label">Beamer ${view.index + 1}${view.isSplit ? ` · ${Math.round(view.start * 100)}-${Math.round(view.end * 100)}%` : ""}</div>
      <div class="projector-source">${h(resource?.name || shortPath(view.source) || "Ressource auswählen")}</div>
    </button>
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

function renderResource(resource) {
  return `
    <div class="resource-item ${appState.snapshot.active_resource_id === resource.id ? "active" : ""}" draggable="true" data-resource="${resource.id}">
      <button class="resource-preview resource-select" data-resource="${resource.id}">
        ${resource.is_video
          ? `<video src="${h(resource.url)}" muted loop playsinline></video>`
          : `<img src="${h(resource.preview_url)}" alt="" />`}
      </button>
      <div class="resource-meta">
        <div class="resource-name" title="${h(resource.name)}">${h(resource.name)}</div>
        <div class="resource-type">${resource.is_video ? "Video" : "Bild"}${resource.missing ? " · fehlt" : ""}</div>
      </div>
      <div class="resource-buttons">
        <button class="resource-rename" data-resource="${resource.id}">Umbenennen</button>
        <button class="danger resource-delete" data-resource="${resource.id}">Löschen</button>
      </div>
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
            <div class="resource-preview">None</div>
            <div>Kein Bild</div>
          </button>
          ${resources.map(resource => `
            <button class="picker-row picker-resource" data-resource="${resource.id}">
              <div class="resource-preview">
                ${resource.is_video ? `<video src="${h(resource.url)}" muted playsinline></video>` : `<img src="${h(resource.preview_url)}" alt="" />`}
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
  const logs = snap.logs || [];
  return `
    <aside class="drawer">
      <div class="resource-header">
        <div class="section-title">Diagnose</div>
        <button id="diagnostics-close">Schließen</button>
      </div>
      <div class="drawer-body">
        <div class="diag-grid">
          <div class="diag-pre">${h(JSON.stringify({
            project: snap.save_data,
            current_project_path: snap.current_project_path,
            active_scene_index: snap.active_scene_index,
            active_resource_id: snap.active_resource_id,
            projection_active: snap.projection_active,
            scenes: snap.scenes?.length || 0,
            resources: snap.resources?.length || 0,
          }, null, 2))}</div>
          <div class="logs">
            ${logs.slice().reverse().map(line => `<div class="log-line ${h(line.level)}">${h(line.message)}</div>`).join("")}
          </div>
        </div>
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

function bindWorkspace() {
  document.querySelectorAll("[data-menu]").forEach(button => {
    button.addEventListener("click", () => handleMenu(button.dataset.menu));
  });

  document.getElementById("diagnostics-toggle")?.addEventListener("click", () => {
    appState.diagnostics = !appState.diagnostics;
    render();
  });
  document.getElementById("diagnostics-close")?.addEventListener("click", () => {
    appState.diagnostics = false;
    render();
  });
  document.getElementById("open-folder")?.addEventListener("click", () => run("open_project_folder"));

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
    button.addEventListener("click", () => {
      appState.modal = { type: "picker", projectorIndex };
      render();
    });
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
  } else if (action === "load") {
    appState.startupTab = "load";
    run("close_project");
  } else if (action === "save") {
    run("save_project", {}, { message: "Gespeichert" });
  } else if (action === "save-as") {
    run("save_as", {}, { message: "Kopie gespeichert" });
  } else if (action === "import") {
    run("import_project", {}, { message: "Projekt importiert" });
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
