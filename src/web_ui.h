#pragma once

namespace GUPet {

static const char* INDEX_HTML = R"rawliteral(<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>GUPet — Настройки</title>
<style>
  :root {
    --m3-primary: #D97757;
    --m3-on-primary: #ffffff;
    --m3-primary-container: #FFE5DC;
    --m3-on-primary-container: #3E160A;
    --m3-secondary-container: #F4DCD2;
    --m3-on-secondary-container: #32160E;
    --m3-surface: #FDF8F6;
    --m3-surface-container-low: #F7F1EE;
    --m3-surface-container: #F1EBE7;
    --m3-surface-container-high: #EBE5E1;
    --m3-on-surface: #1E1B1A;
    --m3-on-surface-variant: #524440;
    --m3-outline: #85736E;
    --m3-outline-variant: #D7C2BC;
    --m3-radius: 24px;
    --m3-radius-sm: 16px;
    --m3-font: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
  }

  * { box-sizing: border-box; margin: 0; padding: 0; }
  body {
    font-family: var(--m3-font);
    background: var(--m3-surface);
    color: var(--m3-on-surface);
    line-height: 1.5;
    padding: 16px 12px 64px 12px;
    display: flex;
    justify-content: center;
  }

  .container {
    width: 100%;
    max-width: 520px;
    display: flex;
    flex-direction: column;
    gap: 14px;
  }

  /* Top Bar */
  .top-bar {
    text-align: center;
    padding: 8px 0 4px 0;
  }
  .top-bar h1 {
    font-size: 24px;
    font-weight: 700;
    letter-spacing: -0.3px;
    color: var(--m3-on-surface);
  }
  .top-bar p {
    font-size: 13px;
    color: var(--m3-on-surface-variant);
    margin-top: 1px;
  }

  /* Card */
  .card {
    background: var(--m3-surface-container-low);
    border-radius: var(--m3-radius);
    padding: 18px 20px;
    display: flex;
    flex-direction: column;
    gap: 12px;
    box-shadow: 0 1px 3px rgba(0,0,0,0.03);
    border: 1px solid rgba(0,0,0,0.04);
  }

  /* Hero Card */
  .hero-card {
    background: linear-gradient(145deg, #FFEFE8 0%, #F5E5DC 100%);
    border-radius: 28px;
    padding: 22px 20px;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 14px;
    position: relative;
    border: 1px solid rgba(217, 119, 87, 0.18);
  }

  .hero-canvas-wrap {
    width: 140px;
    height: 100px;
    display: flex;
    align-items: center;
    justify-content: center;
  }

  #hero-canvas {
    image-rendering: pixelated;
    image-rendering: crisp-edges;
  }

  .hero-row {
    width: 100%;
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding-top: 8px;
    border-top: 1px solid rgba(0,0,0,0.06);
  }

  .hero-info h2 {
    font-size: 16px;
    font-weight: 600;
  }
  .hero-info p {
    font-size: 12px;
    color: var(--m3-on-surface-variant);
  }

  /* Section Title */
  .section-title {
    display: flex;
    align-items: center;
    gap: 8px;
    font-size: 14px;
    font-weight: 600;
    color: var(--m3-primary);
    margin-top: 4px;
    margin-bottom: -4px;
    padding: 0 6px;
  }

