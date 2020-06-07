#include "cbase.h"

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ProgressBar.h>

#include "c_mom_player.h"
#include "hud.h"
#include "hudelement.h"
#include "iclientmode.h"

#include "mom_system_gamemode.h"
#include "weapon/weapon_mom_concgrenade.h"

#include "tier0/memdbgon.h"

using namespace vgui;

static MAKE_TOGGLE_CONVAR(mom_hud_conc_timer_enable, "1", FCVAR_ARCHIVE, "Toggles the conc timer on or off.\n");
static MAKE_TOGGLE_CONVAR(mom_hud_conc_timer_countdown, "0", FCVAR_ARCHIVE, "Toggles whether the countdown for the conc timer is shown.\n");

class CHudConcTimer : public CHudElement, public EditablePanel
{
    DECLARE_CLASS_SIMPLE(CHudConcTimer, EditablePanel);

  public:
    CHudConcTimer(const char *pElementName);

    bool ShouldDraw() OVERRIDE;
    void OnThink() OVERRIDE;
    void Reset() OVERRIDE;

  private:
    CMomentumConcGrenade *m_pGrenade;
    ContinuousProgressBar *m_pTimer;
    Label *m_pTimerLabel;
};

DECLARE_HUDELEMENT(CHudConcTimer);

CHudConcTimer::CHudConcTimer(const char *pElementName)
    : CHudElement(pElementName), EditablePanel(g_pClientMode->GetViewport(), "HudConcTimer")
{
    m_pGrenade = nullptr;
    m_pTimer = new ContinuousProgressBar(this, "ConcTimer");
    m_pTimerLabel = new Label(this, "ConcTimerLabel", "Timer");

    LoadControlSettings("resource/ui/HudConcTimer.res");
}

void CHudConcTimer::Reset()
{
    m_pGrenade = nullptr;
}

bool CHudConcTimer::ShouldDraw()
{
    if (!mom_hud_conc_timer_enable.GetBool())
        return false;

    C_MomentumPlayer *pPlayer = C_MomentumPlayer::GetLocalMomPlayer();

    if (!pPlayer || !g_pGameModeSystem->GameModeIs(GAMEMODE_CONC) || !pPlayer->IsAlive())
        return false;

    const auto pWeapon = pPlayer->GetActiveWeapon();

    if (!pWeapon || pWeapon->GetWeaponID() != WEAPON_CONCGRENADE)
        return false;

    m_pGrenade = static_cast<CMomentumConcGrenade*>(pWeapon);

    return CHudElement::ShouldDraw();
}

void CHudConcTimer::OnThink()
{
    if (!m_pGrenade)
        return;

    // Reset the timer label when the conc explodes
    if (mom_hud_conc_timer_countdown.GetBool() && m_pGrenade->GetGrenadeTimer() <= 0.0f)
    {
        m_pTimerLabel->SetText("TIMER");
    }

    /*
    // Turn timer red while inside start zone to indicate that concs can't be thrown
    if (!m_pGrenade->IsChargeEnabled())
    {
        m_pTimer->SetFgColor(Color(192, 28, 0, 140));
        m_pTimer->SetProgress(1.0f);

        m_pTimerLabel->SetVisible(false);
    }

    // If charge label was disabled by start zone, enable it again
    if (!m_pTimerLabel->IsVisible())
        m_pTimerLabel->SetVisible(true);
    */

    m_pTimer->SetFgColor(Color(235, 235, 235, 255));
    float flMaxTimer = m_pGrenade->GetMaxTimer();

    if (!CloseEnough(flMaxTimer, 0.0f, FLT_EPSILON))
    {
        float flGrenadeTimer = m_pGrenade->GetGrenadeTimer();

        if (flGrenadeTimer > 0)
        {
            float flPrimedTime = max(0, gpGlobals->curtime - flGrenadeTimer);
            float flCookedPercent = min(1.0, flPrimedTime / flMaxTimer);

            m_pTimer->SetProgress(flCookedPercent);

            if (mom_hud_conc_timer_countdown.GetBool())
            {
                char buf[64];
                Q_snprintf(buf, sizeof(buf), "%.2f s", flPrimedTime);

                 m_pTimerLabel->SetText(buf);
            }
        }
        else
        {
            m_pTimer->SetProgress(0.0f);
        }
    }
}