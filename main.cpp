// ImGui DX11 UI - Borderless Topmost Chinese Edition
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <cstdio>

static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;
static int                      g_winW = 780;
static int                      g_winH = 660;
static HWND                     g_hwnd = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_NCHITTEST: {
        // Enable dragging on empty title bar area WITHOUT breaking ImGui clicks
        LRESULT hit = DefWindowProcW(hWnd, msg, wParam, lParam);
        if (hit == HTCLIENT)
        {
            POINT pt = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
            ScreenToClient(hWnd, &pt);
            RECT rc;
            GetClientRect(hWnd, &rc);
            // Title bar zone: y 0..35, but exclude right 100px (exit button)
            if (pt.y >= 0 && pt.y < 36 && pt.x > 0 && pt.x < rc.right - 100)
                return HTCAPTION;
        }
        return hit;
    }
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ============================================================
// Aimbot
// ============================================================
struct AimbotSettings {
    bool    enabled = false;
    float   fov = 120.0f;
    float   smooth = 4.0f;
    int     bone = 0;
    float   max_distance = 200.0f;
    bool    visible_check = true;
    bool    draw_fov = true;
    float   fov_color[4] = { 1.0f, 0.0f, 0.0f, 0.4f };
};

// ============================================================
// ESP
// ============================================================
struct ESPSettings {
    bool    skeleton = false;
    bool    box = true;
    bool    snapline = false;
    bool    weapon = false;
    bool    name = false;
    bool    health = true;
    bool    team = false;
    float   max_distance = 300.0f;
    int     box_style = 0;

    float   skeleton_color[4]  = { 1.0f, 1.0f, 1.0f, 1.0f };
    float   box_visible[4]     = { 1.0f, 0.0f, 0.0f, 1.0f };
    float   box_invisible[4]   = { 1.0f, 0.6f, 0.0f, 1.0f };
    float   snapline_color[4]  = { 0.0f, 1.0f, 1.0f, 1.0f };
    float   weapon_color[4]    = { 1.0f, 0.8f, 0.0f, 1.0f };
    float   name_color[4]      = { 1.0f, 1.0f, 1.0f, 1.0f };
    float   health_color_full[4]  = { 0.0f, 1.0f, 0.0f, 1.0f };
    float   health_color_low[4]   = { 1.0f, 0.0f, 0.0f, 1.0f };
    float   team_color[4]      = { 0.3f, 0.6f, 1.0f, 1.0f };
};

// ============================================================
// Items
// ============================================================
struct ItemSettings {
    bool show_weapons = false;
    bool show_vehicles = false;
    bool show_medical = false;
    bool show_throwables = false;
    bool show_armor = false;
    bool show_attachments = false;
    bool show_ammo = false;

    // AR
    bool wpn_m416 = true;     bool wpn_akm = true;
    bool wpn_scarl = false;   bool wpn_m16a4 = false;
    bool wpn_beryl = false;   bool wpn_aug = false;
    bool wpn_groza = false;   bool wpn_qbz = false;
    bool wpn_g36c = false;    bool wpn_ace32 = false;
    bool wpn_mk47 = false;    bool wpn_k2 = false;
    // DMR
    bool wpn_mini14 = false;  bool wpn_sks = false;
    bool wpn_slr = false;     bool wpn_vss = false;
    bool wpn_mk12 = false;    bool wpn_qbu = false;
    bool wpn_dragunov = false;
    // SR
    bool wpn_kar98 = false;   bool wpn_m24 = false;
    bool wpn_awm = false;     bool wpn_win94 = false;
    bool wpn_lynx = false;
    // SMG
    bool wpn_ump45 = true;    bool wpn_vector = false;
    bool wpn_uzi = false;     bool wpn_tommy = false;
    bool wpn_mp5k = false;    bool wpn_bizon = false;
    bool wpn_p90 = false;     bool wpn_js9 = false;
    // SG
    bool wpn_s686 = false;    bool wpn_s1897 = false;
    bool wpn_s12k = false;    bool wpn_dbs = false;
    bool wpn_o12 = false;
    // LMG
    bool wpn_dp28 = false;    bool wpn_m249 = true;
    bool wpn_mg3 = false;
    // Pistol
    bool wpn_p18c = false;    bool wpn_p92 = false;
    bool wpn_p1911 = false;   bool wpn_deagle = false;
    bool wpn_skorpion = false;bool wpn_r1895 = false;
    // Melee
    bool wpn_pan = false;     bool wpn_machete = false;
    bool wpn_crowbar = false; bool wpn_sickle = false;

    float weapon_color[4] = { 1.0f, 0.50f, 0.00f, 1.0f };

