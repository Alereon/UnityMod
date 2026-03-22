#include "../g_local.h"

static JSON_t *G_TvT_Stats_BuildMatchJSON(tvt_statsGroup_t *groups, int numGroups, qtime_t *time);

void G_TvT_Stats_TrackDamage(gentity_t *targ, gentity_t *attacker, int damage) {
    if (!targ->client || !attacker || !attacker->client || targ == attacker) {
        return;
    }

    if (damage <= 0) {
        return;
    }

    if (OnSameTeam(targ, attacker)) {
        attacker->client->tvt.stats.teamDmg += damage;
    }
    attacker->client->tvt.stats.dmgGiven += damage;
    targ->client->tvt.stats.dmgReceived += damage;
}

void G_TvT_Stats_TrackKill(gentity_t *self, gentity_t *attacker) {
    if (!self || !self->client || !attacker || !attacker->client) {
        return;
    }

    if (attacker == self) {
        self->client->tvt.stats.suicides++;
    }
    else {
        if (OnSameTeam(self, attacker)) {
            attacker->client->tvt.stats.teamKills++;
        }
        attacker->client->tvt.stats.kills++;
        self->client->tvt.stats.deaths++;
    }
}

static const tvt_statsColumn_t g_tvt_statsCols[] = {
    {"Kills", STAT_KILLS, HIGHLIGHT_HIGHEST, qfalse},
    {"Deaths", STAT_DEATHS, HIGHLIGHT_LOWEST, qfalse},
    {"Suicides", STAT_SUICIDES, HIGHLIGHT_LOWEST, qfalse},
    {"Team Kills", STAT_TEAMKILLS, HIGHLIGHT_LOWEST, qtrue},
    {"Dmg Given", STAT_DMG_GIVEN, HIGHLIGHT_HIGHEST, qfalse},
    {"Dmg Received", STAT_DMG_RECV, HIGHLIGHT_LOWEST, qfalse},
    {"NET Dmg", STAT_NET_DMG, HIGHLIGHT_HIGHEST, qfalse},
    {"Team Dmg", STAT_TEAM_DMG, HIGHLIGHT_LOWEST, qtrue},
    {"Score", STAT_SCORE, HIGHLIGHT_HIGHEST, qfalse},
};

#define STATS_NUM_COLS (sizeof(g_tvt_statsCols) / sizeof(g_tvt_statsCols[0]))

static qboolean G_TvT_Stats_ColActive(int col) {
    if (g_tvt_statsCols[col].teamOnly && g_gametype.integer < GT_TEAM) {
        return qfalse;
    }
    return qtrue;
}

static void G_TvT_Stats_Transform(int *stats, gclient_t *cl) {
    tvt_ClientStats_t *s = &cl->tvt.stats;
    stats[STAT_KILLS] = s->kills;
    stats[STAT_DEATHS] = s->deaths;
    stats[STAT_SUICIDES] = s->suicides;
    stats[STAT_TEAMKILLS] = s->teamKills;
    stats[STAT_DMG_GIVEN] = s->dmgGiven;
    stats[STAT_DMG_RECV] = s->dmgReceived;
    stats[STAT_NET_DMG] = s->dmgGiven - s->dmgReceived;
    stats[STAT_TEAM_DMG] = s->teamDmg;
    stats[STAT_SCORE] = cl->ps.persistant[PERS_SCORE];
}

static int G_TvT_Stats_GatherPlayers(tvt_EndGamePlayer_t *out, team_t team) {
    int i;
    int count = 0;

    for (i = 0; i < level.maxclients; i++) {
        gclient_t *cl = &level.clients[i];

        if (cl->pers.connected != CON_CONNECTED || cl->sess.sessionTeam == TEAM_SPECTATOR) {
            continue;
        }
        if (team != TEAM_FREE && cl->sess.sessionTeam != team) {
            continue;
        }

        out[count].clientNum = i;
        G_TvT_Stats_Transform(out[count].vals, cl);
        count++;
    }

    return count;
}

