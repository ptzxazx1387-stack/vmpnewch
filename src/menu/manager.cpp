#include <Windows.h>
#include "../vendor/imgui/imgui.h"
#include "../proxy.h"

namespace Menu {

static struct {
    bool esp_on = true;
    bool esp_box = true;
    bool esp_snapline = true;
    bool esp_health = true;
    bool esp_distance = true;
    bool esp_skeleton = false;
    float esp_max_dist = 500.0f;

    bool aim_on = true;
    int aim_bone = 0;
    float aim_fov = 3.0f;
    float aim_smooth = 4.0f;
    int aim_key = VK_LBUTTON;

    bool vis_no_fog = false;
    bool vis_night = false;
    float gamma = 1.0f;

    bool misc_radar = false;
    bool misc_crosshair = false;

    inline static const char* bones[] = { "Head", "Neck", "Chest", "Pelvis", "Left Foot", "Right Foot" };
} g_cfg;

void Render() {
    if (!Cheat::g_MenuOpen) return;

    ImGui::SetNextWindowSize(ImVec2(520, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("VMP Cheat - ESP & Aimbot", &Cheat::g_MenuOpen,
        ImGuiWindowFlags_NoCollapse);

    if (ImGui::BeginTabBar("MainTabs")) {

        // ===== ESP Tab =====
        if (ImGui::BeginTabItem("ESP")) {
            ImGui::Checkbox("ESP Enabled", &g_cfg.esp_on);
            ImGui::Separator();
            ImGui::Checkbox("Box ESP", &g_cfg.esp_box);
            ImGui::Checkbox("Snaplines", &g_cfg.esp_snapline);
            ImGui::Checkbox("Health Bars", &g_cfg.esp_health);
            ImGui::Checkbox("Distance", &g_cfg.esp_distance);
            ImGui::Checkbox("Skeleton ESP", &g_cfg.esp_skeleton);
            ImGui::SliderFloat("Max Distance", &g_cfg.esp_max_dist, 10.f, 2000.f, "%.0fm");
            ImGui::EndTabItem();
        }

        // ===== Aimbot Tab =====
        if (ImGui::BeginTabItem("Aimbot")) {
            ImGui::Checkbox("Enable Aimbot", &g_cfg.aim_on);
            ImGui::Separator();
            ImGui::Combo("Target Bone", &g_cfg.aim_bone, g_cfg.bones,
                IM_ARRAYSIZE(g_cfg.bones));
            ImGui::SliderFloat("Aim FOV", &g_cfg.aim_fov, 0.5f, 30.0f, "%.1f deg");
            ImGui::SliderFloat("Smooth Factor", &g_cfg.aim_smooth, 0.5f, 20.0f, "%.1f");
            ImGui::EndTabItem();
        }

        // ===== Visuals Tab =====
        if (ImGui::BeginTabItem("Visuals")) {
            ImGui::Checkbox("No Fog", &g_cfg.vis_no_fog);
            ImGui::Checkbox("Night Mode", &g_cfg.vis_night);
            ImGui::SliderFloat("Gamma", &g_cfg.gamma, 0.5f, 3.0f);
            ImGui::EndTabItem();
        }

        // ===== Misc Tab =====
        if (ImGui::BeginTabItem("Misc")) {
            ImGui::Checkbox("2D Radar", &g_cfg.misc_radar);
            ImGui::Checkbox("Crosshair", &g_cfg.misc_crosshair);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

// Getter for config values used by ESP/Aimbot modules
bool esp_on()    { return g_cfg.esp_on; }
bool esp_box()   { return g_cfg.esp_box; }
bool esp_line()   { return g_cfg.esp_snapline; }
bool esp_hp()    { return g_cfg.esp_health; }
bool esp_dist()   { return g_cfg.esp_distance; }
bool esp_skel()   { return g_cfg.esp_skeleton; }
float esp_maxd()   { return g_cfg.esp_max_dist; }

bool aim_on()    { return g_cfg.aim_on; }
int aim_bone()    { return g_cfg.aim_bone; }
float aim_fov()   { return g_cfg.aim_fov; }
float aim_smooth() { return g_cfg.aim_smooth; }

} // namespace Menu

// Global wrapper for dx11_hook.cpp extern call
void Menu_Render() { Menu::Render(); }