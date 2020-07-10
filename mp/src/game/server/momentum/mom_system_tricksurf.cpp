#include "cbase.h"

#include "mom_system_tricksurf.h"

#include "filesystem.h"
#include "fmtstr.h"
#include "mapzones_build.h"

#include "mom_player.h"
#include "mom_triggers.h"
#include "mom_system_gamemode.h"
#include "mom_system_saveloc.h"
#include "momentum/util/mom_util.h"

#include "tier0/memdbgon.h"

CON_COMMAND_F(mom_tricksurf_record, "Start recording zones to make a trick.\n", FCVAR_MAPPING)
{
    g_pTricksurfSystem->StartRecording();
}

CON_COMMAND_F(mom_tricksurf_record_stop, "Stop recording zones and make a trick. Takes trick name as first param.\n", FCVAR_MAPPING)
{
    g_pTricksurfSystem->StopRecording(args.Arg(1));
}

CON_COMMAND_F(mom_tricksurf_map_tele, "Teleports to a specific map teleport.\n", FCVAR_CLIENTCMD_CAN_EXECUTE)
{
    if (args.ArgC() <= 1)
    {
        Warning("Usage: \"mom_tricksurf_map_tele <num>\".\n");
        return;
    }

    g_pTricksurfSystem->GoToMapTeleport(Q_atoi(args.Arg(1)));
}

CON_COMMAND_F(mom_tricksurf_map_tele_create, "Creates a map teleport, takes a name as a parameter.\n", FCVAR_MAPPING | FCVAR_CLIENTCMD_CAN_EXECUTE)
{
    if (args.ArgC() <= 1)
    {
        Warning("Usage: \"mom_tricksurf_map_tele_create \"<name>\".\n");
        return;
    }

    g_pTricksurfSystem->CreateMapTeleport(args.Arg(1));
}

TrickStepConstraint_MaxSpeed::TrickStepConstraint_MaxSpeed()
{
    m_flMaxSpeed = 410.0f;
}

bool TrickStepConstraint_MaxSpeed::PlayerPassesConstraint(CMomentumPlayer* pPlayer)
{
    return pPlayer->GetAbsVelocity().Length() <= m_flMaxSpeed;
}

void TrickStepConstraint_MaxSpeed::LoadFromKeyValues(KeyValues* pKvIn)
{
    m_flMaxSpeed = pKvIn->GetFloat("speed", 410.0f);
}

void TrickStepConstraint_MaxSpeed::SaveToKeyValues(KeyValues* pKvOut)
{
    pKvOut->SetFloat("speed", m_flMaxSpeed);
}

CTrickStep::CTrickStep()
{
    m_bOptional = false;
    m_pTrigger = nullptr;
}

bool CTrickStep::PlayerPassesConstraints(CMomentumPlayer* pPlayer)
{
    FOR_EACH_VEC(m_vecConstraints, i)
    {
        if (!m_vecConstraints[i]->PlayerPassesConstraint(pPlayer))
            return false;
    }

    return true;
}

void CTrickStep::SaveToKV(KeyValues* pKvOut)
{
    pKvOut->SetBool("optional", m_bOptional);
    pKvOut->SetInt("zone", m_pTrigger->m_iID);

    if (!m_vecConstraints.IsEmpty())
    {
        const auto pConstraintsKv = new KeyValues("constraints");

        FOR_EACH_VEC(m_vecConstraints, i)
        {
            const auto pConstraintKV = pConstraintsKv->CreateNewKey();
            pConstraintKV->SetInt("type", m_vecConstraints[i]->GetType());

            const auto pConstraintDataKV = new KeyValues("data");
            m_vecConstraints[i]->SaveToKeyValues(pConstraintDataKV);
            pConstraintKV->AddSubKey(pConstraintDataKV);
        }

        pKvOut->AddSubKey(pConstraintsKv);
    }
}