static void G_TvT_Stats_Highlight(tvt_statsGroup_t *groups, int numGroups, int colOffset) {
    tvt_colInfo_t cols[STATS_NUM_COLS];
    int           numActive = 0;
    int           totalPlayers = 0;
    int           i, j, k;

    for (j = 0; j < numGroups; j++) {
        totalPlayers += groups[j].count;
    }
    if (totalPlayers < 2) {
        return;
    }

    // Build active column map and find best value per column in one pass.
    for (i = 0; i < STATS_NUM_COLS; i++) {
        tvt_statType_t stat;
        tvt_colInfo_t *col;
        qboolean       first = qtrue;

        if (!G_TvT_Stats_ColActive(i)) {
            continue;
        }

        col = &cols[numActive];
        col->colIdx = i;
        col->allSame = qtrue;
        stat = g_tvt_statsCols[i].stat;

        for (j = 0; j < numGroups; j++) {
            for (k = 0; k < groups[j].count; k++) {
                int val = groups[j].players[k].vals[stat];
                if (first) {
                    col->bestVal = val;
                    first = qfalse;
                }
                else {
                    if (val != col->bestVal) {
                        col->allSame = qfalse;
                    }
                    if (g_tvt_statsCols[i].highlight == HIGHLIGHT_LOWEST ? val < col->bestVal
                                                                         : val > col->bestVal) {
                        col->bestVal = val;
                    }
                }
            }
        }
        numActive++;
    }

    // Color cells that match the best value.
    for (i = 0; i < numActive; i++) {
        tvt_statType_t stat;

        if (cols[i].allSame) {
            continue;
        }

        stat = g_tvt_statsCols[cols[i].colIdx].stat;

        for (j = 0; j < numGroups; j++) {
            for (k = 0; k < groups[j].count; k++) {
                if (groups[j].players[k].vals[stat] == cols[i].bestVal) {
                    TvT_Table_SetCellColor(&groups[j].table->rows[k], i + colOffset, S_COLOR_GREEN);
                }
            }
        }
    }
}

static int G_TvT_Stats_SortByScore(const void *a, const void *b) {
    const tvt_EndGamePlayer_t *pa = (const tvt_EndGamePlayer_t *)a;
    const tvt_EndGamePlayer_t *pb = (const tvt_EndGamePlayer_t *)b;
    return pb->vals[STAT_SCORE] - pa->vals[STAT_SCORE];
}

static table_t *G_TvT_Stats_BuildTable(const char *teamName, const char *teamColor,
                                       tvt_EndGamePlayer_t *players, int count) {
    int         i, j;
    int         totals[STAT_COUNT];
    int         colOffset;
    table_t    *t;
    tableRow_t *row;

    qsort(players, count, sizeof(tvt_EndGamePlayer_t), G_TvT_Stats_SortByScore);

    t = TvT_Table_Create();
    TvT_Table_SetBorder(t, qfalse);
    if (teamName) {
        TvT_Table_AddCol(t, "TEAM", ALIGN_LEFT);
    }
    TvT_Table_AddCol(t, "Player", ALIGN_LEFT);
    colOffset = teamName ? 2 : 1;
    for (j = 0; j < STATS_NUM_COLS; j++) {
        if (!G_TvT_Stats_ColActive(j)) {
            continue;
        }
        TvT_Table_AddCol(t, g_tvt_statsCols[j].name, ALIGN_RIGHT);
    }

    memset(totals, 0, sizeof(totals));

    for (i = 0; i < count; i++) {
        gclient_t *cl = &level.clients[players[i].clientNum];
        int        displayCol = colOffset;

        row = TvT_Table_AddRow(t);
        if (teamName) {
            TvT_Table_SetCell(t, row, 0, va("%s%s", teamColor, teamName));
            TvT_Table_SetCell(t, row, 1, cl->pers.netname);
        }
        else {
            TvT_Table_SetCell(t, row, 0, cl->pers.netname);
        }
        for (j = 0; j < STATS_NUM_COLS; j++) {
            tvt_statType_t stat = g_tvt_statsCols[j].stat;
            if (!G_TvT_Stats_ColActive(j)) {
                continue;
            }
            TvT_Table_SetCell(t, row, displayCol++, va("%d", players[i].vals[stat]));
            totals[stat] += players[i].vals[stat];
        }
    }

    if (teamName) {
        int displayCol = colOffset;

        row = TvT_Table_AddRow(t);
        TvT_Table_SetRowSep(row, qtrue);
        TvT_Table_SetCell(t, row, 0, va("%s%s", teamColor, teamName));
        TvT_Table_SetCell(t, row, 1, "Totals");
        for (j = 0; j < STATS_NUM_COLS; j++) {
            if (!G_TvT_Stats_ColActive(j)) {
                continue;
            }
            TvT_Table_SetCell(t, row, displayCol++, va("%d", totals[g_tvt_statsCols[j].stat]));
        }
    }

    return t;
}