    // Vehicles
    bool veh_dacia = true;    bool veh_rally = true;
    bool veh_uaz = false;     bool veh_mirado = false;
    bool veh_coupe = false;   bool veh_zima = false;
    bool veh_blanc = false;   bool veh_pickup = false;
    bool veh_van = false;     bool veh_buggy = false;
    bool veh_motorcycle = false; bool veh_scooter = false;
    bool veh_atv = false;     bool veh_boat = false;
    bool veh_aquarail = false;bool veh_brdm = false;
    bool veh_glider = false;  bool veh_bike = false;
    bool veh_porter = false;  bool veh_foodtruck = false;
    bool veh_pillar = false;

    float vehicle_color[4] = { 0.0f, 0.7f, 1.0f, 1.0f };

    // Medical
    bool medic_bandage = true; bool medic_firstaid = true;
    bool medic_medkit = true;  bool medic_painkiller = false;
    bool medic_energy = false; bool medic_adrenaline = false;
    bool medic_rescue = false;

    float medic_color[4] = { 0.0f, 1.0f, 0.5f, 1.0f };

    // Throwables
    bool throw_frag = true;   bool throw_molotov = false;
    bool throw_smoke = false; bool throw_stun = false;
    bool throw_bz = false;    bool throw_c4 = false;
    bool throw_sticky = false;

    float throw_color[4] = { 0.8f, 0.4f, 0.0f, 1.0f };

    // Armor
    bool armor_helmet1 = true;  bool armor_helmet2 = true;
    bool armor_helmet3 = true;  bool armor_vest1 = true;
    bool armor_vest2 = true;    bool armor_vest3 = true;
    bool armor_bp1 = false;     bool armor_bp2 = false;
    bool armor_bp3 = false;     bool armor_ghillie = false;

    float armor_color[4] = { 0.6f, 0.4f, 0.8f, 1.0f };

    // Attachments
    bool att_reddot = true;   bool att_holo = true;
    bool att_2x = false;      bool att_3x = false;
    bool att_4x = false;      bool att_6x = false;
    bool att_8x = false;      bool att_15x = false;
    bool att_canted = false;
    bool att_supp_ar = true;  bool att_supp_sr = false;
    bool att_supp_smg = false;bool att_supp_pis = false;
    bool att_comp_ar = false; bool att_comp_smg = false;
    bool att_comp_sr = false;
    bool att_fh_ar = false;   bool att_fh_smg = false;
    bool att_fh_sr = false;
    bool att_grip_vert = false;  bool att_grip_ang = false;
    bool att_grip_half = false;  bool att_grip_thumb = false;
    bool att_grip_light = false;
    bool att_mag_ext_ar = true;   bool att_mag_qd_ar = false;
    bool att_mag_exqd_ar = false;
    bool att_mag_ext_smg = true;  bool att_mag_qd_smg = false;
    bool att_mag_exqd_smg = false;
    bool att_mag_ext_sr = false;  bool att_mag_qd_sr = false;
    bool att_mag_exqd_sr = false;
    bool att_stock_tac = false;   bool att_stock_cheek = false;
    bool att_stock_loop = false;  bool att_stock_heavy = false;

    float att_color[4] = { 0.9f, 0.7f, 0.2f, 1.0f };

    // Ammo
    bool ammo_556 = true;    bool ammo_762 = true;
    bool ammo_9mm = false;   bool ammo_45 = false;
    bool ammo_12g = false;   bool ammo_300 = false;
    bool ammo_bolt = false;

    float ammo_color[4] = { 0.7f, 0.5f, 0.3f, 1.0f };
};

static AimbotSettings  g_aimbot;
static ESPSettings     g_esp;
static ItemSettings    g_items;
static int             g_activeTab = 0;
static bool            g_requestExit = false;
static bool            g_cnFontOk = false;

#define CONFIG_FILE "ui_config.bin"

void SaveConfig()
{
    FILE* f = fopen(CONFIG_FILE, "wb");
    if (!f) return;
    fwrite(&g_aimbot, sizeof(g_aimbot), 1, f);
    fwrite(&g_esp,    sizeof(g_esp),    1, f);
    fwrite(&g_items,  sizeof(g_items),  1, f);
    fclose(f);
}

void LoadConfig()
{
    FILE* f = fopen(CONFIG_FILE, "rb");
    if (!f) return;
    fread(&g_aimbot, sizeof(g_aimbot), 1, f);
    fread(&g_esp,    sizeof(g_esp),    1, f);
    fread(&g_items,  sizeof(g_items),  1, f);
    fclose(f);
}

