#include "../g_local.h"

void G_TvT_SyncReadyMask(void) {
    int i;

    if (!level.clients) {
        return;
    }

    for (i = 0; i < g_maxclients.integer; i++) {
        if (level.clients[i].pers.connected == CON_CONNECTED) {
            level.clients[i].ps.stats[STAT_CLIENTS_READY] = level.tvt.match.readyMask;
        }
    }
}

qboolean G_TvT_CheckReadyUp(void) {
    int        i, ready, notReady, redCount, blueCount;
    gclient_t *cl;

    if (!tvt_matchMode.integer) {
        return qfalse;
    }

    if (level.tvt.match.matchInProgress || level.intermissiontime) {
        return qfalse;
    }

    ready = 0;
    notReady = 0;
    redCount = 0;
    blueCount = 0;
    for (i = 0; i < g_maxclients.integer; i++) {
        cl = level.clients + i;
        if (cl->pers.connected != CON_CONNECTED) {
            continue;
        }
        if (g_entities[i].r.svFlags & SVF_BOT) {
            continue;
        }
        if (cl->sess.sessionTeam == TEAM_RED) {
            redCount++;
        }
        else if (cl->sess.sessionTeam == TEAM_BLUE) {
            blueCount++;
        }
        else {
            continue;
        }
        if (i < 16 && (level.tvt.match.readyMask & (1 << i))) {
            ready++;
        }
        else {
            notReady++;
        }
    }

    if (level.tvt.match.restartPending) {
        if (notReady > 0) {
            level.tvt.match.restartPending = qfalse;
            trap_SetConfigstring(CS_WARMUP, va("%i", -1));
        }
        else if (level.time >= level.tvt.match.restartTime + 5000) {
            level.tvt.match.readyMask = 0;
            level.tvt.match.restartPending = qfalse;
            level.tvt.match.matchInProgress = qtrue;
            G_TvT_SyncReadyMask();
            trap_SetConfigstring(CS_WARMUP, "");
            trap_Cvar_Set("g_restarted", "1");
            trap_SendConsoleCommand(EXEC_APPEND, "map_restart 0\n");
            level.restarted = qtrue;
        }
        return qtrue;
    }

    if (ready > 0 && notReady == 0 && redCount > 0 && blueCount > 0) {
        level.tvt.match.restartPending = qtrue;
        level.tvt.match.restartTime = level.time;
        trap_SetConfigstring(CS_WARMUP, va("%i", level.time + 5000));
        trap_SendServerCommand(-1, "cp \"All players ready!\n\"");
    }
    else if (redCount + blueCount > 0) {
        trap_SetConfigstring(CS_WARMUP, va("%i", -1));
    }

    return qtrue;
}

void G_TvT_RegisterCachedTable(table_t **tablePtr) {
    tvt_CachedTable_t *node;

    node = malloc(sizeof(tvt_CachedTable_t));
    if (!node) {
        Com_Error(ERR_FATAL, "G_TvT_RegisterCachedTable: out of memory");
    }
    node->table = tablePtr;
    node->next = level.tvt.cachedTables;
    level.tvt.cachedTables = node;
}

void G_TvT_UpdateCachedTables(void) {
    tvt_CachedTable_t *node;

    for (node = level.tvt.cachedTables; node; node = node->next) {
        if (*node->table && !(*node->table)->accentColorExplicit) {
            (*node->table)->accentColor = tvt_defaultAccentColor;
        }
    }
}

void G_TvT_Init(void) {
    G_TvT_RegisterCvars();
    G_TvT_Vote_Init();
    G_TvT_Subs_Load();
}

void G_TvT_Shutdown(void) {
    G_TvT_Subs_Save();
}
