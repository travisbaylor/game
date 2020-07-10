#pragma once

#include "weapon_mom_smg.h"

#ifdef CLIENT_DLL
#define CMomentumLMG C_MomentumLMG
#endif

class CMomentumLMG : public CMomentumSMG
{
public:
    DECLARE_CLASS(CMomentumLMG, CWeaponBaseGun);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();
};