// ============================================================
// Style - polished dark theme
// ============================================================
void SetupStyle()
{
    ImGuiStyle& s = ImGui::GetStyle();
    s.Alpha = 1.0f;
    s.DisabledAlpha = 0.35f;
    s.WindowPadding = ImVec2(10, 10);
    s.WindowRounding = 6.0f;
    s.WindowBorderSize = 0.0f;
    s.ChildRounding = 6.0f;
    s.ChildBorderSize = 1.0f;
    s.FramePadding = ImVec2(8, 5);
    s.FrameRounding = 4.0f;
    s.FrameBorderSize = 1.0f;
    s.ItemSpacing = ImVec2(10, 6);
    s.ItemInnerSpacing = ImVec2(6, 4);
    s.IndentSpacing = 20.0f;
    s.ScrollbarSize = 10.0f;
    s.ScrollbarRounding = 5.0f;
    s.GrabMinSize = 10.0f;
    s.GrabRounding = 4.0f;
    s.TabRounding = 5.0f;
    s.TabBorderSize = 1.0f;
    s.ButtonTextAlign = ImVec2(0.5f, 0.5f);

    ImVec4* c = s.Colors;
    c[ImGuiCol_Text]                = ImVec4(0.92f, 0.93f, 0.94f, 1.00f);
    c[ImGuiCol_TextDisabled]        = ImVec4(0.38f, 0.40f, 0.42f, 1.00f);
    c[ImGuiCol_WindowBg]            = ImVec4(0.05f, 0.06f, 0.08f, 1.00f);
    c[ImGuiCol_ChildBg]             = ImVec4(0.07f, 0.08f, 0.10f, 1.00f);
    c[ImGuiCol_PopupBg]             = ImVec4(0.08f, 0.09f, 0.12f, 0.95f);
    c[ImGuiCol_Border]              = ImVec4(0.16f, 0.17f, 0.20f, 0.50f);
    c[ImGuiCol_BorderShadow]        = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_FrameBg]             = ImVec4(0.10f, 0.11f, 0.14f, 1.00f);
    c[ImGuiCol_FrameBgHovered]      = ImVec4(0.16f, 0.17f, 0.21f, 1.00f);
    c[ImGuiCol_FrameBgActive]       = ImVec4(0.18f, 0.19f, 0.24f, 1.00f);
    c[ImGuiCol_TitleBg]             = ImVec4(0.05f, 0.06f, 0.08f, 1.00f);
    c[ImGuiCol_TitleBgActive]       = ImVec4(0.07f, 0.08f, 0.11f, 1.00f);
    c[ImGuiCol_TitleBgCollapsed]    = ImVec4(0.05f, 0.06f, 0.08f, 1.00f);
    c[ImGuiCol_MenuBarBg]           = ImVec4(0.08f, 0.09f, 0.12f, 1.00f);
    c[ImGuiCol_ScrollbarBg]         = ImVec4(0.04f, 0.05f, 0.07f, 1.00f);
    c[ImGuiCol_ScrollbarGrab]       = ImVec4(0.18f, 0.19f, 0.24f, 0.50f);
    c[ImGuiCol_ScrollbarGrabHovered]= ImVec4(0.28f, 0.29f, 0.36f, 0.70f);
    c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.38f, 0.39f, 0.48f, 1.00f);
    c[ImGuiCol_CheckMark]           = ImVec4(0.00f, 0.90f, 0.40f, 1.00f);
    c[ImGuiCol_SliderGrab]          = ImVec4(0.00f, 0.70f, 0.30f, 1.00f);
    c[ImGuiCol_SliderGrabActive]    = ImVec4(0.00f, 0.90f, 0.40f, 1.00f);
    c[ImGuiCol_Button]              = ImVec4(0.12f, 0.13f, 0.17f, 1.00f);
    c[ImGuiCol_ButtonHovered]       = ImVec4(0.18f, 0.20f, 0.26f, 1.00f);
    c[ImGuiCol_ButtonActive]        = ImVec4(0.09f, 0.10f, 0.14f, 1.00f);
    c[ImGuiCol_Header]              = ImVec4(0.12f, 0.13f, 0.17f, 1.00f);
    c[ImGuiCol_HeaderHovered]       = ImVec4(0.18f, 0.20f, 0.26f, 1.00f);
    c[ImGuiCol_HeaderActive]        = ImVec4(0.06f, 0.50f, 0.20f, 0.60f);
    c[ImGuiCol_Separator]           = ImVec4(0.16f, 0.17f, 0.21f, 0.50f);
    c[ImGuiCol_SeparatorHovered]    = ImVec4(0.30f, 0.31f, 0.38f, 0.60f);
    c[ImGuiCol_SeparatorActive]     = ImVec4(0.00f, 0.80f, 0.35f, 0.60f);
    c[ImGuiCol_ResizeGrip]          = ImVec4(0.16f, 0.17f, 0.22f, 1.00f);
    c[ImGuiCol_ResizeGripHovered]   = ImVec4(0.22f, 0.24f, 0.30f, 1.00f);
    c[ImGuiCol_ResizeGripActive]    = ImVec4(0.00f, 0.80f, 0.35f, 1.00f);
    c[ImGuiCol_Tab]                 = ImVec4(0.09f, 0.10f, 0.14f, 1.00f);
    c[ImGuiCol_TabHovered]          = ImVec4(0.16f, 0.18f, 0.24f, 1.00f);
    c[ImGuiCol_TabActive]           = ImVec4(0.00f, 0.55f, 0.22f, 1.00f);
    c[ImGuiCol_TabUnfocused]        = ImVec4(0.07f, 0.08f, 0.11f, 1.00f);
    c[ImGuiCol_TabUnfocusedActive]  = ImVec4(0.00f, 0.38f, 0.15f, 1.00f);
    c[ImGuiCol_TextSelectedBg]      = ImVec4(0.00f, 0.65f, 0.28f, 0.35f);
    c[ImGuiCol_DragDropTarget]      = ImVec4(0.00f, 0.80f, 0.35f, 0.90f);
    c[ImGuiCol_NavHighlight]        = ImVec4(0.00f, 0.80f, 0.35f, 0.50f);
    c[ImGuiCol_NavWindowingHighlight]=ImVec4(0.00f, 0.80f, 0.35f, 0.50f);
    c[ImGuiCol_NavWindowingDimBg]   = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
    c[ImGuiCol_ModalWindowDimBg]    = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
}

