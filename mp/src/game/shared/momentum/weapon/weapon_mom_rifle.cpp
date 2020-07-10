#include "cbase.h"

#include "mom_player_shared.h"
#include "weapon_mom_rifle.h"

#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(MomentumRifle, DT_MomentumRifle)

BEGIN_NETWORK_TABLE(CMomentumRifle, DT_MomentumRifle)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CMomentumRifle)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_momentum_rifle, CMomentumRifle);
PRECACHE_WEAPON_REGISTER(weapon_momentum_rifle);