void CTrickStep::LoadFromKV(KeyValues* pKvIn)
{
    m_bOptional = pKvIn->GetBool("optional");

    m_pTrigger = g_pTricksurfSystem->GetTrickZone(pKvIn->GetInt("zone", -1));

    if (!m_pTrigger)
    {
        Warning("Failed to load required trick zone from ID %i!\n", pKvIn->GetInt("zone", -1));
        Assert(false);
        return;
    }

    const auto pConstraintsKV = pKvIn->FindKey("constraints");
    if (pConstraintsKV && !pConstraintsKV->IsEmpty())
    {
        FOR_EACH_SUBKEY(pConstraintsKV, pConstraintKV)
        {
            ITrickStepConstraint *pConstraint = nullptr;
            const auto iType = pConstraintKV->GetInt("type", CONSTRAINT_INVALID);
            switch (iType)
            {
            case CONSTRAINT_SPEED_MAX:
                pConstraint = new TrickStepConstraint_MaxSpeed;
            default:
                break;
            }

            if (!pConstraint)
            {
                Warning("!!! Unknown trick constraint type %i !!!\n", iType);
                continue;
            }

            const auto pDataKV = pConstraintKV->FindKey("data");
            if (!pDataKV || pDataKV->IsEmpty())
            {
                Warning("!!! Invalid trick constraint data for type %i !!!\n", iType);
                delete pConstraint;
                continue;
            }

            pConstraint->LoadFromKeyValues(pDataKV); // MOM_TODO check for failure
            m_vecConstraints.AddToTail(pConstraint);
        }
    }
}

CTrickInfo::CTrickInfo()
{
    m_szCreationDate[0] = '\0';
    m_szName[0] = '\0';
    m_szCreatorName[0] = '\0';
}

CTrickInfo::~CTrickInfo()
{
    m_vecTags.PurgeAndDeleteElements();
}

void CTrickInfo::SaveToKV(KeyValues* pKvOut)
{
    const auto pInfoKV = new KeyValues("info");

    pInfoKV->SetString("name", m_szName);
    pInfoKV->SetString("creator", m_szCreatorName);
    pInfoKV->SetString("creation_date", m_szCreationDate);

    if (!m_vecTags.IsEmpty())
    {
        const auto pTagsKv = new KeyValues("tags");
        FOR_EACH_VEC(m_vecTags, i)
        {
            const auto pTag = m_vecTags[i];
            pTagsKv->SetString(CFmtStr("%i", pTag->m_iID), pTag->m_szTagName);
        }
        pInfoKV->AddSubKey(pTagsKv);
    }

    pKvOut->AddSubKey(pInfoKV);
}

void CTrickInfo::LoadFromKV(KeyValues* pKvIn)
{
    Q_strncpy(m_szName, pKvIn->GetString("name"), sizeof(m_szName));
    Q_strncpy(m_szCreatorName, pKvIn->GetString("creator"), sizeof(m_szCreatorName));
    Q_strncpy(m_szCreationDate, pKvIn->GetString("creation_date"), sizeof(m_szCreationDate));

    const auto pTagsKV = pKvIn->FindKey("tags");
    if (pTagsKV && !pTagsKV->IsEmpty())
    {
        FOR_EACH_VALUE(pTagsKV, pKvTag)
        {
            CTrickTag *pTag = new CTrickTag;
            Q_strncpy(pTag->m_szTagName, pKvTag->GetString(), sizeof(pTag->m_szTagName));
            pTag->m_iID = Q_atoi(pKvTag->GetName());

            m_vecTags.AddToTail(pTag);
        }
    }
}


CTrick::CTrick()
{
    m_iID = -1;
}

void CTrick::SetName(const char* pName)
{
    Q_strncpy(m_Info.m_szName, pName, sizeof(m_Info.m_szName));
}

CTrickStep* CTrick::Step(int iStepIndx)
{
    if (iStepIndx < 0 || iStepIndx >= m_vecSteps.Count())
        return nullptr;

    return m_vecSteps[iStepIndx];
}

