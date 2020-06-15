#pragma once

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ProgressBar.h>

#include "hudelement.h"

#include "mom_concgrenade.h"
#include "weapon/weapon_mom_concgrenade.h"

class CHudConcTimer : public CHudElement, public vgui::EditablePanel
{
    DECLARE_CLASS_SIMPLE(CHudConcTimer, EditablePanel);

    CHudConcTimer(const char *pElementName);

    bool ShouldDraw() OVERRIDE;
    void OnThink() OVERRIDE;
    void Reset() OVERRIDE;

  private:
    CMomentumConcGrenade *m_pGrenade;
    vgui::ContinuousProgressBar *m_pTimer;
    vgui::Label *m_pTimerLabel;
};

DECLARE_HUDELEMENT(CHudConcTimer);

class CHudConcEntPanel : public vgui::Panel
{
    DECLARE_CLASS_SIMPLE(CHudConcEntPanel, vgui::Panel);

    CHudConcEntPanel();
    ~CHudConcEntPanel();

    void Init(CMomConcProjectile *pEntity);
    void OnThink() OVERRIDE;
    void OnTick() OVERRIDE;

    bool ShouldDraw();

    bool GetEntityPosition(int &sx, int &sy);
    void ComputeAndSetSize();

  private:
    CMomConcProjectile *m_pGrenade;
    vgui::ContinuousProgressBar *m_pHudTimer;
    vgui::Label *m_pTimerLabel;

    int m_iOrgWidth;
    int m_iOrgHeight;
    int m_iOrgOffsetX;
    int m_iOrgOffsetY;
    // Offset from entity that we should draw
    int m_OffsetX, m_OffsetY;
    // Position of the panel
    int m_iPosX, m_iPosY;
};