// ============================================================
// Helpers
// ============================================================
void Section(const char* label)
{
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.00f, 0.90f, 0.40f, 1.00f));
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();
}

bool CatHeader(const char* label, bool enabled, const float* clr)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(clr[0], clr[1], clr[2], 1.0f));
    bool open = ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen);
    ImGui::PopStyleColor();
    return open && enabled;
}

void Ck2(const char* a, bool* va, const char* b, bool* vb, float x2 = 180.0f)
{
    ImGui::Checkbox(a, va);
    ImGui::SameLine(x2);
    ImGui::Checkbox(b, vb);
}

// ============================================================
// Tab: Aimbot
// ============================================================
void DrawAimbotTab()
{
    Section("自瞄设置");

    ImGui::Checkbox("启用自瞄", &g_aimbot.enabled);
    ImGui::SameLine();
    ImGui::TextColored(g_aimbot.enabled ? ImVec4(0, 1, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1),
        g_aimbot.enabled ? "已激活" : "已关闭");

    ImGui::Spacing();
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.60f);
    ImGui::SliderFloat("FOV 半径", &g_aimbot.fov, 20.0f, 500.0f, "%.0f px");
    ImGui::SliderFloat("平滑度", &g_aimbot.smooth, 1.0f, 20.0f, "%.1f");
    ImGui::PopItemWidth();

    const char* bones[] = { "头部", "颈部", "胸部", "骨盆" };
    ImGui::Combo("目标骨骼", &g_aimbot.bone, bones, 4);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.60f);
    ImGui::SliderFloat("最大距离", &g_aimbot.max_distance, 10.0f, 500.0f, "%.0f m");
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Checkbox("可见性检查", &g_aimbot.visible_check);
    ImGui::Checkbox("绘制 FOV 圆圈", &g_aimbot.draw_fov);
    if (g_aimbot.draw_fov)
        ImGui::ColorEdit4("FOV 颜色", g_aimbot.fov_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
}

// ============================================================
// Tab: ESP
// ============================================================
void DrawESPTab()
{
    Section("玩家透视");

    Ck2("骨骼",  &g_esp.skeleton, "2D 方框",    &g_esp.box);
    Ck2("天线",  &g_esp.snapline, "手持武器",    &g_esp.weapon);
    Ck2("名字",  &g_esp.name,     "血量",        &g_esp.health);
    ImGui::Checkbox("团队编号", &g_esp.team);

    ImGui::Spacing();
    Section("透视选项");

    const char* box_styles[] = { "2D 方框", "四角方框", "填充方框" };
    ImGui::Combo("方框样式", &g_esp.box_style, box_styles, 3);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.60f);
    ImGui::SliderFloat("最大透视距离", &g_esp.max_distance, 50.0f, 1000.0f, "%.0f m");
    ImGui::PopItemWidth();

    ImGui::Spacing();
    Section("颜色设置");

    float colW = ImGui::GetContentRegionAvail().x * 0.48f;
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.07f, 0.09f, 1.0f));

    ImGui::BeginChild("ECL", ImVec2(colW, 0), ImGuiChildFlags_Borders);
    if (g_esp.skeleton)    ImGui::ColorEdit4("骨骼",        g_esp.skeleton_color,    ImGuiColorEditFlags_NoInputs);
    if (g_esp.box)         { ImGui::ColorEdit4("方框-可见", g_esp.box_visible,       ImGuiColorEditFlags_NoInputs);
                             ImGui::ColorEdit4("方框-掩体", g_esp.box_invisible,     ImGuiColorEditFlags_NoInputs); }
    if (g_esp.snapline)    ImGui::ColorEdit4("天线",        g_esp.snapline_color,    ImGuiColorEditFlags_NoInputs);
    if (g_esp.weapon)      ImGui::ColorEdit4("武器文字",    g_esp.weapon_color,      ImGuiColorEditFlags_NoInputs);
    ImGui::EndChild();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::SameLine();
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.07f, 0.09f, 1.0f));

    ImGui::BeginChild("ECR", ImVec2(colW, 0), ImGuiChildFlags_Borders);
    if (g_esp.name)        ImGui::ColorEdit4("名字文字",    g_esp.name_color,        ImGuiColorEditFlags_NoInputs);
    if (g_esp.health)      { ImGui::ColorEdit4("血量-满",   g_esp.health_color_full, ImGuiColorEditFlags_NoInputs);
                             ImGui::ColorEdit4("血量-低",   g_esp.health_color_low,  ImGuiColorEditFlags_NoInputs); }
    if (g_esp.team)        ImGui::ColorEdit4("团队文字",    g_esp.team_color,        ImGuiColorEditFlags_NoInputs);
    ImGui::EndChild();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    if (!g_esp.skeleton && !g_esp.box && !g_esp.snapline && !g_esp.weapon && !g_esp.name && !g_esp.health && !g_esp.team)
        ImGui::TextDisabled("请先开启上方透视功能，颜色选项才会显示");
}

