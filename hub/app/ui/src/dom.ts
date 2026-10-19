type Child = Node | string | number | null | undefined | false;
type Props = Record<string, unknown>;

/** Builds an element. `on*` props become listeners, `class` sets the class, everything else a property or attribute. */
export function h<K extends keyof HTMLElementTagNameMap>(
  tag: K,
  props: Props | null = null,
  ...children: (Child | Child[])[]
): HTMLElementTagNameMap[K] {
  const el = document.createElement(tag);
  for (const [k, v] of Object.entries(props ?? {})) {
    if (v === undefined || v === null || v === false) continue;
    if (k.startsWith("on") && typeof v === "function") el.addEventListener(k.slice(2).toLowerCase(), v as EventListener);
    else if (k === "class") el.className = String(v);
    else if (k in el) (el as unknown as Props)[k] = v;
    else el.setAttribute(k, v === true ? "" : String(v));
  }
  for (const c of children.flat()) {
    if (c === null || c === undefined || c === false) continue;
    el.append(c instanceof Node ? c : String(c));
  }
  return el;
}

export function clear(el: Element): void {
  while (el.firstChild) el.removeChild(el.firstChild);
}

/** Pixel-art picture from rows of characters; `.` is empty. */
export function pixelArt(rows: string[], palette: Record<string, string>, scale: number): HTMLCanvasElement {
  const c = document.createElement("canvas");
  c.width = rows[0].length * scale;
  c.height = rows.length * scale;
  c.className = "pixel";
  const g = c.getContext("2d")!;
  rows.forEach((r, y) =>
    [...r].forEach((ch, x) => {
      if (palette[ch]) {
        g.fillStyle = palette[ch];
        g.fillRect(x * scale, y * scale, scale, scale);
      }
    }),
  );
  return c;
}

export const FLOWER = [
  "................",
  "................",
  "...w.ww.ww.w....",
  "..wwwwwwwwwww...",
  "..wwywwywwyww...",
  "..wwwwwwwwwww...",
  "...w.ww.ww.w....",
  ".......g........",
  ".......g........",
  "....g..g..g.....",
  ".....g.g.g......",
  "......ggg.......",
  ".......g........",
  ".......g........",
  ".......g........",
  "................",
];
export const FLOWER_PALETTE = { w: "#ff77a8", y: "#fff1e8", g: "#00e436" };
