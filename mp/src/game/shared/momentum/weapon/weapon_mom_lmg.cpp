#include "cbase.h"

#include "weapon_mom_lmg.h"
#include "mom_player_shared.h"

#include "tier0/memdbgon.h"


IMPLEMENT_NETWORKCLASS_ALIASED(MomentumLMG, DT_MomentumLMG)

BEGIN_NETWORK_TABLE(CMomentumLMG, DT_MomentumLMG)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CMomentumLMG)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_momentum_lmg, CMomentumLMG);
PRECACHE_WEAPON_REGISTER(weapon_momentum_lmg);