void CTrick::SaveToKV(KeyValues* pKvOut)
{
    pKvOut->SetName(CFmtStr("%i", m_iID));

    m_Info.SaveToKV(pKvOut);

    KeyValues *pKvSteps = new KeyValues("steps");
    FOR_EACH_VEC(m_vecSteps, i)
    {
        const auto pKvStep = pKvSteps->CreateNewKey();
        m_vecSteps[i]->SaveToKV(pKvStep);
    }
    pKvOut->AddSubKey(pKvSteps);
}

bool CTrick::LoadFromKV(KeyValues* pKvIn)
{
    m_iID = Q_atoi(pKvIn->GetName());

    const auto pInfoKV = pKvIn->FindKey("info");
    if (!pInfoKV || pInfoKV->IsEmpty())
        return false;

    m_Info.LoadFromKV(pInfoKV);

    const auto pStepsKV = pKvIn->FindKey("steps");
    if (!pStepsKV || pStepsKV->IsEmpty())
        return false;

    FOR_EACH_SUBKEY(pStepsKV, pStepKV)
    {
        const auto pStep = new CTrickStep;
        pStep->LoadFromKV(pStepKV);
        m_vecSteps.AddToTail(pStep);
    }

    return true;
}

CTrickAttempt::CTrickAttempt(CTrick *pTrick) : m_pTrick(pTrick)
{
    m_iStartTick = gpGlobals->tickcount;
    m_iCurrentStep = 0;
}

bool CTrickAttempt::ShouldContinue(CTriggerTrickZone *pZone, CMomentumPlayer *pPlayer)
{
    int iTotalSteps = m_pTrick->StepCount();
    int iNextStep = m_iCurrentStep + 1;

    // Go through the trick stepping over optional zones to find our next step
    for (auto pNextStep = m_pTrick->Step(iNextStep);
         iNextStep < iTotalSteps && pNextStep;
         pNextStep = m_pTrick->Step(++iNextStep))
    {
        // Early out if this trick's sequence is properly broken
        if (pNextStep->GetTrigger() != pZone && !pNextStep->IsOptional())
            return false;

        // Here: the trigger is our zone, AND/OR we're optional

        if (pNextStep->GetTrigger() == pZone)
        {
            if (pNextStep->IsOptional())
            {
                // Increase our step and continue
                m_iCurrentStep = iNextStep;
                return true;
            }

            // Otherwise we're necessary

            // Do we pass the constraints?
            if (!pNextStep->PlayerPassesConstraints(pPlayer))
                return false;

            // Trigger was our guy, wasn't optional, and we passed the constraints. Get outta here.
            break;
        }

        // Otherwise the trigger isn't our guy, but we're still optional, so we continue the loop.
        // We either find the next optional zone in the sequence, the next required zone,
        // or you hit a no-no zone and your trick will crash and burn
    }

    // Is the trick done?
    if (iNextStep == iTotalSteps - 1)
    {
        Complete(pPlayer);
        g_pTricksurfSystem->CompleteTrick(this);
        return false; // This removes the attempt automatically
    }

    if (iNextStep >= iTotalSteps)
    {
        return false;
    }

    m_iCurrentStep = iNextStep;
    return true;
}

void CTrickAttempt::Complete(CMomentumPlayer* pPlayer)
{
    ClientPrint(pPlayer, HUD_PRINTTALK, CFmtStr("Trick \"%s\" completed in %.2fs!", m_pTrick->GetName(), GetElapsed()));
}

CMapTeleport::CMapTeleport()
{
    m_pLoc = nullptr;
    m_szName[0] = '\0';
}

void CMapTeleport::SaveToKV(KeyValues* pKvOut)
{
    pKvOut->SetString("name", m_szName);
    KeyValues *pLocKV = new KeyValues("loc");
    MomUtil::KVSaveVector(pLocKV, "pos", m_pLoc->pos);
    MomUtil::KVSaveQAngles(pLocKV, "ang", m_pLoc->ang);
    pKvOut->AddSubKey(pLocKV);
}