static char *G_TvT_Stats_WrapOutput(char *a, char *b) {
    int   aLen = a ? strlen(a) : 0;
    int   bLen = b ? strlen(b) : 0;
    char *out = malloc(aLen + bLen + 4);
    char *p;

    if (!out) {
        free(a);
        free(b);
        return NULL;
    }

    p = out;
    *p++ = '\n';
    if (a) {
        memcpy(p, a, aLen);
        p += aLen;
    }
    if (a && b) {
        *p++ = '\n';
    }
    if (b) {
        memcpy(p, b, bLen);
        p += bLen;
    }
    *p++ = '\n';
    *p = '\0';

    free(a);
    free(b);
    return out;
}

static void G_TvT_Stats_Print(const char *text) {
    int i;

    for (i = 0; i < level.maxclients; i++) {
        if (level.clients[i].pers.connected == CON_CONNECTED) {
            G_TvT_LongPrint(i, text);
        }
    }
}

qboolean G_TvT_Cmd_Stats(gentity_t *ent) {
    tvt_EndGamePlayer_t players[MAX_CLIENTS];
    int                 numPlayers;
    table_t            *t;
    tvt_statsGroup_t    group;

    numPlayers = G_TvT_Stats_GatherPlayers(players, TEAM_FREE);

    if (!numPlayers) {
        G_TvT_Printf(TVT_ENT_TO_CN(ent), "No active players on the server.");
        return qtrue;
    }

    t = G_TvT_Stats_BuildTable(NULL, NULL, players, numPlayers);

    group.players = players;
    group.table = t;
    group.count = numPlayers;
    group.team = TEAM_FREE;
    G_TvT_Stats_Highlight(&group, 1, 1);

    G_TvT_TablePrint(t, TVT_ENT_TO_CN(ent));
    TvT_Table_Destroy(t);

    return qtrue;
}

static void G_TvT_Stats_LogToFile(JSON_t *match, qtime_t *time);

static void G_TvT_Stats_Publish(tvt_statsGroup_t *groups, int numGroups) {
    qtime_t time;
    JSON_t *match;

    trap_RealTime(&time);
    match = G_TvT_Stats_BuildMatchJSON(groups, numGroups, &time);
    if (!match) {
        return;
    }

    G_TvT_Subs_SendStats(match);
    G_TvT_Stats_LogToFile(match, &time);
}

static void G_TvT_Stats_EndGameFFA(void) {
    tvt_EndGamePlayer_t players[MAX_CLIENTS];
    int                 numPlayers;
    table_t            *t;
    tvt_statsGroup_t    group;
    char               *combined;

    numPlayers = G_TvT_Stats_GatherPlayers(players, TEAM_FREE);

    if (!numPlayers) {
        return;
    }

    t = G_TvT_Stats_BuildTable(NULL, NULL, players, numPlayers);

    group.players = players;
    group.table = t;
    group.count = numPlayers;
    group.team = TEAM_FREE;
    G_TvT_Stats_Highlight(&group, 1, 1);

    combined = G_TvT_Stats_WrapOutput(TvT_Table_ToString(t), NULL);
    TvT_Table_Destroy(t);

    if (combined) {
        G_TvT_Stats_Print(combined);
        free(combined);
    }

    G_TvT_Stats_Publish(&group, 1);
}