  /* Switch */
  .switch-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    padding: 8px 0;
    border-bottom: 1px solid rgba(0,0,0,0.04);
  }
  .switch-row:last-child { border-bottom: none; }
  .switch-label {
    display: flex;
    flex-direction: column;
  }
  .switch-title {
    font-size: 14px;
    font-weight: 500;
  }
  .switch-desc {
    font-size: 12px;
    color: var(--m3-on-surface-variant);
  }

  .m3-switch {
    position: relative;
    display: inline-block;
    width: 52px;
    height: 32px;
    flex-shrink: 0;
  }
  .m3-switch input { opacity: 0; width: 0; height: 0; }
  .m3-track {
    position: absolute;
    cursor: pointer;
    inset: 0;
    background-color: var(--m3-surface-container-high);
    border: 2px solid var(--m3-outline);
    border-radius: 32px;
    transition: 0.25s cubic-bezier(0.4, 0, 0.2, 1);
  }
  .m3-thumb {
    position: absolute;
    content: "";
    height: 16px;
    width: 16px;
    left: 6px;
    bottom: 6px;
    background-color: var(--m3-outline);
    border-radius: 50%;
    transition: 0.25s cubic-bezier(0.4, 0, 0.2, 1);
  }
  .m3-switch input:checked + .m3-track {
    background-color: var(--m3-primary);
    border-color: var(--m3-primary);
  }
  .m3-switch input:checked + .m3-track .m3-thumb {
    transform: translateX(20px);
    width: 22px;
    height: 22px;
    left: 4px;
    bottom: 3px;
    background-color: var(--m3-on-primary);
  }

  /* Species row */
  .species-grid {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 10px;
  }
  .species-card {
    background: var(--m3-surface);
    border: 2px solid transparent;
    border-radius: 18px;
    padding: 12px 6px;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 8px;
    cursor: pointer;
    transition: 0.2s ease;
  }
  .species-card.selected {
    background: var(--m3-secondary-container);
    border-color: var(--m3-primary);
  }
  .species-card span {
    font-size: 13px;
    font-weight: 600;
  }

  /* Palettes */
  .palette-grid {
    display: flex;
    justify-content: space-between;
    gap: 8px;
    padding: 6px 0;
  }
  .palette-swatch {
    width: 44px;
    height: 44px;
    border-radius: 50%;
    cursor: pointer;
    border: 3px solid transparent;
    position: relative;
    transition: 0.15s ease;
    box-shadow: 0 2px 4px rgba(0,0,0,0.12);
  }
  .palette-swatch.selected {
    outline: 2px solid var(--m3-primary);
    outline-offset: 3px;
  }
  .palette-swatch svg {
    display: none;
    position: absolute;
    inset: 0;
    margin: auto;
    fill: #fff;
    width: 20px;
    height: 20px;
  }
  .palette-swatch.selected svg { display: block; }

  /* Chips */
  .chip-group {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
  }
  .m3-chip {
    background: var(--m3-surface);
    border: 1px solid var(--m3-outline-variant);
    border-radius: 14px;
    padding: 7px 14px;
    font-size: 13px;
    font-weight: 500;
    cursor: pointer;
    color: var(--m3-on-surface);
    transition: 0.15s ease;
  }
  .m3-chip.selected {
    background: var(--m3-primary-container);
    color: var(--m3-on-primary-container);
    border-color: var(--m3-primary);
    font-weight: 600;
  }

  /* Buttons */
  .btn-row {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 10px;
  }
  .btn-row-3 {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 8px;
  }
  .btn-tonal {
    background: var(--m3-secondary-container);
    color: var(--m3-on-secondary-container);
    border: none;
    border-radius: 14px;
    padding: 11px 12px;
    font-size: 13px;
    font-weight: 600;
    cursor: pointer;
    text-align: center;
    transition: 0.15s ease;
  }
  .btn-tonal:hover { opacity: 0.88; }
  .btn-primary {
    background: var(--m3-primary);
    color: var(--m3-on-primary);
    border: none;
    border-radius: 14px;
    padding: 11px 12px;
    font-size: 13px;
    font-weight: 600;
    cursor: pointer;
    transition: 0.15s ease;
  }
  .btn-outline {
    background: transparent;
    border: 1.5px solid var(--m3-outline-variant);
    color: var(--m3-primary);
    border-radius: 14px;
    padding: 10px 12px;
    font-size: 13px;
    font-weight: 600;
    cursor: pointer;
    transition: 0.15s ease;
  }

  /* Slider & Custom size input */
  .slider-row {
    display: flex;
    flex-direction: column;
    gap: 8px;
  }
  .slider-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
  }
  .slider-header span { font-size: 13px; font-weight: 500; }
  .size-input-wrap {
    display: flex;
    align-items: center;
    gap: 6px;
  }
  .size-input-wrap input {
    width: 68px;
    padding: 4px 8px;
    border-radius: 10px;
    border: 1.5px solid var(--m3-primary);
    background: var(--m3-surface);
    color: var(--m3-primary);
    font-size: 14px;
    font-weight: 700;
    text-align: center;
  }
  input[type=range] {
    -webkit-appearance: none;
    width: 100%;
    height: 8px;
    background: var(--m3-surface-container-high);
    border-radius: 6px;
    outline: none;
  }
  input[type=range]::-webkit-slider-thumb {
    -webkit-appearance: none;
    width: 22px;
    height: 22px;
    border-radius: 50%;
    background: var(--m3-primary);
    cursor: pointer;
    box-shadow: 0 1px 4px rgba(0,0,0,0.2);
  }

  /* Skins list */
  .skin-item {
    background: var(--m3-surface);
    border-radius: var(--m3-radius-sm);
    padding: 10px 14px;
    display: flex;
    align-items: center;
    justify-content: space-between;
    border: 1.5px solid transparent;
    cursor: pointer;
    transition: 0.15s ease;
  }
  .skin-item.active {
    background: var(--m3-secondary-container);
    border-color: var(--m3-primary);
  }
  .skin-name { font-size: 13px; font-weight: 600; }
  .skin-meta { font-size: 11px; color: var(--m3-on-surface-variant); }
  .skin-del-btn {
    background: none;
    border: none;
    color: #BA1A1A;
    font-size: 18px;
    cursor: pointer;
    padding: 4px;
  }

  /* Stats Badge */
  .stats-bar {
    display: flex;
    justify-content: space-around;
    padding: 8px 0;
    text-align: center;
  }
  .stat-val { font-size: 18px; font-weight: 700; color: var(--m3-primary); }
  .stat-lbl { font-size: 11px; color: var(--m3-on-surface-variant); }

  /* Toast notification */
  #toast {
    position: fixed;
    bottom: 20px;
    left: 50%;
    transform: translateX(-50%) translateY(100px);
    background: #322F2E;
    color: #F5EFEF;
    padding: 10px 20px;
    border-radius: 18px;
    font-size: 13px;
    box-shadow: 0 4px 12px rgba(0,0,0,0.18);
    transition: 0.3s cubic-bezier(0.175, 0.885, 0.32, 1.275);
    z-index: 1000;
    opacity: 0;
    pointer-events: none;
  }
  #toast.show {
    transform: translateX(-50%) translateY(0);
    opacity: 1;
  }