void CMapTeleport::LoadFromKV(KeyValues* pKvIn)
{
    Q_strncpy(m_szName, pKvIn->GetString("name", "UNNAMED MAP TELEPORT!!!"), sizeof(m_szName));

    const auto pLocKV = pKvIn->FindKey("loc");
    if (pLocKV && !pLocKV->IsEmpty())
    {
        m_pLoc = new SavedLocation_t;
        m_pLoc->Load(pKvIn->FindKey("loc"));
    }
}

CTricksurfSystem::CTricksurfSystem() : CAutoGameSystem("CTricksurfSystem")
{
    m_bRecording = false;
}

void CTricksurfSystem::LevelInitPostEntity()
{
    //if (!g_pGameModeSystem->GameModeIs(GAMEMODE_TRICKSURF))
    //    return;

    // LoadTrickDataFromFile();
}

void CTricksurfSystem::LevelShutdownPreEntity()
{
    if (!g_pGameModeSystem->GameModeIs(GAMEMODE_TRICKSURF) || !gpGlobals->mapname.ToCStr())
        return;

    SaveTrickDataToFile();

    ClearTrickAttempts();
    m_vecRecordedZones.RemoveAll();
    m_llTrickList.PurgeAndDeleteElements();
    m_vecTrickZones.RemoveAll();
    m_vecMapTeleports.PurgeAndDeleteElements();
}

// If on zone enter the zone is either one of the optionals until a required, or is the required,
// the current trick stays in and the iterator increases. If neither of those, the trick is removed from m_llCurrentTricks.
void CTricksurfSystem::OnTrickZoneEnter(CTriggerTrickZone *pZone, CMomentumPlayer *pPlayer)
{
    if (m_bRecording)
    {
        m_vecRecordedZones.AddToTail(pZone);
    }
    else
    {
        FOR_EACH_VEC_BACK(m_vecCurrentTrickAttempts, i)
        {
            const auto pTrickAttempt = m_vecCurrentTrickAttempts[i];

            if (!pTrickAttempt->ShouldContinue(pZone, pPlayer))
            {
                DevMsg("Not continuing trick %s !\n", pTrickAttempt->GetTrick()->GetName());
                m_vecCurrentTrickAttempts.Remove(i);
                delete pTrickAttempt;
            }
            else
            {
                DevMsg("Continuing trick %s ...\n", pTrickAttempt->GetTrick()->GetName());
            }
        }
    }
}

// If the zone is the start of a trick and we pass the start's constraints, add it to the list of current tricks (if not already there)
void CTricksurfSystem::OnTrickZoneExit(CTriggerTrickZone *pZone, CMomentumPlayer *pPlayer)
{
    if (m_bRecording)
    {
        if (m_vecRecordedZones.IsEmpty())
            m_vecRecordedZones.AddToTail(pZone);
    }
    else
    {
        FOR_EACH_LL(m_llTrickList, i)
        {
            const auto pTrick = m_llTrickList[i];
            const auto pTrickFirstStep = pTrick->Step(0);
            if (pTrickFirstStep->GetTrigger() == pZone && pTrickFirstStep->PlayerPassesConstraints(pPlayer))
            {
                bool bFound = false;
                FOR_EACH_VEC(m_vecCurrentTrickAttempts, trickItr)
                {
                    const auto pTrickAttempt = m_vecCurrentTrickAttempts[trickItr];
                    if (pTrickAttempt->GetTrick() == pTrick)
                    {
                        bFound = true;
                        break;
                    }
                }

                if (!bFound)
                {
                    const auto pTrickAttempt = new CTrickAttempt(pTrick);
                    m_vecCurrentTrickAttempts.AddToTail(pTrickAttempt);
                    DevMsg("Added trick attempt for trick %s !\n", pTrick->GetName());
                }
            }
        }
    }
}

void CTricksurfSystem::CompleteTrick(CTrickAttempt* pAttempt)
{
    // Woohoo!
}

