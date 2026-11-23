from ..constants import PhysicalProperties, VisualProperties

_FIELD_PHYS = PhysicalProperties.Field
_FIELD_VIS = VisualProperties.Field
_DISPLAY = VisualProperties.Display


# --- HELPERS DE CONVERSIÓN Y DIBUJO DUAL (Tkinter + PIL) ---
def _mm_to_px(value_mm: float) -> float:
    return value_mm * _DISPLAY.SCALE_PX_PER_MM


def _pos_to_px(x_mm: float, y_mm: float) -> tuple[float, float]:
    """Convierte coordenadas locales de la cancha (mm desde esquina sup. izq.) a píxeles."""
    return (
        _DISPLAY.LEFT_PADDING_PX + _mm_to_px(x_mm),
        _DISPLAY.UPPER_PADDING_PX + _mm_to_px(y_mm),
    )


def _draw_line(canvas, draw_context, p1: tuple[float, float], p2: tuple[float, float],
               color: str = _FIELD_VIS.LINE_COLOR, width: int = _FIELD_VIS.LINE_WIDTH_PX) -> None:
    coords = [p1[0], p1[1], p2[0], p2[1]]
    canvas.create_line(*coords, fill=color, width=width)
    if draw_context:
        draw_context.line(coords, fill=color, width=width)


def _draw_rect(canvas, draw_context, p1: tuple[float, float], p2: tuple[float, float],
               fill: str | None = None, outline: str = _FIELD_VIS.LINE_COLOR,
               width: int = _FIELD_VIS.LINE_WIDTH_PX) -> None:
    # PIL exige x0 <= x1 y y0 <= y1
    bbox = [min(p1[0], p2[0]), min(p1[1], p2[1]), max(p1[0], p2[0]), max(p1[1], p2[1])]
    canvas.create_rectangle(*bbox, fill=fill or "", outline=outline, width=width)
    if draw_context:
        draw_context.rectangle(bbox, fill=fill, outline=outline, width=width)


def _draw_oval(canvas, draw_context, center: tuple[float, float], radius_px: float,
               fill: str | None = None, outline: str | None = _FIELD_VIS.LINE_COLOR,
               width: int = _FIELD_VIS.LINE_WIDTH_PX) -> None:
    cx, cy = center
    bbox = [cx - radius_px, cy - radius_px, cx + radius_px, cy + radius_px]
    canvas.create_oval(*bbox, fill=fill or "", outline=outline or "", width=width)
    if draw_context:
        draw_context.ellipse(bbox, fill=fill, outline=outline, width=width)


# --- FUNCIONES PÚBLICAS ---

def draw_visual_circle(canvas, draw_context, x_mm: float, y_mm: float,
                       color: str = _FIELD_VIS.LINE_COLOR) -> None:
    center = _pos_to_px(x_mm, y_mm)
    radius_px = _mm_to_px(_FIELD_VIS.MARK_CIRCLE_RADIUS_MM)
    _draw_oval(canvas, draw_context, center, radius_px, fill=color, outline=None, width=0)


def draw_visual_cross(canvas, draw_context, x_mm: float, y_mm: float,
                      color: str = _FIELD_VIS.LINE_COLOR) -> None:
    cx, cy = _pos_to_px(x_mm, y_mm)
    half = _mm_to_px(_FIELD_VIS.MARK_CROSS_SIZE_MM) / 2
    _draw_line(canvas, draw_context, (cx - half, cy - half), (cx + half, cy + half), color=color)
    _draw_line(canvas, draw_context, (cx + half, cy - half), (cx - half, cy + half), color=color)


def draw_field(canvas, draw_context) -> None:
    w = _FIELD_PHYS.WIDTH_MM
    h = _FIELD_PHYS.HEIGHT_MM
    c = _FIELD_PHYS.CORNER_LENGTH_MM

    # 1. Borde del campo
    _draw_rect(canvas, draw_context, _pos_to_px(0, 0), _pos_to_px(w, h), fill=_FIELD_VIS.BG_COLOR)

    # 2. Esquinas a 45°
    corners = [
        ((c, 0), (0, c)),             # Superior izquierda
        ((w - c, 0), (w, c)),         # Superior derecha
        ((c, h), (0, h - c)),         # Inferior izquierda
        ((w - c, h), (w, h - c)),     # Inferior derecha
    ]
    for p1_mm, p2_mm in corners:
        _draw_line(canvas, draw_context, _pos_to_px(*p1_mm), _pos_to_px(*p2_mm))

    # 3. Línea central y círculo central
    _draw_line(canvas, draw_context, _pos_to_px(w / 2, 0), _pos_to_px(w / 2, h))
    _draw_oval(
        canvas,
        draw_context,
        _pos_to_px(w / 2, h / 2),
        _mm_to_px(_FIELD_VIS.CENTER_CIRCLE_RADIUS_MM),
    )

    # 4. Áreas chicas (izquierda y derecha)
    area_d = _FIELD_VIS.AREA_DEPTH_MM
    area_y1 = (h - _FIELD_VIS.AREA_HEIGHT_MM) / 2
    area_y2 = (h + _FIELD_VIS.AREA_HEIGHT_MM) / 2
    _draw_rect(canvas, draw_context, _pos_to_px(0, area_y1), _pos_to_px(area_d, area_y2))
    _draw_rect(canvas, draw_context, _pos_to_px(w - area_d, area_y1), _pos_to_px(w, area_y2))

    # 5. Porterías (izquierda y derecha)
    goal_d = _FIELD_PHYS.GOAL_DEPTH_MM
    goal_y1 = (h - _FIELD_PHYS.GOAL_WIDTH_MM) / 2
    goal_y2 = (h + _FIELD_PHYS.GOAL_WIDTH_MM) / 2
    _draw_rect(canvas, draw_context, _pos_to_px(-goal_d, goal_y1), _pos_to_px(0, goal_y2))
    _draw_rect(canvas, draw_context, _pos_to_px(w, goal_y1), _pos_to_px(w + goal_d, goal_y2))

    # 6. Marcas reglamentarias (círculos y cruces)
    for x_mm, y_mm in _FIELD_VIS.FREE_BALL_CIRCLES_MM:
        draw_visual_circle(canvas, draw_context, x_mm, y_mm)

    for x_mm, y_mm in _FIELD_VIS.FREE_BALL_CROSSES_MM:
        draw_visual_cross(canvas, draw_context, x_mm, y_mm)