static void G_TvT_Stats_EndGameTeam(void) {
    static const team_t teams[] = {TEAM_RED, TEAM_BLUE};
    tvt_EndGamePlayer_t teamPlayers[2][MAX_CLIENTS];
    tvt_statsGroup_t    groups[2];
    int                 numGroups = 0;
    char               *combined;
    int                 t;

    for (t = 0; t < 2; t++) {
        int count = G_TvT_Stats_GatherPlayers(teamPlayers[t], teams[t]);
        if (count) {
            groups[numGroups].players = teamPlayers[t];
            groups[numGroups].table = G_TvT_Stats_BuildTable(TeamName(teams[t]), TeamColorString(teams[t]),
                                                             teamPlayers[t], count);
            groups[numGroups].count = count;
            groups[numGroups].team = teams[t];
            numGroups++;
        }
    }

    if (!numGroups) {
        return;
    }

    G_TvT_Stats_Highlight(groups, numGroups, 2);

    if (numGroups == 2) {
        TvT_Table_SyncWidths(groups[0].table, groups[1].table);
    }

    combined = G_TvT_Stats_WrapOutput(
        TvT_Table_ToString(groups[0].table),
        numGroups == 2 ? TvT_Table_ToString(groups[1].table) : NULL);

    for (t = 0; t < numGroups; t++) {
        TvT_Table_Destroy(groups[t].table);
    }

    if (combined) {
        G_TvT_Stats_Print(combined);
        free(combined);
    }

    G_TvT_Stats_Publish(groups, numGroups);
}

static const char *g_tvt_gametypeNames[] = {"ffa", "holocron", "jedimaster", "duel", "single", "team", "saga", "ctf", "cty"};

static JSON_t *G_TvT_Stats_PlayerToJSON(tvt_EndGamePlayer_t *player) {
    gclient_t *cl = &level.clients[player->clientNum];
    JSON_t    *p  = TvT_JSON_CreateObject();
    char       cleanName[MAX_NETNAME];

    Q_strncpyz(cleanName, cl->pers.netname, sizeof(cleanName));
    Q_CleanStr(cleanName, (qboolean)(jk2startversion == VERSION_1_02));

    TvT_JSON_AddItemToObject(p, "name", TvT_JSON_CreateString(cleanName));
    TvT_JSON_AddItemToObject(p, "score", TvT_JSON_CreateNumber(player->vals[STAT_SCORE]));
    TvT_JSON_AddItemToObject(p, "kills", TvT_JSON_CreateNumber(player->vals[STAT_KILLS]));
    TvT_JSON_AddItemToObject(p, "deaths", TvT_JSON_CreateNumber(player->vals[STAT_DEATHS]));
    TvT_JSON_AddItemToObject(p, "suicides", TvT_JSON_CreateNumber(player->vals[STAT_SUICIDES]));
    TvT_JSON_AddItemToObject(p, "dmgGiven", TvT_JSON_CreateNumber(player->vals[STAT_DMG_GIVEN]));
    TvT_JSON_AddItemToObject(p, "dmgReceived", TvT_JSON_CreateNumber(player->vals[STAT_DMG_RECV]));

    if (g_gametype.integer >= GT_TEAM) {
        TvT_JSON_AddItemToObject(p, "teamKills", TvT_JSON_CreateNumber(player->vals[STAT_TEAMKILLS]));
        TvT_JSON_AddItemToObject(p, "teamDmg", TvT_JSON_CreateNumber(player->vals[STAT_TEAM_DMG]));
    }

    return p;
}

