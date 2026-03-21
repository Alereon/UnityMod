#ifndef G_2V2MOD_MAIN_H
#define G_2V2MOD_MAIN_H

#define TVT_SPAWN 0
#define TVT_FIRST_SPAWN 1

#define TVT_PRINT_MAX_JOBS 8

typedef struct {
    int   clientNum;
    char *text;
    int   offset;
    char  lastColor;
} tvt_printJob_t;

typedef struct {
    int kills;
    int deaths;
    int suicides;
    int teamKills;
    int dmgGiven;
    int dmgReceived;
    int teamDmg;
} tvt_ClientStats_t;

// Client mod struct
typedef struct {
    tvt_ClientStats_t stats;
    qboolean          queued;
    int               queueTime;
    qboolean          isHeadlessClient;	// is a headless client or demobot
    int               pendingButtons;
    byte              pendingGenericCmd;
} tvt_ClientState_t;

typedef struct {
    int      playedLastRound;
    qboolean matchInProgress;
    qboolean restartPending;
    int      restartTime;
    int      readyMask;
} tvt_MatchState_t;

typedef struct tvt_CachedTable_s {
    table_t                  **table;
    struct tvt_CachedTable_s  *next;
} tvt_CachedTable_t;

// Global mod struct
typedef struct {
    int                physicsMsec;
    int                spawnArmor[2];
    int                spawnItems[2];
    tvt_printJob_t     printJobs[TVT_PRINT_MAX_JOBS];
    int                numPrintJobs;
    tvt_MatchState_t   match;
    char               colorChar;
    tvt_CachedTable_t *cachedTables;
} tvt_ModState_t;

void     G_TvT_Init(void);
qboolean G_TvT_CheckReadyUp(void);
void     G_TvT_SyncReadyMask(void);
void     G_TvT_RegisterCachedTable(table_t **tablePtr);
void     G_TvT_UpdateCachedTables(void);

#endif
