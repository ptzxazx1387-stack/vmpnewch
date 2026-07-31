#include <Windows.h>
#include <cmath>
#include "../utils/vector.h"
#include "../game/entity.h"
#include "../game/context.h"
#include "../esp/w2s.h"

namespace Aimbot {

struct { bool on=true; int bone=0x796E; float fov=3.f, smooth=4.f; int key=VK_LBUTTON; } cfg;

Vec3 CalcAngle(const Vec3& from, const Vec3& to) {
    Vec3 d = {to.x-from.x, to.y-from.y, to.z-from.z};
    float dist = sqrtf(d.x*d.x + d.y*d.y);
    return {-atan2f(d.z,dist)*57.3f, atan2f(d.y,d.x)*57.3f, 0};
}

void Run() {
    if (!cfg.on) return;
    if (!(GetAsyncKeyState(cfg.key) & 0x8000)) return;

    auto* loc = GameContext::GetLocalPlayer();
    if (!loc) return;
    Vec3 myPos = loc->GetPos();

    float best = cfg.fov;
    uint64_t target = 0;
    Vec3 tpos;

    for (auto& e : GameContext::GetAllEntities()) {
        if (e.addr == loc->addr || e.GetHealth() <= 0) continue;
        CPed p(e.addr);
        Vec3 b = p.GetBonePos(cfg.bone);
        Vec3 a = CalcAngle(myPos, b);
        if (fabsf(a.x) < best && fabsf(a.y) < best) {
            best = fmaxf(fabsf(a.x), fabsf(a.y));
            target = e.addr; tpos = b;
        }
    }
    if (!target) return;

    Vec3 ta = CalcAngle(myPos, tpos);
    if (g::CameraAddr) {
        *(float*)(g::CameraAddr + 0x48) = ta.x;  // pitch
        *(float*)(g::CameraAddr + 0x2E4) = ta.y; // yaw
    }
}
} // namespace Aimbot

// Global wrapper for dx11_hook.cpp extern call
void Aimbot_Run() { Aimbot::Run(); }