static JSON_t *G_TvT_Stats_BuildMatchJSON(tvt_statsGroup_t *groups, int numGroups, qtime_t *time) {
    JSON_t     *match = TvT_JSON_CreateObject();
    char        timestamp[64];
    char        mapname[MAX_QPATH];
    const char *gtName;

    Com_sprintf(timestamp, sizeof(timestamp), "%02d-%02d-%04d %02d:%02d:%02d",
                time->tm_mday, time->tm_mon + 1, time->tm_year + 1900,
                time->tm_hour, time->tm_min, time->tm_sec);

    trap_Cvar_VariableStringBuffer("mapname", mapname, sizeof(mapname));

    gtName = (g_gametype.integer >= 0 && g_gametype.integer < GT_MAX_GAME_TYPE)
                 ? g_tvt_gametypeNames[g_gametype.integer]
                 : "unknown";

    TvT_JSON_AddItemToObject(match, "timestamp", TvT_JSON_CreateString(timestamp));
    TvT_JSON_AddItemToObject(match, "map", TvT_JSON_CreateString(mapname));
    TvT_JSON_AddItemToObject(match, "gametype", TvT_JSON_CreateString(gtName));
    TvT_JSON_AddItemToObject(match, "matchMode", TvT_JSON_CreateBool((qboolean)tvt_matchMode.integer));

    if (g_gametype.integer >= GT_TEAM) {
        JSON_t *teamsObj = TvT_JSON_CreateObject();
        int     g;

        for (g = 0; g < numGroups; g++) {
            JSON_t *teamObj    = TvT_JSON_CreateObject();
            JSON_t *playersArr = TvT_JSON_CreateArray();
            char    teamKey[12];
            int     i;

            Q_strncpyz(teamKey, TeamName(groups[g].team), sizeof(teamKey));
            Q_strlwr(teamKey);

            TvT_JSON_AddItemToObject(teamObj, "score", TvT_JSON_CreateNumber(level.teamScores[groups[g].team]));

            for (i = 0; i < groups[g].count; i++) {
                TvT_JSON_AddItemToArray(playersArr, G_TvT_Stats_PlayerToJSON(&groups[g].players[i]));
            }

            TvT_JSON_AddItemToObject(teamObj, "players", playersArr);
            TvT_JSON_AddItemToObject(teamsObj, teamKey, teamObj);
        }

        TvT_JSON_AddItemToObject(match, "teams", teamsObj);
    }
    else {
        JSON_t *playersArr = TvT_JSON_CreateArray();
        int     i;

        for (i = 0; i < groups[0].count; i++) {
            TvT_JSON_AddItemToArray(playersArr, G_TvT_Stats_PlayerToJSON(&groups[0].players[i]));
        }

        TvT_JSON_AddItemToObject(match, "players", playersArr);
    }

    return match;
}

static void G_TvT_Stats_LogToFile(JSON_t *match, qtime_t *time) {
    JSON_t       *root    = NULL;
    JSON_t       *matches;
    char         *serialized;
    fileHandle_t  f;
    char          filepath[MAX_QPATH];
    int           fileLen;

    Com_sprintf(filepath, sizeof(filepath), "%s/%02d-%02d-%04d.json",
                tvt_matchMode.integer ? TVT_MATCH_LOG_DIR : TVT_CASUAL_LOG_DIR,
                time->tm_mday, time->tm_mon + 1, time->tm_year + 1900);

    fileLen = trap_FS_FOpenFile(filepath, &f, FS_READ);
    if (fileLen > 0) {
        char *buf = malloc(fileLen + 1);
        if (buf) {
            trap_FS_Read(buf, fileLen, f);
            buf[fileLen] = '\0';
            trap_FS_FCloseFile(f);

            root = TvT_JSON_Deserialize(buf, fileLen, NULL);
            free(buf);
        }
        else {
            trap_FS_FCloseFile(f);
        }
    }
    else if (f) {
        trap_FS_FCloseFile(f);
    }

    if (!root || root->type != TYPE_OBJECT) {
        TvT_JSON_FreeValue(root);
        root = TvT_JSON_CreateObject();
    }

    matches = TvT_JSON_GetObjectItem(root, "matches");
    if (!matches) {
        matches = TvT_JSON_CreateArray();
        TvT_JSON_AddItemToObject(root, "matches", matches);
    }

    TvT_JSON_AddItemToArray(matches, match);

    serialized = TvT_JSON_Serialize(root, qtrue, NULL);
    TvT_JSON_FreeValue(root);

    if (!serialized) {
        return;
    }

    fileLen = trap_FS_FOpenFile(filepath, &f, FS_WRITE);
    if (f) {
        trap_FS_Write(serialized, strlen(serialized), f);
        trap_FS_FCloseFile(f);
    }

    free(serialized);
}

void G_TvT_Stats_EndGame(void) {
    level.tvt.match.matchInProgress = qfalse;

    if (g_gametype.integer >= GT_TEAM) {
        G_TvT_Stats_EndGameTeam();
    }
    else {
        G_TvT_Stats_EndGameFFA();
    }
}
