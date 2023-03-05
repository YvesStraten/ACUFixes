#include "pch.h"

#include "AutoAssemblerKinda.h"
#include "Hack_EnterWindowsWhenRisPressed.h"
#include "Hack_SlowMenacingWalkAndAutowalk.h"
#include "Hack_CycleEquipmentWhenScrollingMousewheel.h"

// Async-constructing the AutoAssemblerWrapper<CodeHolderObject> is better, because all the VirtualAllocs
// produce a noticeable stutter on creation otherwise.
template<typename Ty>
class AsyncConstructed
{
public:
    Ty* m_CachedPtr = nullptr;
    std::optional<Ty> m_Instance;
    std::future<void> m_Future;
    std::optional<std::exception> m_Exception;
public:
    AsyncConstructed()
    {
        m_Future = std::async(std::launch::async, [&]() {
            try
            {
                m_Instance.emplace();
                m_CachedPtr = &m_Instance.value();
            }
            catch (const std::exception& e)
            {
                m_Exception = e;
            }
            });
    }
    Ty* get() { return m_CachedPtr; }
    std::exception* GetException()
    {
        return m_Exception ? &m_Exception.value() : nullptr;
    }
};

template<typename floatlike>
floatlike simple_interp(floatlike mn, floatlike mx)
{
    auto now = GetTickCount64();
    float speed = 0.001f * 1.5f;
    float interp = sin(now * speed);
    interp = (interp + 1) / 2;
    return mn + (mx - mn) * interp;
}
void FOVGames(AllRegisters* params)
{
    params->XMM1.f0 = simple_interp(0.5f, 1.0f);
}
struct PlayWithFOV : AutoAssemblerCodeHolder_Base
{
    PlayWithFOV()
    {
        PresetScript_CCodeInTheMiddle(
            0x141F3FE3B, 6
            , FOVGames
            , RETURN_TO_RIGHT_AFTER_STOLEN_BYTES
            , true);
    }
};

#include "ACU/ACUGetSingletons.h"
#include "ACU/Entity.h"
#include "ImGui3D.h"
#include "ACU/ThrowTargetPrecision.h"
void OverrideThrowPredictorBeamPosition(AllRegisters* params)
{
    // At this injection point (0x14055B4BB) RBX == ThrowTargetPrecision* and takes two values:
    // two systems that receive messages about the predictor beam's results.
    // One regulates camera rotation around Z, the other one - rotation around camera's left-right axis.
    Entity* player = ACU::GetPlayer();
    static Vector4f farthestResult;
    ThrowTargetPrecision* thr = (ThrowTargetPrecision*)params->rbx_;
    ImGui3D::DrawLocationNamed((Vector3f&)thr->predictionBeamOrigin, "Prediction beam origin");
    ImGui3D::DrawLocationNamed((Vector3f&)thr->trackerCrawlsTowardPredictorBeamEnd, "Prediction beam terminus");
    thr->trackerCrawlsTowardPredictorBeamEnd.z = player->GetPosition().z + 1;
    //(Vector3f&)thr->cameraTrackerMovingTowardPredictionBeamEnd = player->GetPosition() + Vector3f{ 0, 3, 1 };
}
struct PlayWithBombAimCameraTracker2 : AutoAssemblerCodeHolder_Base
{
    PlayWithBombAimCameraTracker2()
    {
        PresetScript_CCodeInTheMiddle(
            0x14055B4BB, 8
            , OverrideThrowPredictorBeamPosition
            , RETURN_TO_RIGHT_AFTER_STOLEN_BYTES
            , true);
    }
};


extern bool g_showDevExtraOptions;
#include "Hack_ModifyAimingFOV.h"
#include "MyLog.h"
class MyHacks
{
public:
    AutoAssembleWrapper<EnterWindowWhenRisPressed> enterWindowsByPressingAButton;
    AutoAssembleWrapper<AllowSlowMenacingWalkAndAutowalk> menacingWalkAndAutowalk;
    AutoAssembleWrapper<PlayWithFOV> fovGames;
    AutoAssembleWrapper<PlayWithBombAimCameraTracker2> bombAimExperiments2;
    AutoAssembleWrapper<ModifyConditionalFOVs> changeZoomLevelsWhenAimingBombs;
    AutoAssembleWrapper<InputInjection_CycleEquipmentWhenScrollingMousewheel> cycleEquipmentUsingMouseWheel;

    template<class Hack>
    void DrawCheckboxForHack(Hack& hack, const std::string_view& text)
    {
        if (auto* instance = &hack)
        {
            bool isActive = instance->IsActive();
            if (ImGui::Checkbox(text.data(), &isActive))
            {
                instance->Toggle();
            }
        }
    }
    void ToggleDefaultHacks()
    {
        if (!enterWindowsByPressingAButton.IsActive())
        {
            enterWindowsByPressingAButton.Activate();
            menacingWalkAndAutowalk.Activate();
            changeZoomLevelsWhenAimingBombs.Activate();
            cycleEquipmentUsingMouseWheel.Activate();
        }
        else
        {
            enterWindowsByPressingAButton.Deactivate();
            menacingWalkAndAutowalk.Deactivate();
            changeZoomLevelsWhenAimingBombs.Deactivate();
            cycleEquipmentUsingMouseWheel.Deactivate();
        }
    }
    void OnKeyJustPressed(int keyCode)
    {
        switch (keyCode)
        {
        case VK_NUMPAD7:
            ToggleDefaultHacks();
        default:
            break;
        }
    }
    void DrawControls()
    {
        DrawCheckboxForHack(enterWindowsByPressingAButton, "Enter nearby windows by pressing a button");
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
                "When climbing on a wall, press the specified key (default 'R' like in Syndicate)\n"
                "to enter a nearby window."
            );
        }
        DrawCheckboxForHack(menacingWalkAndAutowalk, "Allow Autowalk and the Slow Menacing Walk");
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
                "CapsLock toggles the slow and menacing walk;\n"
                "When walking in any direction, tap the Autowalk key. Then let go\n"
                "of directional keys, and Arno will keep walking in the same direction.\n"
                "Alternatively:\n"
                " - Stand still\n"
                " - Press and release Autowalk key\n"
                " - Within the next second or so, start walking and let go."
            );
        }
        DrawCheckboxForHack(changeZoomLevelsWhenAimingBombs, "Change Zoom Levels when aiming Bombs");
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
                "FOV is increased when aiming bombs and the Guillotine Gun.\n"
                "Press Right Mouse Button when aiming a bomb to zoom in."
            );
        }
        DrawCheckboxForHack(cycleEquipmentUsingMouseWheel, "Cycle through equipment using mouse wheel");
        if (g_showDevExtraOptions)
        {
            DrawCheckboxForHack(fovGames, "Play with FOV");
            DrawCheckboxForHack(bombAimExperiments2, "Bomb aim experiments2");
        }
    }
};
std::optional<MyHacks> g_MyHacks;
void DrawHacksControls()
{
    if (g_MyHacks)
    {
        g_MyHacks->DrawControls();
    }
}

#include "MyVariousHacks.h"


void MyVariousHacks::Start()
{
    g_MyHacks.emplace();
    g_MyHacks->ToggleDefaultHacks();
}
void MyVariousHacks::MyHacks_OnKeyJustPressed(int keyCode)
{
    if (g_MyHacks)
    {
        g_MyHacks->OnKeyJustPressed(keyCode);
    }
}
