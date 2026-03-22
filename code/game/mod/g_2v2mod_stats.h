#ifndef G_2V2MOD_STATS_H
#define G_2V2MOD_STATS_H

#define TVT_MATCH_LOG_DIR  "match_logs"
#define TVT_CASUAL_LOG_DIR "casual_logs"

typedef enum {
    STAT_KILLS,
    STAT_DEATHS,
    STAT_SUICIDES,
    STAT_TEAMKILLS,
    STAT_DMG_GIVEN,
    STAT_DMG_RECV,
    STAT_NET_DMG,
    STAT_TEAM_DMG,
    STAT_SCORE,
    STAT_COUNT
} tvt_statType_t;

typedef enum {
    HIGHLIGHT_HIGHEST,
    HIGHLIGHT_LOWEST
} tvt_highlightDir_t;

typedef struct {
    const char        *name;
    tvt_statType_t     stat;
    tvt_highlightDir_t highlight;
    qboolean           teamOnly;
} tvt_statsColumn_t;

typedef struct {
    int clientNum;
    int vals[STAT_COUNT];
} tvt_EndGamePlayer_t;

typedef struct {
    tvt_EndGamePlayer_t *players;
    table_t             *table;
    int                  count;
    team_t               team;
} tvt_statsGroup_t;

typedef struct {
    int      colIdx;
    int      bestVal;
    qboolean allSame;
} tvt_colInfo_t;

void     G_TvT_Stats_TrackDamage(gentity_t *targ, gentity_t *attacker, int damage);
void     G_TvT_Stats_TrackKill(gentity_t *self, gentity_t *attacker);
qboolean G_TvT_Cmd_Stats(gentity_t *ent);
void     G_TvT_Stats_EndGame(void);

#endif