</style>
</head>
<body>

<div class="container">
  <!-- Top Bar -->
  <div class="top-bar">
    <h1>GUPet</h1>
    <p>Пиксельный питомец</p>
  </div>

  <!-- Hero Card -->
  <div class="hero-card">
    <div class="hero-canvas-wrap">
      <canvas id="hero-canvas" width="108" height="78"></canvas>
    </div>
    <div class="hero-row">
      <div class="hero-info">
        <h2 id="pet-status-title">Питомец активен</h2>
        <p id="pet-status-desc">Отображается поверх окон рабочего стола</p>
      </div>
      <label class="m3-switch">
        <input type="checkbox" id="hero-toggle" checked onchange="togglePet(this.checked)">
        <span class="m3-track"><span class="m3-thumb"></span></span>
      </label>
    </div>
  </div>

  <!-- Section: Species -->
  <div class="section-title">
    <svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor"><path d="M4.5 11c1.38 0 2.5-1.12 2.5-2.5S5.88 6 4.5 6 2 7.12 2 8.5 3.12 11 4.5 11zm15 0c1.38 0 2.5-1.12 2.5-2.5S20.88 6 19.5 6 17 7.12 17 8.5s1.12 2.5 2.5 2.5zm-7-2c1.38 0 2.5-1.12 2.5-2.5S13.88 4 12.5 4 10 5.12 10 6.5 11.12 9 12.5 9zM12 11.5c-3.5 0-6 2.61-6 5.5 0 2.12 1.35 3.5 3.25 3.5 1.18 0 2.19-.57 2.75-1.44.56.87 1.57 1.44 2.75 1.44 1.9 0 3.25-1.38 3.25-3.5 0-2.89-2.5-5.5-6-5.5z"/></svg>
    Выбери питомца
  </div>
  <div class="species-grid">
    <div class="species-card selected" id="species-gupet" onclick="setSpecies('gupet')">
      <canvas id="prev-gupet" width="54" height="39"></canvas>
      <span>GUPet</span>
    </div>
    <div class="species-card" id="species-cat" onclick="setSpecies('cat')">
      <canvas id="prev-cat" width="54" height="39"></canvas>
      <span>Кошка</span>
    </div>
    <div class="species-card" id="species-dog" onclick="setSpecies('dog')">
      <canvas id="prev-dog" width="54" height="39"></canvas>
      <span>Собака</span>
    </div>
  </div>

  <!-- Section: Color & Custom Size -->
  <div class="card">
    <div style="font-size: 14px; font-weight: 600;">Цветовая палитра</div>
    <div class="palette-grid">
      <div class="palette-swatch selected" style="background: #D97757;" onclick="setPalette(0)">
        <svg viewBox="0 0 24 24"><path d="M9 16.2L4.8 12l-1.4 1.4L9 19 21 7l-1.4-1.4L9 16.2z"/></svg>
      </div>
      <div class="palette-swatch" style="background: #4FBF98;" onclick="setPalette(1)">
        <svg viewBox="0 0 24 24"><path d="M9 16.2L4.8 12l-1.4 1.4L9 19 21 7l-1.4-1.4L9 16.2z"/></svg>
      </div>
      <div class="palette-swatch" style="background: #9B8AEE;" onclick="setPalette(2)">
        <svg viewBox="0 0 24 24"><path d="M9 16.2L4.8 12l-1.4 1.4L9 19 21 7l-1.4-1.4L9 16.2z"/></svg>
      </div>
      <div class="palette-swatch" style="background: #4FA8E8;" onclick="setPalette(3)">
        <svg viewBox="0 0 24 24"><path d="M9 16.2L4.8 12l-1.4 1.4L9 19 21 7l-1.4-1.4L9 16.2z"/></svg>
      </div>
      <div class="palette-swatch" style="background: #EE85AE;" onclick="setPalette(4)">
        <svg viewBox="0 0 24 24"><path d="M9 16.2L4.8 12l-1.4 1.4L9 19 21 7l-1.4-1.4L9 16.2z"/></svg>
      </div>
      <div class="palette-swatch" style="background: #697580;" onclick="setPalette(5)">
        <svg viewBox="0 0 24 24"><path d="M9 16.2L4.8 12l-1.4 1.4L9 19 21 7l-1.4-1.4L9 16.2z"/></svg>
      </div>
    </div>

    <!-- Any Size Control -->
    <div class="slider-row" style="margin-top: 6px;">
      <div class="slider-header">
        <span>Размер питомца (любой масштаб)</span>
        <div class="size-input-wrap">
          <input type="number" step="0.1" min="0.5" max="30.0" id="size-input" onchange="applyCustomSize(this.value)">
          <span style="font-size: 13px; font-weight: 600; color: var(--m3-primary);">x</span>
        </div>
      </div>
      <input type="range" min="1.0" max="15.0" step="0.1" id="size-slider" oninput="onSizeSlider(this.value)">
      <div class="chip-group" style="margin-top: 4px;">
        <button class="m3-chip" onclick="setSizePreset(1.5)">Мини (1.5x)</button>
        <button class="m3-chip" onclick="setSizePreset(3.0)">Компакт (3x)</button>
        <button class="m3-chip selected" id="chip-std" onclick="setSizePreset(4.0)">Стандарт (4x)</button>
        <button class="m3-chip" onclick="setSizePreset(6.5)">Большой (6.5x)</button>
        <button class="m3-chip" onclick="setSizePreset(10.0)">Огромный (10x)</button>
        <button class="m3-chip" onclick="setSizePreset(15.0)">Гигант (15x)</button>
      </div>
    </div>
  </div>

  <!-- Section: Poop -->
  <div class="section-title">💩 Кучки</div>
  <div class="card" style="background: var(--m3-secondary-container);">
    <div class="switch-row" style="border: none; padding: 0;">
      <div class="switch-label">
        <span class="switch-title">Самостоятельные кучки</span>
        <span class="switch-desc">Питомец иногда оставляет сюрпризы</span>
      </div>
      <label class="m3-switch">
        <input type="checkbox" id="chk-poop" onchange="updateBool('poop', this.checked)">
        <span class="m3-track"><span class="m3-thumb"></span></span>
      </label>
    </div>
    <div class="btn-row">
      <button class="btn-tonal" style="background: rgba(255,255,255,0.7);" onclick="triggerAction('poop')">Покакать сейчас</button>
      <button class="btn-tonal" style="background: rgba(255,255,255,0.7);" onclick="triggerAction('clean')">Убрать кучки</button>
    </div>
  </div>

  <!-- Section: Behaviors -->
  <div class="section-title">✨ Поведение</div>
  <div class="card">
    <div class="switch-row">
      <div class="switch-label">
        <span class="switch-title">Гуляет сам</span>
        <span class="switch-desc">Ходит, прыгает, машет лапками, выглядывает из края</span>
      </div>
      <label class="m3-switch"><input type="checkbox" id="chk-wander" onchange="updateBool('wander', this.checked)"><span class="m3-track"><span class="m3-thumb"></span></span></label>
    </div>
    <div class="switch-row">
      <div class="switch-label">
        <span class="switch-title">Реплики</span>
        <span class="switch-desc">Всплывающие диалоговые облачка во время игры</span>
      </div>
      <label class="m3-switch"><input type="checkbox" id="chk-phrases" onchange="updateBool('phrases', this.checked)"><span class="m3-track"><span class="m3-thumb"></span></span></label>
    </div>
    <div class="switch-row">
      <div class="switch-label">
        <span class="switch-title">Гравитация и физика</span>
        <span class="switch-desc">Падает вниз, скользит и отскакивает от границ</span>
      </div>
      <label class="m3-switch"><input type="checkbox" id="chk-gravity" onchange="updateBool('gravity', this.checked)"><span class="m3-track"><span class="m3-thumb"></span></span></label>
    </div>
    <div class="switch-row">
      <div class="switch-label">
        <span class="switch-title">Реакция на встряхивание</span>
        <span class="switch-desc">Кувыркается при резком перемещении</span>
      </div>
      <label class="m3-switch"><input type="checkbox" id="chk-shake" onchange="updateBool('shake', this.checked)"><span class="m3-track"><span class="m3-thumb"></span></span></label>
    </div>
    <div class="switch-row">
      <div class="switch-label">
        <span class="switch-title">Звуковой отклик</span>
        <span class="switch-desc">Приятные звуковые щелчки при касаниях и прыжках</span>
      </div>
      <label class="m3-switch"><input type="checkbox" id="chk-haptics" onchange="updateBool('haptics', this.checked)"><span class="m3-track"><span class="m3-thumb"></span></span></label>
    </div>
    <div class="switch-row">
      <div class="switch-label">
        <span class="switch-title">Танцует под музыку</span>
        <span class="switch-desc">Определяет звук системы и пускается в пляс</span>
      </div>
      <label class="m3-switch"><input type="checkbox" id="chk-dance" onchange="updateBool('dance', this.checked)"><span class="m3-track"><span class="m3-thumb"></span></span></label>
    </div>
    <div class="switch-row">
      <div class="switch-label">
        <span class="switch-title">Всегда поверх всех окон</span>
        <span class="switch-desc">Питомец гуляет над браузером, играми и приложениями</span>
      </div>
      <label class="m3-switch"><input type="checkbox" id="chk-topmost" onchange="updateBool('alwaysOnTop', this.checked)"><span class="m3-track"><span class="m3-thumb"></span></span></label>
    </div>
    <div class="switch-row">
      <div class="switch-label">
        <span class="switch-title">Режим призрака</span>
        <span class="switch-desc">Клики мыши проходят насквозь (не мешает в играх)</span>
      </div>
      <label class="m3-switch"><input type="checkbox" id="chk-ghost" onchange="updateBool('ghostMode', this.checked)"><span class="m3-track"><span class="m3-thumb"></span></span></label>
    </div>
    <div class="switch-row">
      <div class="switch-label">
        <span class="switch-title">Автозапуск системы</span>
        <span class="switch-desc">Запускать питомца при входе в систему</span>
      </div>
      <label class="m3-switch"><input type="checkbox" id="chk-autostart" onchange="updateBool('autostart', this.checked)"><span class="m3-track"><span class="m3-thumb"></span></span></label>
    </div>
  </div>

  <!-- Section: Sleep -->
  <div class="card">
    <div style="font-size: 14px; font-weight: 600;">Когда засыпать</div>
    <div class="chip-group">
      <button class="m3-chip" id="sleep-1" onclick="setSleep(1)">1 мин</button>
      <button class="m3-chip selected" id="sleep-3" onclick="setSleep(3)">3 мин</button>
      <button class="m3-chip" id="sleep-5" onclick="setSleep(5)">5 мин</button>
      <button class="m3-chip" id="sleep-10" onclick="setSleep(10)">10 мин</button>
      <button class="m3-chip" id="sleep-0" onclick="setSleep(0)">Никогда</button>
    </div>
  </div>

  <!-- Section: Custom Skins -->
  <div class="card">
    <div style="display: flex; justify-content: space-between; align-items: center;">
      <div style="font-size: 14px; font-weight: 600;">Пользовательские скины</div>
      <label class="btn-outline" style="font-size: 12px; padding: 6px 12px; cursor: pointer;">
        + Импорт .gupet
        <input type="file" id="skin-file-input" accept=".gupet,.zip" style="display: none;" onchange="uploadSkin(this)">
      </label>
    </div>
    <div id="skins-container" style="display: flex; flex-direction: column; gap: 8px;">
      <div class="skin-item active" onclick="selectSkin('')">
        <div>
          <div class="skin-name">Встроенный вид</div>
          <div class="skin-meta">Оригинальный пиксельный питомец</div>
        </div>
      </div>
    </div>
  </div>

  <!-- Section: Quick Actions -->
  <div class="card">
    <div style="font-size: 14px; font-weight: 600;">Быстрые действия</div>
    <div class="btn-row-3">
      <button class="btn-tonal" onclick="triggerAction('recenter')">В центр</button>
      <button class="btn-tonal" onclick="triggerAction('say')">Сказать фразу</button>
      <button class="btn-tonal" onclick="triggerAction('clean')">Очистить</button>
    </div>
  </div>

  <!-- Statistics -->
  <div class="card" style="padding: 14px 20px;">
    <div class="stats-bar">
      <div>
        <div class="stat-val" id="stat-pets">0</div>
        <div class="stat-lbl">Поглаживаний</div>
      </div>
      <div style="border-left: 1px solid rgba(0,0,0,0.08);"></div>
      <div>
        <div class="stat-val" id="stat-poops">0</div>
        <div class="stat-lbl">Кучек убрано</div>
      </div>
    </div>
  </div>
