#include <Windows.h>
#include <cstdio>
#include "../vendor/imgui/imgui.h"
#include "../game/entity.h"
#include "../game/context.h"
#include "w2s.h"

namespace ESP {

struct Config { bool on=true, box=true, line=true, hp=true, dist=true, skel=false; float maxd=500.f; } cfg;

void Run() {
    if (!cfg.on) return;
    auto* loc = GameContext::GetLocalPlayer();
    if (!loc || loc->GetPos().x == 0) return;

    for (auto& e : GameContext::GetAllEntities()) {
        if (e.addr == loc->addr) continue;
        if (e.GetHealth() <= 0) continue;

        float d = loc->GetPos().dist_to(e.GetPos());
        if (d > cfg.maxd) continue;

        Vec2 s;
        if (!WorldToScreen(e.GetPos(), s)) continue;

        ImColor clr(255, 0, 0, 255);
        float h = 1000.f / d * 12.f;
        float w = h * 0.4f;

        // Box ESP
        if (cfg.box) {
            ImGui::GetBackgroundDrawList()->AddRect(
                ImVec2(s.x - w/2, s.y - h/2),
                ImVec2(s.x + w/2, s.y + h/2), clr, 0.f, 0, 1.5f);
        }

        // Snapline
        if (cfg.line) {
            ImGui::GetBackgroundDrawList()->AddLine(
                ImVec2((float)g::ScreenW/2, (float)g::ScreenH),
                ImVec2(s.x, s.y), clr, 1.f);
        }

        // Health
        if (cfg.hp) {
            float hp = e.GetHealth() / 100.f;
            ImColor hc = (hp > 0.5f) ? ImColor(0,255,0) : ImColor(255,165,0);
            if (hp < 0.25f) hc = ImColor(255,0,0);
            float bx = s.x - w/2 - 6.f;
            ImGui::GetBackgroundDrawList()->AddRectFilled(
                ImVec2(bx, s.y + h/2 - h * hp), ImVec2(bx + 3, s.y + h/2), hc);
        }

        // Distance
        if (cfg.dist) {
            char t[16]; sprintf(t, "%.0fm", d);
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(s.x, s.y + h/2), clr, t);
        }

        // Skeleton (stub — bone drawing not yet implemented)
        if (cfg.skel) {
            // TODO: implement DrawBones for skeleton ESP
        }
    }
}

} // namespace ESP

// Global wrapper for dx11_hook.cpp extern call
void ESP_Run() { ESP::Run(); }