void CTricksurfSystem::ClearTrickAttempts()
{
    m_vecCurrentTrickAttempts.PurgeAndDeleteElements();
}

void CTricksurfSystem::StartRecording()
{
    if (m_bRecording)
    {
        Warning("Already recording tricks!\n");
        return;
    }

    m_vecRecordedZones.RemoveAll();
    m_bRecording = true;
}

void CTricksurfSystem::StopRecording(const char *pTrickName)
{
    if (!m_bRecording)
    {
        Warning("Not recording tricks!\n");
        return;
    }

    CTrick *pTrick = new CTrick;
    pTrick->SetID(m_llTrickList.AddToTail(pTrick));
    pTrick->SetName(pTrickName);

    FOR_EACH_VEC(m_vecRecordedZones, i)
    {
        auto pZone = m_vecRecordedZones[i];

        auto pTrickStep = new CTrickStep;
        pTrickStep->SetTrigger(pZone);

        if (i == 0)
        {
            pTrickStep->AddConstraint(new TrickStepConstraint_MaxSpeed);
        }

        pTrick->AddStep(pTrickStep);
    }

    m_bRecording = false;

    SaveTrickDataToFile();
}

CTriggerTrickZone* CTricksurfSystem::GetTrickZone(int id)
{
    if (id < 0 || id >= m_vecTrickZones.Count())
        return nullptr;

    return m_vecTrickZones[id];
}

void CTricksurfSystem::AddZone(CTriggerTrickZone* pZone)
{
    if (pZone->m_iID == -1) // Created by zone creator, assign an ID
    {
        pZone->m_iID = m_vecTrickZones.AddToTail();
    }
    else
    {
        m_vecTrickZones.EnsureCount(pZone->m_iID + 1); // ensure we can support this new ID
    }

    m_vecTrickZones[pZone->m_iID] = pZone;
}

void CTricksurfSystem::CreateMapTeleport(const char* pName)
{
    const auto pMapTeleport = new CMapTeleport;
    Q_strncpy(pMapTeleport->m_szName, pName, sizeof(pMapTeleport->m_szName));
    pMapTeleport->m_pLoc = g_pMOMSavelocSystem->CreateSaveloc();

    m_vecMapTeleports.AddToTail(pMapTeleport);

    SaveTrickDataToFile();
}

void CTricksurfSystem::GoToMapTeleport(int iTeleportNum)
{
    if (iTeleportNum <= 0 || iTeleportNum > m_vecMapTeleports.Count())
    {
        Warning("Invalid map teleport number %i!\n", iTeleportNum);
        return;
    }

    ClearTrickAttempts();

    const auto pMapTele = m_vecMapTeleports[iTeleportNum - 1];

    const auto pPlayer = CMomentumPlayer::GetLocalPlayer();
    pPlayer->Teleport(&pMapTele->m_pLoc->pos, &pMapTele->m_pLoc->ang, nullptr);
}

const char* CTricksurfSystem::GetTricksFileName()
{
    return CFmtStr("%s/%s.tricks", ZONE_FOLDER, gpGlobals->mapname.ToCStr());
}

