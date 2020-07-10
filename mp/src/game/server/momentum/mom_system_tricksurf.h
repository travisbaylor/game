#pragma once

#include "igamesystem.h"

struct SavedLocation_t;
class CTriggerTrickZone;
class CMomentumPlayer;

enum TrickConstraintType_t
{
    CONSTRAINT_SPEED_MAX = 0,
    CONSTRAINT_SPEED_AXIS,
    CONSTRAINT_NO_LANDING,


    // Add more constraints above me
    CONSTRAINT_COUNT,

    CONSTRAINT_FIRST = CONSTRAINT_SPEED_MAX,
    CONSTRAINT_LAST = CONSTRAINT_COUNT - 1,
    CONSTRAINT_INVALID = -1,
};

abstract_class ITrickStepConstraint
{
public:
    virtual bool PlayerPassesConstraint(CMomentumPlayer *pPlayer) = 0;
    virtual void SaveToKeyValues(KeyValues *pKvOut) = 0;
    virtual void LoadFromKeyValues(KeyValues *pKvIn) = 0;
    virtual TrickConstraintType_t GetType() = 0;
    virtual ~ITrickStepConstraint() {}
};

class TrickStepConstraint_MaxSpeed : public ITrickStepConstraint
{
public:
    TrickStepConstraint_MaxSpeed();

    TrickConstraintType_t GetType() override { return CONSTRAINT_SPEED_MAX; }
    bool PlayerPassesConstraint(CMomentumPlayer* pPlayer) override;

    void LoadFromKeyValues(KeyValues* pKvIn) override;
    void SaveToKeyValues(KeyValues* pKvOut) override;

private:
    float m_flMaxSpeed;
};

class CTrickStep
{
public:
    CTrickStep();

    bool PlayerPassesConstraints(CMomentumPlayer *pPlayer);
    void AddConstraint(ITrickStepConstraint *pConstraint) { m_vecConstraints.AddToTail(pConstraint); }

    void SetTrigger(CTriggerTrickZone *pZone) { m_pTrigger = pZone; }
    CTriggerTrickZone *GetTrigger() const { return m_pTrigger; }

    void SetOptional(bool bOptional) { m_bOptional = bOptional; }
    bool IsOptional() const { return m_bOptional; }

    void SaveToKV(KeyValues *pKvOut);
    void LoadFromKV(KeyValues *pKvIn);

private:
    bool m_bOptional;
    CTriggerTrickZone *m_pTrigger;
    CUtlVector<ITrickStepConstraint*> m_vecConstraints;
};

struct CTrickTag
{
    int m_iID;
    char m_szTagName[128];
};

struct CTrickInfo
{
    char m_szName[128];
    char m_szCreationDate[128];
    char m_szCreatorName[64];
    CUtlVector<CTrickTag*> m_vecTags;

    CTrickInfo();
    ~CTrickInfo();

    void SaveToKV(KeyValues *pKvOut);
    void LoadFromKV(KeyValues *pKvIn);
};

class CTrick
{
public:
    CTrick();

    void SetID(int iID) { m_iID = iID; }
    int GetID() { return m_iID; }

    void SetName(const char *pName);
    const char *GetName() const { return m_Info.m_szName; }

    int StepCount() const { return m_vecSteps.Count(); }
    CTrickStep* Step(int iStepIndx);

    void AddStep(CTrickStep *pStep) { m_vecSteps.AddToTail(pStep); }

    void SaveToKV(KeyValues *pKvOut);
    bool LoadFromKV(KeyValues *pKvIn);

private:
    int m_iID; // Website
    CTrickInfo m_Info;

    CUtlVector<CTrickStep*> m_vecSteps;
};

class CTrickAttempt
{
public:
    CTrickAttempt(CTrick *pTrick);

    bool ShouldContinue(CTriggerTrickZone *pZone, CMomentumPlayer *pPlayer);

    void Complete(CMomentumPlayer *pPlayer);

    CTrick *GetTrick() const { return m_pTrick; }

    // In seconds
    float GetElapsed() const { return gpGlobals->interval_per_tick * float(gpGlobals->tickcount - m_iStartTick); }
private:
    int m_iStartTick; // Timer purposes
    int m_iCurrentStep; // Current trick step that we're on

    CTrick *m_pTrick;
};

struct CMapTeleport
{
    char m_szName[32];
    SavedLocation_t *m_pLoc;

    CMapTeleport();
    void SaveToKV(KeyValues *pKvOut);
    void LoadFromKV(KeyValues *pKvIn);
};

class CTricksurfSystem : public CAutoGameSystem
{
public:
    CTricksurfSystem();

    void LevelInitPostEntity() override; // Load tricks, start recording
    void LevelShutdownPreEntity() override; // Stop recording, clear out tricks memory

    void SaveTrickDataToFile();
    void LoadTrickDataFromFile();
    void LoadTrickDataFromSite(KeyValues *pKvTrickData);

    void OnTrickZoneEnter(CTriggerTrickZone *pZone, CMomentumPlayer *pPlayer);
    void OnTrickZoneExit(CTriggerTrickZone *pZone, CMomentumPlayer *pPlayer);

    void CompleteTrick(CTrickAttempt *pAttempt);
    void ClearTrickAttempts();

    void StartRecording(); // Creating a trick
    void StopRecording(const char *pTrickName); // Stops recording the trick

    CTriggerTrickZone *GetTrickZone(int id);
    void AddZone(CTriggerTrickZone *pZone);

    void CreateMapTeleport(const char *pName);
    void GoToMapTeleport(int iTeleportNum);

private:
    const char *GetTricksFileName();

    CUtlVector<CTriggerTrickZone*> m_vecRecordedZones;
    bool m_bRecording;

    // Tricks currently being attempted.
    CUtlVector<CTrickAttempt*> m_vecCurrentTrickAttempts;
    // Every trick loaded for the map
    CUtlLinkedList<CTrick*> m_llTrickList;
    // Keeping track. ID is their index into the array.
    CUtlVector<CTriggerTrickZone*> m_vecTrickZones;
    CUtlVector<CMapTeleport*> m_vecMapTeleports;
};

extern CTricksurfSystem *g_pTricksurfSystem;