</div>

<div id="toast"></div>

<script>
  let currentSettings = {};
  let currentPalette = 0;
  let currentSpecies = 'gupet';
  let phase = 0;

  const PALETTES = [
    { body: "#D97757", eye: "#37221E", shade: "#B7553F", outline: "#743927", highlight: "#FFAC85" },
    { body: "#4FBF98", eye: "#163328", shade: "#2D9875", outline: "#205E4A", highlight: "#9CE4C6" },
    { body: "#9B8AEE", eye: "#29234A", shade: "#7866CB", outline: "#4B3D8B", highlight: "#D3CAFF" },
    { body: "#4FA8E8", eye: "#163248", shade: "#317DAC", outline: "#20577F", highlight: "#B4E2FF" },
    { body: "#EE85AE", eye: "#4B1F35", shade: "#C95783", outline: "#873958", highlight: "#FFC4D6" },
    { body: "#697580", eye: "#F4F1E8", shade: "#45505D", outline: "#303842", highlight: "#AEB8C1" }
  ];

  function showToast(msg) {
    const t = document.getElementById('toast');
    t.innerText = msg;
    t.classList.add('show');
    setTimeout(() => t.classList.remove('show'), 2500);
  }

  // Draw Pixel Pet on Canvas
  function drawPixelPet(ctx, species, palIdx, scale, phaseVal) {
    ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
    const pal = PALETTES[palIdx] || PALETTES[0];
    const unit = scale;
    const topPad = unit * 2;
    const breathe = Math.sin(phaseVal * 3.1) * 0.35 * unit;

    const gupetL = [5, 4, 3, 3, 3, 4, 5];
    const gupetR = [13, 14, 15, 15, 15, 14, 13];
    const catL = [5, 4, 3, 2, 2, 2, 2, 2, 3, 4];
    const catR = [13, 14, 15, 16, 16, 16, 16, 16, 15, 14];

    ctx.save();
    ctx.translate(0, -breathe);

    function px(x, y, w, h, col) {
      ctx.fillStyle = col;
      ctx.fillRect(Math.round(x * unit), Math.round(topPad + y * unit), Math.round(w * unit), Math.round(h * unit));
    }

    // Shadow
    px(4, 12.5, 10, 0.6, "rgba(53,37,32,0.3)");

    const isGupet = (species === 'gupet');
    const leftArr = isGupet ? gupetL : catL;
    const rightArr = isGupet ? gupetR : catR;
    const startY = isGupet ? 3 : 2;

    // Ears / Tail
    if (species === 'cat') {
      px(4, 2, 4, 4, pal.outline);
      px(5, 1, 3, 4, pal.body);
      px(6, 1, 1, 2, "#FFB2BC");
      px(11, 1, 3, 4, pal.body);
      px(11, 2, 4, 4, pal.outline);
      px(12, 1, 1, 2, "#FFB2BC");
      px(14, 8, 2, 1, pal.outline);
      px(15, 6, 2, 2, pal.body);
    } else if (species === 'dog') {
      px(1, 3, 4, 4, pal.outline);
      px(2, 3, 3, 4, pal.shade);
      px(13, 3, 4, 4, pal.outline);
      px(13, 3, 3, 4, pal.shade);
      px(14, 6, 2, 2, pal.outline);
      px(15, 5, 2, 2, pal.body);
    }

    // Body
    for (let r = 0; r < leftArr.length; r++) {
      const y = startY + r;
      const l = leftArr[r];
      const rgt = rightArr[r];
      px(l - 1, y, rgt - l + 2, 1, pal.outline);
      px(l, y, rgt - l, 1, pal.body);
      if (r >= 2 && r <= 6) px(rgt - 2, y, 1, 1, pal.shade);
      if (r >= 1 && r <= 4) px(l + 1, y, 1, 1, pal.highlight);
    }

    // Legs
    const legs = isGupet ? [4, 6, 11, 13] : [4, 7, 11, 14];
    const legY = isGupet ? 10 : 11;
    for (let i = 0; i < legs.length; i++) {
      px(legs[i], legY, 2, 1, pal.outline);
      px(legs[i], legY, 1, 1, pal.body);
    }

    // Arms
    px(1, 7, 2, 2, pal.outline);
    px(2, 7, 1, 2, pal.body);
    px(16, 7, 2, 2, pal.outline);
    px(16, 7, 1, 2, pal.body);

    // Eyes
    const blink = Math.floor(phaseVal * 1.5) % 8 === 0;
    const eyeY = 5;
    const eyeLeft = isGupet ? 6 : 5;
    const eyeRight = 11;

    if (blink) {
      px(eyeLeft - 1, eyeY + 1, 3, 1, pal.eye);
      px(eyeRight - 1, eyeY + 1, 3, 1, pal.eye);
    } else {
      px(eyeLeft, eyeY, 1, 2, pal.eye);
      px(eyeLeft, eyeY, 1, 1, "#FFFFFF");
      px(eyeRight, eyeY, 1, 2, pal.eye);
      px(eyeRight, eyeY, 1, 1, "#FFFFFF");
    }

    // Mouth / Muzzle
    if (species === 'dog') {
      px(7, 7, 4, 2, "#FFE2BE");
      px(8, 7, 2, 1, pal.outline);
    } else if (species === 'cat') {
      px(8, 7, 1, 1, "#FFA9B7");
      px(7, 8, 2, 1, pal.outline);
    } else {
      px(8, 7, 1, 1, pal.outline);
      px(9, 8, 1, 1, pal.outline);
    }

    ctx.restore();
  }

  // Animation Loop
  function animLoop() {
    phase += 0.04;
    const heroCtx = document.getElementById('hero-canvas').getContext('2d');
    drawPixelPet(heroCtx, currentSpecies, currentPalette, 5.8, phase);

    // Mini previews
    drawPixelPet(document.getElementById('prev-gupet').getContext('2d'), 'gupet', currentPalette, 2.8, phase);
    drawPixelPet(document.getElementById('prev-cat').getContext('2d'), 'cat', currentPalette, 2.8, phase);
    drawPixelPet(document.getElementById('prev-dog').getContext('2d'), 'dog', currentPalette, 2.8, phase);

    requestAnimationFrame(animLoop);
  }

  // Fetch Settings
  async function loadSettings() {
    try {
      const r = await fetch('/api/settings');
      const s = await r.json();
      currentSettings = s;
      currentSpecies = s.species || 'gupet';
      currentPalette = s.palette || 0;

      // Update UI elements
      document.getElementById('hero-toggle').checked = !!s.show;
      document.getElementById('pet-status-title').innerText = s.show ? "Питомец активен" : "Питомец скрыт";
      document.getElementById('pet-status-desc').innerText = s.show ? "Отображается поверх окон рабочего стола" : "Нажмите переключатель, чтобы позвать";

      // Species
      document.querySelectorAll('.species-card').forEach(el => el.classList.remove('selected'));
      const sc = document.getElementById('species-' + currentSpecies);
      if (sc) sc.classList.add('selected');

      // Palette
      document.querySelectorAll('.palette-swatch').forEach((el, i) => {
        el.classList.toggle('selected', i === currentPalette);
      });

      // Size
      const sz = parseFloat(s.sizeScale) || 4.0;
      document.getElementById('size-slider').value = sz;
      document.getElementById('size-input').value = sz.toFixed(1);

      // Checkboxes
      const bKeys = ['wander', 'phrases', 'gravity', 'shake', 'haptics', 'poop', 'dance', 'alwaysOnTop', 'ghostMode', 'autostart'];
      bKeys.forEach(k => {
        const el = document.getElementById('chk-' + k);
        if (el) el.checked = !!s[k];
      });

      // Sleep
      const mins = s.sleepMinutes !== undefined ? s.sleepMinutes : 3;
      document.querySelectorAll('[id^="sleep-"]').forEach(el => el.classList.remove('selected'));
      const selChip = document.getElementById('sleep-' + mins);
      if (selChip) selChip.classList.add('selected');

      // Stats
      document.getElementById('stat-pets').innerText = s.petsCount || 0;
      document.getElementById('stat-poops').innerText = s.poopsCleaned || 0;

      loadSkins();
    } catch (e) {
      console.error(e);
    }
  }

  async function postSettings(diff) {
    try {
      const r = await fetch('/api/settings', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(diff)
      });
      const res = await r.json();
      currentSettings = { ...currentSettings, ...diff };
      return res;
    } catch (e) {
      console.error(e);
    }
  }

  function togglePet(val) {
    document.getElementById('pet-status-title').innerText = val ? "Питомец активен" : "Питомец скрыт";
    document.getElementById('pet-status-desc').innerText = val ? "Отображается поверх окон рабочего стола" : "Нажмите переключатель, чтобы позвать";
    postSettings({ show: val });
    showToast(val ? "Питомец появился на экране!" : "Питомец спрятался");
  }

  function setSpecies(sp) {
    currentSpecies = sp;
    document.querySelectorAll('.species-card').forEach(el => el.classList.remove('selected'));
    document.getElementById('species-' + sp).classList.add('selected');
    postSettings({ species: sp, customSkinId: "" });
    showToast("Выбран вид: " + (sp==='cat'?'Кошка':sp==='dog'?'Собака':'GUPet'));
  }

  function setPalette(idx) {
    currentPalette = idx;
    document.querySelectorAll('.palette-swatch').forEach((el, i) => {
      el.classList.toggle('selected', i === idx);
    });
    postSettings({ palette: idx });
  }

  function onSizeSlider(val) {
    const f = parseFloat(val);
    document.getElementById('size-input').value = f.toFixed(1);
    postSettings({ sizeScale: f });
  }

  function applyCustomSize(val) {
    let f = parseFloat(val);
    if (isNaN(f) || f <= 0.1) f = 4.0;
    document.getElementById('size-slider').value = Math.min(15.0, Math.max(1.0, f));
    document.getElementById('size-input').value = f.toFixed(1);
    postSettings({ sizeScale: f });
    showToast("Масштаб установлен: " + f.toFixed(1) + "x");
  }

  function setSizePreset(val) {
    applyCustomSize(val);
  }

  function updateBool(key, val) {
    postSettings({ [key]: val });
  }

  function setSleep(mins) {
    document.querySelectorAll('[id^="sleep-"]').forEach(el => el.classList.remove('selected'));
    const sel = document.getElementById('sleep-' + mins);
    if (sel) sel.classList.add('selected');
    postSettings({ sleepMinutes: mins });
    showToast(mins === 0 ? "Сон отключён" : "Засыпать через " + mins + " мин");
  }

  async function triggerAction(act) {
    try {
      await fetch('/api/action', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action: act })
      });
      if (act === 'recenter') showToast("Питомец вернулся в центр экрана!");
      if (act === 'poop') showToast("Питомец сделал кучку!");
      if (act === 'clean') showToast("Все кучки очищены!");
      if (act === 'say') showToast("Питомец говорит!");
    } catch (e) {
      console.error(e);
    }
  }

  async function loadSkins() {
    try {
      const r = await fetch('/api/skins');
      const skins = await r.json();
      const cont = document.getElementById('skins-container');
      cont.innerHTML = `
        <div class="skin-item ${!currentSettings.customSkinId ? 'active' : ''}" onclick="selectSkin('')">
          <div>
            <div class="skin-name">Встроенный вид</div>
            <div class="skin-meta">Оригинальный пиксельный питомец</div>
          </div>
        </div>
      `;
      skins.forEach(sk => {
        const item = document.createElement('div');
        item.className = 'skin-item' + (currentSettings.customSkinId === sk.id ? ' active' : '');
        item.onclick = (e) => {
          if (e.target.closest('.skin-del-btn')) return;
          selectSkin(sk.id);
        };
        item.innerHTML = `
          <div>
            <div class="skin-name">${escapeHtml(sk.name)}</div>
            <div class="skin-meta">Автор: ${escapeHtml(sk.author || 'Неизвестен')}</div>
          </div>
          <button class="skin-del-btn" onclick="deleteSkin('${sk.id}')" title="Удалить">&times;</button>
        `;
        cont.appendChild(item);
      });
    } catch (e) {
      console.error(e);
    }
  }

  function selectSkin(id) {
    postSettings({ customSkinId: id });
    loadSkins();
    showToast(id ? "Выбран пользовательский скин" : "Сброшено на встроенный вид");
  }

  async function deleteSkin(id) {
    if (!confirm("Удалить этот скин?")) return;
    try {
      await fetch('/api/skins/' + id, { method: 'DELETE' });
      showToast("Скин удалён");
      loadSkins();
    } catch (e) {
      console.error(e);
    }
  }

  async function uploadSkin(input) {
    if (!input.files || !input.files[0]) return;
    const file = input.files[0];
    const form = new FormData();
    form.append('file', file);
    try {
      const r = await fetch('/api/import_skin', { method: 'POST', body: form });
      const res = await r.json();
      if (res.ok) {
        showToast("Скин «" + res.name + "» успешно импортирован!");
        loadSettings();
      } else {
        showToast("Ошибка: " + (res.error || "не удалось"));
      }
    } catch (e) {
      showToast("Ошибка загрузки скина");
    }
    input.value = "";
  }

  function escapeHtml(str) {
    return (str || '').replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
  }

  // Start
  animLoop();
  loadSettings();
</script>

</body>
</html>
)rawliteral";

} // namespace GUPet