void CTricksurfSystem::LoadTrickDataFromFile()
{
    const auto pMapName = gpGlobals->mapname.ToCStr();

    KeyValuesAD pTrickFile("TrickData");
    if (!pTrickFile->LoadFromFile(g_pFullFileSystem, GetTricksFileName(), "MOD"))
    {
        Warning("No trick data file found for the map %s !\n", pMapName);
        return;
    }

    const auto pMapTelesKV = pTrickFile->FindKey("map_teles");
    if (pMapTelesKV && !pMapTelesKV->IsEmpty())
    {
        FOR_EACH_SUBKEY(pMapTelesKV, pMapTeleKV)
        {
            const auto pMapTele = new CMapTeleport;
            pMapTele->LoadFromKV(pMapTeleKV);

            m_vecMapTeleports.AddToTail(pMapTele);
        }
    }

    const auto pZones = pTrickFile->FindKey("zones");
    if (!pZones || pZones->IsEmpty())
    {
        Warning("No zones found in the trick data for map %s!\n", pMapName);
        return;
    }

    FOR_EACH_SUBKEY(pZones, pZoneKV)
    {
        const auto pEntity = dynamic_cast<CTriggerTrickZone *>(CreateEntityByName("trigger_momentum_trick"));

        AssertMsg(pEntity, "Trick zone entity failed to create!!");

        if (pEntity)
        {
            if (!pEntity->LoadFromKeyValues(pZoneKV))
            {
                Warning("Failed to load trick zone!\n");
                Assert(false);
                return;
            }

            CMomBaseZoneBuilder *pBaseBuilder = CreateZoneBuilderFromKeyValues(pZoneKV);

            pBaseBuilder->BuildZone();
            pEntity->Spawn();
            pBaseBuilder->FinishZone(pEntity);

            pEntity->Activate();

            delete pBaseBuilder;
        }
    }

    // Load tricks for map
    const auto pTricks = pTrickFile->FindKey("tricks");
    if (!pTricks || pTricks->IsEmpty())
    {
        Warning("No tricks found in the trick data for map %s!\n", pMapName);
        return;
    }

    FOR_EACH_SUBKEY(pTricks, pTrickKV)
    {
        CTrick *pNewTrick = new CTrick;

        if (pNewTrick->LoadFromKV(pTrickKV))
        {
            m_llTrickList.AddToTail(pNewTrick);
        }
    }
}

void CTricksurfSystem::LoadTrickDataFromSite(KeyValues* pKvTrickData)
{
    AssertMsg(false, "Implement me!!");
}

void CTricksurfSystem::SaveTrickDataToFile()
{
    const auto pMapName = gpGlobals->mapname.ToCStr();
    if (!pMapName || !pMapName[0])
        return;

    KeyValuesAD trickDataKV("TrickData");

    if (!m_vecTrickZones.IsEmpty())
    {
        KeyValues *pZonesKV = new KeyValues("zones");

        FOR_EACH_VEC(m_vecTrickZones, i)
        {
            const auto pZoneKV = pZonesKV->CreateNewKey();

            const auto pZoneTrigger = m_vecTrickZones[i];

            bool bSuccess = false;
            if (pZoneTrigger->ToKeyValues(pZoneKV))
            {
                auto pBuilder = CreateZoneBuilderFromExisting(pZoneTrigger);

                bSuccess = pBuilder->Save(pZoneKV);

                delete pBuilder;
            }

            if (!bSuccess)
            {
                Warning("Failed to save zone to file!\n");
                pZonesKV->RemoveSubKey(pZoneKV);
                pZoneKV->deleteThis();
            }
        }

        trickDataKV->AddSubKey(pZonesKV);
    }

    if (m_llTrickList.Count())
    {
        KeyValues *pTricksKV = new KeyValues("tricks");

        FOR_EACH_LL(m_llTrickList, i)
        {
            const auto pTrickKV = pTricksKV->CreateNewKey();

            m_llTrickList[i]->SaveToKV(pTrickKV);
        }

        trickDataKV->AddSubKey(pTricksKV);
    }

    if (m_vecMapTeleports.Count())
    {
        KeyValues *pMapTeleKV = new KeyValues("map_teles");

        FOR_EACH_VEC(m_vecMapTeleports, i)
        {
            const auto pTeleKV = pMapTeleKV->CreateNewKey();

            m_vecMapTeleports[i]->SaveToKV(pTeleKV);
        }

        trickDataKV->AddSubKey(pMapTeleKV);
    }

    trickDataKV->SaveToFile(g_pFullFileSystem, GetTricksFileName(), "MOD");
}

static CTricksurfSystem s_TricksurfSystem;
CTricksurfSystem *g_pTricksurfSystem = &s_TricksurfSystem;