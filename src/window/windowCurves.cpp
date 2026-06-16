#include "window.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// CPU-side monotone cubic Hermite spline (mirrors GLSL evalCurve).
// px / py are the n control-point x and y values in [0,1].
// Values outside [px[0], px[n-1]] are linearly extrapolated so that HDR
// values above 1.0 pass through rather than being hard-clipped.
// ---------------------------------------------------------------------------
static float evalCurveCPU(float v, const float* px, const float* py, int n)
{
    // Linear extrapolation below black point
    if (v < px[0]) {
        float dx0 = std::max(px[1] - px[0], 0.0001f);
        float s0  = (py[1] - py[0]) / dx0;
        return py[0] + s0 * (v - px[0]);
    }

    // Linear extrapolation above white point
    if (v > px[n - 1]) {
        float dxN = std::max(px[n-1] - px[n-2], 0.0001f);
        float sN  = (py[n-1] - py[n-2]) / dxN;
        return py[n-1] + sN * (v - px[n-1]);
    }

    // Find segment
    int seg = n - 2;
    for (int i = 0; i < n - 1; i++) {
        if (v < px[i + 1]) { seg = i; break; }
    }
    int pprev = std::max(seg - 1, 0);
    int pnext = std::min(seg + 2, n - 1);

    float pax = px[seg],    pay = py[seg];
    float pbx = px[seg+1],  pby = py[seg+1];

    float dx = pbx - pax;
    if (dx < 0.0001f) return pay;

    float dxPrev = std::max(pbx       - px[pprev], 0.0001f);
    float dxNext = std::max(px[pnext] - pax,       0.0001f);
    float m0 = (pby       - py[pprev]) / dxPrev;
    float m1 = (py[pnext] - pay)       / dxNext;

    float delta = (pby - pay) / dx;
    if (std::abs(delta) < 0.0001f) {
        m0 = 0.0f; m1 = 0.0f;
    } else {
        m0 = std::clamp(m0 / delta, -3.0f, 3.0f) * delta;
        m1 = std::clamp(m1 / delta, -3.0f, 3.0f) * delta;
    }

    float u  = (v - pax) / dx;
    float u2 = u  * u;
    float u3 = u2 * u;
    float h00 =  2.0f*u3 - 3.0f*u2 + 1.0f;
    float h10 =       u3 - 2.0f*u2 + u;
    float h01 = -2.0f*u3 + 3.0f*u2;
    float h11 =       u3 -      u2;

    return h00*pay + h10*dx*m0 + h01*pby + h11*dx*m1;
}

