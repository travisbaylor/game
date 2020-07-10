#pragma once

#include "weapon_mom_smg.h"

#ifdef CLIENT_DLL
#define CMomentumRifle C_MomentumRifle
#endif

class CMomentumRifle : public CMomentumSMG
{
public:
    DECLARE_CLASS(CMomentumRifle, CWeaponBaseGun);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();
};