// ============================================================
// Tab: Items
// ============================================================
void DrawItemsTab()
{
    float x2 = 190.0f;

    Section("物品分类");
    Ck2("武器",         &g_items.show_weapons,     "载具",         &g_items.show_vehicles,    x2);
    Ck2("医疗物品",     &g_items.show_medical,     "投掷物",       &g_items.show_throwables,  x2);
    Ck2("防具",         &g_items.show_armor,       "配件",         &g_items.show_attachments, x2);
    ImGui::Checkbox("弹药", &g_items.show_ammo);

    ImGui::Spacing();
    ImGui::BeginChild("ItemScroll", ImVec2(0, 0), ImGuiChildFlags_Borders, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    // ---- Weapons ----
    if (CatHeader("武器", g_items.show_weapons, g_items.weapon_color))
    {
        ImGui::ColorEdit4("颜色", g_items.weapon_color, ImGuiColorEditFlags_NoInputs);
        ImGui::Spacing();
        if (ImGui::CollapsingHeader("突击步枪 (AR)", ImGuiTreeNodeFlags_DefaultOpen))
        {
            Ck2("M416",  &g_items.wpn_m416,  "AKM",      &g_items.wpn_akm,      x2);
            Ck2("SCAR-L",&g_items.wpn_scarl,  "M16A4",    &g_items.wpn_m16a4,    x2);
            Ck2("Beryl", &g_items.wpn_beryl,  "AUG",      &g_items.wpn_aug,      x2);
            Ck2("Groza", &g_items.wpn_groza,  "QBZ",      &g_items.wpn_qbz,      x2);
            Ck2("G36C",  &g_items.wpn_g36c,   "ACE32",    &g_items.wpn_ace32,    x2);
            Ck2("MK47",  &g_items.wpn_mk47,   "K2",       &g_items.wpn_k2,       x2);
        }
        if (ImGui::CollapsingHeader("精确射手步枪 (DMR)"))
        {
            Ck2("Mini-14", &g_items.wpn_mini14,  "SKS",     &g_items.wpn_sks,     x2);
            Ck2("SLR",     &g_items.wpn_slr,     "VSS",     &g_items.wpn_vss,     x2);
            Ck2("Mk12",    &g_items.wpn_mk12,    "QBU",     &g_items.wpn_qbu,     x2);
            ImGui::Checkbox("Dragunov", &g_items.wpn_dragunov);
        }
        if (ImGui::CollapsingHeader("狙击步枪 (SR)"))
        {
            Ck2("Kar98k", &g_items.wpn_kar98,  "M24",     &g_items.wpn_m24,     x2);
            Ck2("AWM",    &g_items.wpn_awm,    "Win94",   &g_items.wpn_win94,   x2);
            ImGui::Checkbox("Lynx AMR", &g_items.wpn_lynx);
        }
        if (ImGui::CollapsingHeader("冲锋枪 (SMG)"))
        {
            Ck2("UMP45",  &g_items.wpn_ump45,  "Vector",    &g_items.wpn_vector,  x2);
            Ck2("微型 UZI",&g_items.wpn_uzi,   "Tommy Gun", &g_items.wpn_tommy,   x2);
            Ck2("MP5K",   &g_items.wpn_mp5k,   "PP-Bizon",  &g_items.wpn_bizon,   x2);
            Ck2("P90",    &g_items.wpn_p90,    "JS9",       &g_items.wpn_js9,     x2);
        }
        if (ImGui::CollapsingHeader("霰弹枪 (SG)"))
        {
            Ck2("S686",  &g_items.wpn_s686,   "S1897",  &g_items.wpn_s1897,   x2);
            Ck2("S12K",  &g_items.wpn_s12k,   "DBS",    &g_items.wpn_dbs,     x2);
            ImGui::Checkbox("O12", &g_items.wpn_o12);
        }
        if (ImGui::CollapsingHeader("轻机枪 (LMG)"))
        {
            Ck2("DP-28", &g_items.wpn_dp28, "M249", &g_items.wpn_m249, x2);
            ImGui::Checkbox("MG3", &g_items.wpn_mg3);
        }
        if (ImGui::CollapsingHeader("手枪"))
        {
            Ck2("P18C",    &g_items.wpn_p18c,   "P92",     &g_items.wpn_p92,     x2);
            Ck2("P1911",   &g_items.wpn_p1911,  "Deagle",  &g_items.wpn_deagle,  x2);
            Ck2("Skorpion",&g_items.wpn_skorpion,"R1895",  &g_items.wpn_r1895,   x2);
        }
        if (ImGui::CollapsingHeader("近战武器"))
        {
            Ck2("平底锅", &g_items.wpn_pan,     "砍刀",   &g_items.wpn_machete, x2);
            Ck2("撬棍",   &g_items.wpn_crowbar, "镰刀",   &g_items.wpn_sickle,  x2);
        }
    }

    // ---- Vehicles ----
    if (CatHeader("载具", g_items.show_vehicles, g_items.vehicle_color))
    {
        ImGui::ColorEdit4("颜色", g_items.vehicle_color, ImGuiColorEditFlags_NoInputs);
        ImGui::Spacing();
        if (ImGui::CollapsingHeader("汽车", ImGuiTreeNodeFlags_DefaultOpen))
        {
            Ck2("达西亚 轿车", &g_items.veh_dacia,  "拉力赛车",       &g_items.veh_rally,  x2);
            Ck2("UAZ 吉普车",  &g_items.veh_uaz,    "Mirado 肌肉车",  &g_items.veh_mirado, x2);
            Ck2("Coupe RB",   &g_items.veh_coupe,  "Zima",            &g_items.veh_zima,   x2);
            Ck2("Blanc",      &g_items.veh_blanc,  "皮卡",            &g_items.veh_pickup, x2);
            Ck2("面包车",     &g_items.veh_van,    "蹦蹦车",          &g_items.veh_buggy,  x2);
            Ck2("Porter",     &g_items.veh_porter, "食品卡车",        &g_items.veh_foodtruck, x2);
            ImGui::Checkbox("Pillar 轿车", &g_items.veh_pillar);
        }
        if (ImGui::CollapsingHeader("摩托车"))
        {
            Ck2("摩托车",     &g_items.veh_motorcycle, "小绵羊",       &g_items.veh_scooter,       x2);
            Ck2("沙滩车",     &g_items.veh_atv,        "山地自行车",    &g_items.veh_bike, x2);
        }
        if (ImGui::CollapsingHeader("水上 / 空中"))
        {
            Ck2("快艇",       &g_items.veh_boat,     "摩托艇",       &g_items.veh_aquarail, x2);
            Ck2("装甲车 BRDM",&g_items.veh_brdm,     "滑翔机",       &g_items.veh_glider,   x2);
        }
    }

    // ---- Medical ----
    if (CatHeader("医疗物品", g_items.show_medical, g_items.medic_color))
    {
        ImGui::ColorEdit4("颜色", g_items.medic_color, ImGuiColorEditFlags_NoInputs);
        ImGui::Spacing();
        Ck2("绷带",         &g_items.medic_bandage,    "急救包",        &g_items.medic_firstaid, x2);
        Ck2("医疗箱",       &g_items.medic_medkit,     "止痛药",        &g_items.medic_painkiller,x2);
        Ck2("能量饮料",     &g_items.medic_energy,     "肾上腺素",      &g_items.medic_adrenaline,x2);
        ImGui::Checkbox("紧急呼救器", &g_items.medic_rescue);
    }

    // ---- Throwables ----
    if (CatHeader("投掷物", g_items.show_throwables, g_items.throw_color))
    {
        ImGui::ColorEdit4("颜色", g_items.throw_color, ImGuiColorEditFlags_NoInputs);
        ImGui::Spacing();
        Ck2("破片手雷", &g_items.throw_frag,   "燃烧瓶",     &g_items.throw_molotov,  x2);
        Ck2("烟雾弹",   &g_items.throw_smoke,  "闪光弹",     &g_items.throw_stun,     x2);
        Ck2("蓝圈手雷", &g_items.throw_bz,     "C4 炸弹",    &g_items.throw_c4,       x2);
        ImGui::Checkbox("粘性炸弹", &g_items.throw_sticky);
    }

    // ---- Armor ----
    if (CatHeader("防具", g_items.show_armor, g_items.armor_color))
    {
        ImGui::ColorEdit4("颜色", g_items.armor_color, ImGuiColorEditFlags_NoInputs);
        ImGui::Spacing();
        if (ImGui::CollapsingHeader("头盔", ImGuiTreeNodeFlags_DefaultOpen))
        {
            Ck2("一级头", &g_items.armor_helmet1, "二级头", &g_items.armor_helmet2, x2);
            ImGui::Checkbox("三级头 (特种)", &g_items.armor_helmet3);
        }
        if (ImGui::CollapsingHeader("防弹衣", ImGuiTreeNodeFlags_DefaultOpen))
        {
            Ck2("一级甲", &g_items.armor_vest1, "二级甲", &g_items.armor_vest2, x2);
            ImGui::Checkbox("三级甲 (特种)", &g_items.armor_vest3);
        }
        if (ImGui::CollapsingHeader("背包"))
        {
            Ck2("一级包", &g_items.armor_bp1, "二级包", &g_items.armor_bp2, x2);
            ImGui::Checkbox("三级包", &g_items.armor_bp3);
        }
        ImGui::Checkbox("吉利服", &g_items.armor_ghillie);
    }

    // ---- Attachments ----
    if (CatHeader("配件", g_items.show_attachments, g_items.att_color))
    {
        ImGui::ColorEdit4("颜色", g_items.att_color, ImGuiColorEditFlags_NoInputs);
        ImGui::Spacing();
        if (ImGui::CollapsingHeader("瞄具", ImGuiTreeNodeFlags_DefaultOpen))
        {
            Ck2("红点",     &g_items.att_reddot,  "全息",       &g_items.att_holo,    x2);
            Ck2("2倍镜",    &g_items.att_2x,      "3倍镜",      &g_items.att_3x,      x2);
            Ck2("4倍镜",    &g_items.att_4x,      "6倍镜",      &g_items.att_6x,      x2);
            Ck2("8倍镜",    &g_items.att_8x,      "15倍镜",     &g_items.att_15x,     x2);
            ImGui::Checkbox("侧面瞄具", &g_items.att_canted);
        }
        if (ImGui::CollapsingHeader("枪口"))
        {
            Ck2("消音 (AR)",   &g_items.att_supp_ar,  "消音 (SR)",   &g_items.att_supp_sr,  x2);
            Ck2("消音 (SMG)",  &g_items.att_supp_smg, "消音 (手枪)",  &g_items.att_supp_pis, x2);
            Ck2("补偿 (AR)",   &g_items.att_comp_ar,  "补偿 (SMG)",  &g_items.att_comp_smg, x2);
            ImGui::Checkbox("补偿 (SR)", &g_items.att_comp_sr);
            Ck2("消焰 (AR)",   &g_items.att_fh_ar,    "消焰 (SMG)",  &g_items.att_fh_smg,   x2);
            ImGui::Checkbox("消焰 (SR)", &g_items.att_fh_sr);
        }
        if (ImGui::CollapsingHeader("握把"))
        {
            Ck2("垂直握把",   &g_items.att_grip_vert,  "直角握把",     &g_items.att_grip_ang,  x2);
            Ck2("半截式握把", &g_items.att_grip_half,  "拇指握把",     &g_items.att_grip_thumb, x2);
            ImGui::Checkbox("轻型握把", &g_items.att_grip_light);
        }
        if (ImGui::CollapsingHeader("弹匣"))
        {
            ImGui::TextDisabled("AR / DMR");
            Ck2("扩容 (AR)",     &g_items.att_mag_ext_ar,    "快扩 (AR)",     &g_items.att_mag_qd_ar,    x2);
            ImGui::Checkbox("快拔扩容 (AR)", &g_items.att_mag_exqd_ar);
            ImGui::TextDisabled("SMG");
            Ck2("扩容 (SMG)",    &g_items.att_mag_ext_smg,   "快扩 (SMG)",    &g_items.att_mag_qd_smg,   x2);
            ImGui::Checkbox("快拔扩容 (SMG)", &g_items.att_mag_exqd_smg);
            ImGui::TextDisabled("SR");
            Ck2("扩容 (SR)",     &g_items.att_mag_ext_sr,    "快扩 (SR)",     &g_items.att_mag_qd_sr,    x2);
            ImGui::Checkbox("快拔扩容 (SR)", &g_items.att_mag_exqd_sr);
        }
        if (ImGui::CollapsingHeader("枪托"))
        {
            Ck2("战术枪托 (AR)",&g_items.att_stock_tac,    "托腮板 (SR)",   &g_items.att_stock_cheek, x2);
            Ck2("子弹带 (SG)",  &g_items.att_stock_loop,   "重型枪托",      &g_items.att_stock_heavy, x2);
        }
    }

    // ---- Ammo ----
    if (CatHeader("弹药", g_items.show_ammo, g_items.ammo_color))
    {
        ImGui::ColorEdit4("颜色", g_items.ammo_color, ImGuiColorEditFlags_NoInputs);
        ImGui::Spacing();
        Ck2("5.56mm", &g_items.ammo_556, "7.62mm", &g_items.ammo_762, x2);
        Ck2("9mm",    &g_items.ammo_9mm, ".45 ACP",&g_items.ammo_45,  x2);
        Ck2("12号霰弹",&g_items.ammo_12g,".300 马格南", &g_items.ammo_300, x2);
        ImGui::Checkbox("弩箭", &g_items.ammo_bolt);
    }

    ImGui::EndChild();
}

// ============================================================
// Main
// ============================================================
int main(int, char**)
{
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    g_winW = (int)(screenW * 0.30f);
    g_winH = (int)(screenH * 0.55f);
    if (g_winW < 620) g_winW = 620;
    if (g_winH < 460) g_winH = 460;
    int winX = (screenW - g_winW) / 2;
    int winY = (screenH - g_winH) / 2;

    // Borderless topmost window — WS_POPUP ensures opaque, no click-through
    WNDCLASSEXW wc = {
        sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L,
        GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
        L"ImGui DX11 UI", nullptr
    };
    ::RegisterClassExW(&wc);
    g_hwnd = ::CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_APPWINDOW,
        wc.lpszClassName, L"ImGui DX11 UI",
        WS_POPUP | WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX,
        winX, winY, g_winW, g_winH,
        nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(g_hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(g_hwnd, SW_SHOW);
    ::UpdateWindow(g_hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    SetupStyle();

    // Chinese font
    {
        const ImWchar* ranges = io.Fonts->GetGlyphRangesChineseSimplifiedCommon();
        ImFont* font = io.Fonts->AddFontFromFileTTF(
            "C:\\Windows\\Fonts\\msyh.ttc", 14.0f, nullptr, ranges);
        if (font) { io.FontDefault = font; g_cnFontOk = true; }
        else
        {
            font = io.Fonts->AddFontFromFileTTF(
                "C:\\Windows\\Fonts\\simhei.ttf", 14.0f, nullptr, ranges);
            if (font) { io.FontDefault = font; g_cnFontOk = true; }
            else
                io.FontDefault = io.Fonts->AddFontDefault();
        }
    }

    LoadConfig();

    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ImVec4 clear_color = ImVec4(0.03f, 0.04f, 0.06f, 1.00f);

    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        {
            ImGui::SetNextWindowSize(ImVec2((float)g_winW, (float)g_winH), ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
            ImGui::Begin("##Main", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoBringToFrontOnFocus);

            // ========== Title bar with drag ==========
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.04f, 0.05f, 0.07f, 1.0f));
            ImGui::BeginChild("##TitleBar", ImVec2(0, 34), ImGuiChildFlags_Borders);
            {
                float titleH = 28.0f;

                // Brand
                ImGui::SetCursorPos(ImVec2(16, 6));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.00f, 1.00f, 0.45f, 1.0f));
                ImGui::Text("IMGUI DX11 UI");
                ImGui::PopStyleColor();

                // Exit button (right aligned)
                ImGui::SameLine(ImGui::GetWindowWidth() - 90);
                ImGui::SetCursorPosY(4);
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.50f, 0.06f, 0.06f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.78f, 0.10f, 0.10f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.35f, 0.03f, 0.03f, 1.0f));
                if (ImGui::Button("  退 出  ", ImVec2(75, titleH)))
                    g_requestExit = true;
                ImGui::PopStyleColor(3);
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();

            ImGui::Spacing();

            // ========== Tab bar ==========
            const char* tabs[] = { "  自 瞄  ", "  透 视  ", "  物品显示  " };
            float tabW = (ImGui::GetWindowWidth() - 55) / 3;
            ImGui::SetCursorPosX(12);
            for (int i = 0; i < 3; i++)
            {
                if (i > 0) ImGui::SameLine();
                bool is_active = (g_activeTab == i);
                if (is_active)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.55f, 0.22f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.00f, 0.62f, 0.25f, 1.0f));
                }
                if (ImGui::Button(tabs[i], ImVec2(tabW, 34)))
                    g_activeTab = i;
                if (is_active)
                    ImGui::PopStyleColor(2);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // ========== Tab content ==========
            ImGui::BeginChild("##TabContent", ImVec2(0, 0), ImGuiChildFlags_Borders);
            switch (g_activeTab)
            {
            case 0: DrawAimbotTab(); break;
            case 1: DrawESPTab();    break;
            case 2: DrawItemsTab();  break;
            }
            ImGui::EndChild();

            // ========== Status bar ==========
            ImGui::Separator();
            ImGui::Text("FPS: %.0f  |  %dx%d", io.Framerate, screenW, screenH);
            ImGui::SameLine();
            ImGui::TextColored(g_cnFontOk ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0.3f, 0.3f, 1),
                g_cnFontOk ? "CN Font: OK" : "CN Font: FAIL");
            ImGui::SameLine(ImGui::GetWindowWidth() - 55);
            ImGui::TextDisabled("v4.0");

            ImGui::End();
            ImGui::PopStyleVar();
        }

        if (g_requestExit)
        {
            SaveConfig();
            done = true;
        }

        ImGui::Render();
        const float c[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w,
                             clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, c);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
    }

    SaveConfig();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(g_hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// ============================================================
// DX11 Helpers
// ============================================================

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
        &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
            featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
            &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK) return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}