// ---------------------------------------------------------------------------
// curvesEditor — draws the RGBW tone-curve UI.
// Call from inside a valid ImGui window after BeginChild / at any nesting level.
// Sets paramChange = true whenever the user moves a control point.
// ---------------------------------------------------------------------------
void mainWindow::curvesEditor(bool& paramChange)
{
    if (!validIm()) return;

    // Persistent UI state (per-channel drag tracking)
    static int activeChannel = 0;   // 0=W, 1=R, 2=G, 3=B
    static int dragPoint     = -1;  // index of the point being dragged, -1 = none

    // -----------------------------------------------------------------------
    // Tab bar
    // -----------------------------------------------------------------------
    const char*  tabNames[]  = { "W", "R", "G", "B" };
    const ImVec4 tabColors[] = {
        { 0.82f, 0.82f, 0.82f, 1.0f },   // W — neutral white
        { 0.85f, 0.28f, 0.28f, 1.0f },   // R — red
        { 0.28f, 0.78f, 0.28f, 1.0f },   // G — green
        { 0.30f, 0.52f, 0.92f, 1.0f },   // B — blue
    };

    if (ImGui::BeginTabBar("##CurveTabs")) {
        for (int i = 0; i < 4; i++) {
            ImVec4 tc   = tabColors[i];
            ImVec4 dim  = { tc.x * 0.45f, tc.y * 0.45f, tc.z * 0.45f, 0.85f };
            ImVec4 mid  = { tc.x * 0.65f, tc.y * 0.65f, tc.z * 0.65f, 1.0f  };
            ImGui::PushStyleColor(ImGuiCol_Tab,        dim);
            ImGui::PushStyleColor(ImGuiCol_TabSelected, tc);
            ImGui::PushStyleColor(ImGuiCol_TabHovered, mid);
            if (ImGui::BeginTabItem(tabNames[i])) {
                activeChannel = i;
                ImGui::EndTabItem();
            }
            ImGui::PopStyleColor(3);
        }
        ImGui::EndTabBar();
    }

    // -----------------------------------------------------------------------
    // Canvas geometry — square, fills the available content width.
    // -----------------------------------------------------------------------
    const float availW   = ImGui::GetContentRegionAvail().x * 0.985f;
    const float cSize    = availW;
    const float cOffsetX = (availW - cSize) * 0.5f;
    const ImVec2 cPos    = { ImGui::GetCursorScreenPos().x + cOffsetX,
                             ImGui::GetCursorScreenPos().y };
    const ImVec2 cEnd    = { cPos.x + cSize, cPos.y + cSize };

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Colors
    const ImU32 colBg       = IM_COL32( 28,  28,  28, 255);
    const ImU32 colBorder   = IM_COL32( 85,  85,  85, 255);
    const ImU32 colGrid     = IM_COL32( 55,  55,  55, 255);
    const ImU32 colDiag     = IM_COL32( 75,  75,  75, 255);
    const ImVec4& tc        = tabColors[activeChannel];
    const ImU32 colCurve    = IM_COL32(
        (int)(tc.x * 210), (int)(tc.y * 210), (int)(tc.z * 210), 255);
    const ImU32 colPoint    = IM_COL32(210, 210, 210, 255);
    const ImU32 colPointHov = IM_COL32(255, 240,  60, 255);
    const ImU32 colPointDel = IM_COL32(255,  80,  80, 255);  // alt-hover: delete hint
    const float uiScale     = ImGui::GetFontSize() / 18.0f;
    const float ptR         = 5.0f * uiScale;

    // -----------------------------------------------------------------------
    // Coordinate helpers
    //   toScreen(nx, ny)  — normalised [0,1]² → screen px
    //   fromScreen(p)     — screen px → normalised [0,1]²
    //   (y-axis flipped: ny=0 → canvas bottom, ny=1 → canvas top)
    // -----------------------------------------------------------------------
    auto toScreen = [&](float nx, float ny) -> ImVec2 {
        return { cPos.x + nx * cSize,
                 cPos.y + (1.0f - ny) * cSize };
    };
    auto fromScreen = [&](ImVec2 p) -> ImVec2 {
        return { (p.x - cPos.x) / cSize,
                 1.0f - (p.y - cPos.y) / cSize };
    };

    // -----------------------------------------------------------------------
    // Draw canvas — background, grid, diagonal, and curve clipped to the
    // canvas rectangle.  Control-point circles drawn after the pop.
    // -----------------------------------------------------------------------
    dl->PushClipRect(cPos, cEnd, true);

    dl->AddRectFilled(cPos, cEnd, colBg);

    // Quarter-grid
    for (int i = 1; i < 4; i++) {
        float t  = i * 0.25f;
        float gx = cPos.x + t * cSize;
        float gy = cPos.y + t * cSize;
        dl->AddLine({ gx, cPos.y }, { gx, cEnd.y }, colGrid);
        dl->AddLine({ cPos.x, gy }, { cEnd.x, gy }, colGrid);
    }

    // Identity diagonal
    dl->AddLine(cPos, cEnd, colDiag, 1.0f);

    // Spline curve (64 segments)
    imageCurve& curve = activeImage()->imgParam.curves[activeChannel];
    const int n = curve.n();
    const int STEPS = 64;
    ImVec2 prev = toScreen(0.0f, evalCurveCPU(0.0f, curve.px.data(), curve.py.data(), n));
    for (int s = 1; s <= STEPS; s++) {
        float x = (float)s / STEPS;
        float y = evalCurveCPU(x, curve.px.data(), curve.py.data(), n);
        ImVec2 cur = toScreen(x, y);
        dl->AddLine(prev, cur, colCurve, 1.8f);
        prev = cur;
    }

    dl->PopClipRect();

    // Border drawn after pop so it always renders fully on the clip boundary
    dl->AddRect(cPos, cEnd, colBorder, 0.0f, 0, 1.5f);

    // -----------------------------------------------------------------------
    // Invisible button to capture mouse events.
    // Padded outward by ptR so edge control-point circles still register.
    // -----------------------------------------------------------------------
    ImVec2 hitPos  = { cPos.x - ptR, cPos.y - ptR };
    ImVec2 hitSize = { cSize + ptR * 2.0f, cSize + ptR * 2.0f };
    ImGui::SetCursorScreenPos(hitPos);
    ImGui::InvisibleButton("##curve_canvas", hitSize);
    const bool canvasHov  = ImGui::IsItemHovered();
    const bool mouseDown  = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    const bool mouseClick = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    const bool mouseRel   = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
    const ImVec2 mPos     = ImGui::GetMousePos();

    // Release drag
    if (mouseRel)
        dragPoint = -1;

    if (dragPoint == -1 && canvasHov && mouseClick) {
        // -----------------------------------------------------------------------
        // Find closest point within grab radius
        // -----------------------------------------------------------------------
        int hitIdx = -1;
        float hitDist = ptR + 4.0f * uiScale;
        for (int i = 0; i < n; i++) {
            ImVec2 ps   = toScreen(curve.px[i], curve.py[i]);
            float  dist = std::hypot(mPos.x - ps.x, mPos.y - ps.y);
            if (dist < hitDist) { hitDist = dist; hitIdx = i; }
        }

        if (hitIdx >= 0) {
            // -----------------------------------------------------------------------
            // Click on existing point
            // -----------------------------------------------------------------------
            if (altHeld) {
                // Alt + click: remove interior points only (keep endpoints)
                if (hitIdx != 0 && hitIdx != n - 1 && n > 2) {
                    curve.px.erase(curve.px.begin() + hitIdx);
                    curve.py.erase(curve.py.begin() + hitIdx);
                    paramChange = true;
                }
            } else {
                // Normal click: start dragging
                dragPoint = hitIdx;
            }
        } else if (!altHeld && n < CURVE_MAX_PTS) {
            // -----------------------------------------------------------------------
            // Click on empty canvas: insert a new point on the curve line.
            // The inserted y is the current spline value at that x so the
            // curve shape is unchanged — the user then drags to taste.
            // -----------------------------------------------------------------------
            ImVec2 norm = fromScreen(mPos);
            float nx = std::clamp(norm.x, 0.001f, 0.999f);

            // Don't insert if too close to an existing point's x
            bool tooClose = false;
            for (int i = 0; i < n; i++) {
                if (std::fabs(nx - curve.px[i]) < 0.01f) { tooClose = true; break; }
            }
            if (!tooClose) {
                float ny = std::clamp(
                    evalCurveCPU(nx, curve.px.data(), curve.py.data(), n),
                    0.0f, 1.0f);

                // Find sorted insertion position
                int insertAt = n - 1;
                for (int i = 1; i < n; i++) {
                    if (nx < curve.px[i]) { insertAt = i; break; }
                }
                curve.px.insert(curve.px.begin() + insertAt, nx);
                curve.py.insert(curve.py.begin() + insertAt, ny);
                dragPoint   = insertAt;   // immediately draggable
                paramChange = true;
            }
        }
    }

    // Update dragged point
    if (dragPoint >= 0 && mouseDown) {
        const int np = curve.n();  // may have changed after insert
        ImVec2 norm = fromScreen(mPos);
        float nx = std::clamp(norm.x, 0.0f, 1.0f);
        float ny = std::clamp(norm.y, 0.0f, 1.0f);

        // X constraints: endpoints slide freely within [0,1]; interior points
        // stay between their immediate neighbours with a small gap.
        if (dragPoint == 0) {
            float hi = curve.px[1] - 0.02f;
            nx = std::clamp(nx, 0.0f, std::max(hi, 0.0f));
        } else if (dragPoint == np - 1) {
            float lo = curve.px[np - 2] + 0.02f;
            nx = std::clamp(nx, std::min(lo, 1.0f), 1.0f);
        } else {
            float lo = curve.px[dragPoint - 1] + 0.02f;
            float hi = curve.px[dragPoint + 1] - 0.02f;
            if (lo < hi) nx = std::clamp(nx, lo, hi);
            else         nx = (lo + hi) * 0.5f;
        }

        if (curve.px[dragPoint] != nx || curve.py[dragPoint] != ny) {
            curve.px[dragPoint] = nx;
            curve.py[dragPoint] = ny;
            paramChange = true;
        }
    }

    // -----------------------------------------------------------------------
    // Draw control points on top of the curve
    // -----------------------------------------------------------------------
    for (int i = 0; i < curve.n(); i++) {
        ImVec2 ps   = toScreen(curve.px[i], curve.py[i]);
        float  dist = std::hypot(mPos.x - ps.x, mPos.y - ps.y);
        bool   hov  = (dist < ptR + 5.0f * uiScale) && canvasHov;
        bool   canDelete = altHeld && i != 0 && i != curve.n() - 1 && curve.n() > 2;

        ImU32 col;
        if (dragPoint == i)       col = colPointHov;
        else if (hov && canDelete) col = colPointDel;
        else if (hov)              col = colPointHov;
        else                       col = colPoint;

        dl->AddCircleFilled(ps, ptR, col);
        dl->AddCircle(ps, ptR, IM_COL32(0, 0, 0, 180), 0, 1.5f);
    }

    // Advance cursor past the canvas
    ImGui::SetCursorScreenPos({ cPos.x, cEnd.y + 4.0f });

    // -----------------------------------------------------------------------
    // Reset buttons — centered under the canvas
    // -----------------------------------------------------------------------
    ImGui::Spacing();
    {
        const float sp     = ImGui::GetStyle().ItemSpacing.x;
        const float fp     = ImGui::GetStyle().FramePadding.x * 2.0f;
        const float btn1W  = ImGui::CalcTextSize("Reset Curve").x      + fp;
        const float btn2W  = ImGui::CalcTextSize("Reset All Curves").x + fp;
        const float totalW = btn1W + sp + btn2W;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availW - totalW) * 0.5f);
        if (ImGui::Button("Reset Curve")) {
            curve.reset();
            dragPoint   = -1;
            paramChange = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset All Curves")) {
            activeImage()->imgParam.rst_curves();
            dragPoint   = -1;
            paramChange = true;
        }
    }
    ImGui::Spacing();
}
