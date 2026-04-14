#include "../g_local.h"

static qboolean G_TvT_Cmd_MemStats(gentity_t *ent) {
    tvt_MemStats_t s;
    int            cn = TVT_ENT_TO_CN(ent);
    table_t       *t;
    tableRow_t    *row;

    TvT_Mem_GetStats(&s);

    t = TvT_Table_Create();
    TvT_Table_AddCol(t, "Metric", ALIGN_LEFT);
    TvT_Table_AddCol(t, "Value", ALIGN_RIGHT);

    row = TvT_Table_AddRow(t);
    TvT_Table_SetCell(t, row, 0, "Pool size");
    TvT_Table_SetCell(t, row, 1, va("%d bytes (%d MB)", (int)s.pool_size, (int)(s.pool_size / (1024 * 1024))));

    row = TvT_Table_AddRow(t);
    TvT_Table_SetCell(t, row, 0, "Used");
    TvT_Table_SetCell(t, row, 1, va("%d blocks, %d bytes (%d KB)", s.used_blocks, (int)s.used_bytes, (int)(s.used_bytes / 1024)));

    row = TvT_Table_AddRow(t);
    TvT_Table_SetCell(t, row, 0, "  Largest used");
    TvT_Table_SetCell(t, row, 1, va("%d bytes", (int)s.used_largest));

    row = TvT_Table_AddRow(t);
    TvT_Table_SetCell(t, row, 0, "  Overhead");
    TvT_Table_SetCell(t, row, 1, va("%d bytes (%d per block)", (int)s.used_overhead, (int)BLOCK_OVERHEAD));

    row = TvT_Table_AddRow(t);
    TvT_Table_SetCell(t, row, 0, "Free");
    TvT_Table_SetCell(t, row, 1, va("%d blocks, %d bytes (%d MB)", s.free_blocks, (int)s.free_bytes, (int)(s.free_bytes / (1024 * 1024))));

    row = TvT_Table_AddRow(t);
    TvT_Table_SetCell(t, row, 0, "  Largest free");
    TvT_Table_SetCell(t, row, 1, va("%d bytes", (int)s.free_largest));

    if (s.free_blocks > 1) {
        row = TvT_Table_AddRow(t);
        TvT_Table_SetCell(t, row, 0, "Fragmentation");
        TvT_Table_SetCell(t, row, 1, va("%d free segments", s.free_blocks));
        TvT_Table_SetCellColor(row, 1, S_COLOR_RED);
    }

    G_TvT_TablePrint(t, cn);
    TvT_Table_Destroy(t);

    return qtrue;
}

static qboolean G_TvT_IsEligible(gclient_t *cl) {
    if (cl->pers.connected != CON_CONNECTED) {
        return qfalse;
    }
    if (cl->sess.sessionTeam == TEAM_RED || cl->sess.sessionTeam == TEAM_BLUE) {
        return qtrue;
    }
    return (cl->sess.sessionTeam == TEAM_SPECTATOR && cl->tvt.queued);
}

static int G_TvT_CompareQueueTime(const void *a, const void *b) {
    int ta = level.clients[*(const int *)a].tvt.queueTime;
    int tb = level.clients[*(const int *)b].tvt.queueTime;

    return ta - tb;
}

static int G_TvT_CollectPlayers(int *players, unsigned int *outOldRed, unsigned int *outOldBlue,
                                int *outPriority) {
    int          count = 0;
    int          back = MAX_CLIENTS;
    int          satOut = 0;
    unsigned int curRed = 0, curBlue = 0;
    int          totalSlots;
    int          i;

    for (i = 0; i < level.maxclients; i++) {
        if (!G_TvT_IsEligible(&level.clients[i])) {
            continue;
        }
        if (level.clients[i].sess.sessionTeam == TEAM_RED) {
            curRed |= (1u << i);
        }
        else if (level.clients[i].sess.sessionTeam == TEAM_BLUE) {
            curBlue |= (1u << i);
        }
        if (level.tvt.match.playedLastRound & (1u << i)) {
            players[--back] = i;
        }
        else {
            players[count++] = i;
        }
    }

    *outOldRed = curRed;
    *outOldBlue = curBlue;

    satOut = MAX_CLIENTS - back;
    totalSlots = tvt_teamSize.integer ? tvt_teamSize.integer * 2 : MAX_CLIENTS;

    // Merge both groups into a single contiguous array.
    memmove(players + count, players + back, satOut * sizeof(int));

    if (!tvt_specPrio.integer || count + satOut <= totalSlots) {
        // No priority: shuffle everyone together.
        count += satOut;
        *outPriority = count;
    }
    else {
        // Spectator priority: shuffle team slots from sat-out players,
        // then last-round players fill remaining slots / overflow to spec.
        qsort(players, count, sizeof(int), G_TvT_CompareQueueTime);
        *outPriority = count;
        count += satOut;
    }

    return count;
}

static qboolean G_TvT_Cmd_Shuffle(gentity_t *ent) {
    static int   firstTeam = 0;
    int          players[MAX_CLIENTS];
    unsigned int newRed, newBlue;
    unsigned int oldRed, oldBlue;
    int          teamSize, half, priority, attempts;
    int          count, i, cn;

    count = G_TvT_CollectPlayers(players, &oldRed, &oldBlue, &priority);

    teamSize = tvt_teamSize.integer ? tvt_teamSize.integer : MAX_CLIENTS;
    half = count / 2;
    if (half > teamSize) {
        half = teamSize;
    }

    for (attempts = 0; attempts < 10; attempts++) {
        if (!tvt_specPrio.integer) {
            G_TvT_FisherYatesShuffle(players, count);
        }
        else {
            G_TvT_FisherYatesShuffle(players, priority);
            G_TvT_FisherYatesShuffle(players + priority, count - priority);
        }

        newRed = newBlue = 0;
        for (i = 0; i < count; i++) {
            if (i < half) {
                newRed |= (1u << players[i]);
            }
            else if (i < half * 2) {
                newBlue |= (1u << players[i]);
            }
        }

        // Alternate which team gets the larger half on odd counts.
        if (firstTeam & 1) {
            unsigned int tmp = newRed;
            newRed = newBlue;
            newBlue = tmp;
        }

        if (newRed != oldRed && newRed != oldBlue) {
            break;
        }
    }

    for (i = 0; i < count; i++) {
        cn = players[i];

        if (newRed & (1u << cn)) {
            SetTeam(&g_entities[cn], "red", qtrue);
            level.clients[cn].tvt.queueTime = 0;
        }
        else if (newBlue & (1u << cn)) {
            SetTeam(&g_entities[cn], "blue", qtrue);
            level.clients[cn].tvt.queueTime = 0;
        }
        else {
            if (level.clients[cn].sess.sessionTeam != TEAM_SPECTATOR) {
                SetTeam(&g_entities[cn], "spectator", qtrue);
            }
            level.clients[cn].tvt.queued = qtrue;
            if (!level.clients[cn].tvt.queueTime) {
                level.clients[cn].tvt.queueTime = level.time;
            }
        }
    }

    level.tvt.match.playedLastRound = newRed | newBlue;
    firstTeam ^= 1;

    CheckTeamLeader(TEAM_RED);
    CheckTeamLeader(TEAM_BLUE);

    if (tvt_matchMode.integer) {
        level.tvt.match.matchInProgress = qfalse;
        level.tvt.match.restartPending = qfalse;
        level.tvt.match.readyMask = 0;
        G_TvT_SyncReadyMask();
        trap_SetConfigstring(CS_WARMUP, va("%i", -1));
        trap_SendServerCommand(-1, "cp \"Teams shuffled. Type ^2/ready^7 to start.\n\"");
    }
    else {
        trap_SendServerCommand(-1, "cp \"Teams have been shuffled.\n\"");
    }
    return qtrue;
}

static qboolean G_TvT_Cmd_ModCvars(gentity_t *ent) {
    int         cn = TVT_ENT_TO_CN(ent);
    const  tvt_Cvar_t *cv;
	table_t    *t;
    char        search[MAX_TOKEN_CHARS];

    t = TvT_Table_Create();
    TvT_Table_AddCol(t, "Name", ALIGN_LEFT);
    TvT_Table_AddCol(t, "Description", ALIGN_LEFT);
    TvT_Table_AddCol(t, "Default", ALIGN_LEFT);
    TvT_Table_AddCol(t, "Current", ALIGN_LEFT);

    for (cv = G_TvT_GetCvarTable(); cv->cvarName; cv++) {
        tableRow_t *row = TvT_Table_AddRow(t);

        TvT_Table_SetCell(t, row, 0, cv->cvarName);
        TvT_Table_SetCell(t, row, 1, cv->description);
        if (!Q_stricmp(cv->cvarName, "tvt_color")) {
            TvT_Table_SetCell(t, row, 2, va("^%sCOLOR", cv->defaultString));
            TvT_Table_SetCell(t, row, 3, va("^%sCOLOR", cv->vmCvar->string));
        }
        else {
            TvT_Table_SetCell(t, row, 2, cv->defaultString);
            TvT_Table_SetCell(t, row, 3, cv->vmCvar->string);
        }

        if (strcmp(cv->vmCvar->string, cv->defaultString)) {
            TvT_Table_SetCellColor(row, 3, S_COLOR_GREEN);
        }
    }

    if (trap_Argc() > 2) {
        tvt_FilterCtx_t filter;
        trap_Argv(2, search, sizeof(search));
        filter.colName = "Name";
        filter.search = search;
        TvT_Table_Filter(t, TvT_Table_FilterSubstring, &filter);
    }

    TvT_Table_Sort(t, "Name", qtrue);
    G_TvT_TablePrint(t, cn);
    TvT_Table_Destroy(t);

    return qtrue;
}

static qboolean G_TvT_Cmd_Queue(gentity_t *ent) {
    int        cn = TVT_ENT_TO_CN(ent);
    gclient_t *cl = ent->client;

    if (!cl)
        return qtrue;

    if (g_gametype.integer < GT_TEAM) {
        G_TvT_Printf(cn, "Queue command only works in team based gametypes.\n");
        return qtrue;
    }

    if (cl->sess.sessionTeam != TEAM_SPECTATOR) {
        G_TvT_Printf(cn, "You must be a spectator to use this command.\n");
        return qtrue;
    }

    cl->tvt.queued = !cl->tvt.queued;
    cl->tvt.queueTime = cl->tvt.queued ? level.time : 0;
    G_TvT_Printf(cn, "Queue status: %s^7\n", cl->tvt.queued ? "^2joined" : "^1left");

    return qtrue;
}

static qboolean G_TvT_Cmd_Players(gentity_t *ent) {
    int         cn = TVT_ENT_TO_CN(ent);
    table_t    *t;
    tableRow_t *row;
    int         i;

    t = TvT_Table_Create();
    TvT_Table_AddCol(t, "ID", ALIGN_RIGHT);
    TvT_Table_AddCol(t, "Name", ALIGN_LEFT);
    TvT_Table_AddCol(t, "Queue", ALIGN_LEFT);

    for (i = 0; i < level.maxclients; i++) {
        gclient_t *cl = &level.clients[i];

        if (cl->pers.connected != CON_CONNECTED) {
            continue;
        }

        row = TvT_Table_AddRow(t);
        TvT_Table_SetCell(t, row, 0, va("%d", i));
        TvT_Table_SetCell(t, row, 1, cl->pers.netname);
        TvT_Table_SetCell(t, row, 2, cl->tvt.queued ? "^2X" : "");
    }

    if (g_gametype.integer < GT_TEAM) {
        TvT_Table_HideCol(t, "Queue", qtrue);
    }

    G_TvT_TablePrint(t, cn);
    TvT_Table_Destroy(t);
    return qtrue;
}

static qboolean G_TvT_Cmd_Ready(gentity_t *ent) {
    int        cn = TVT_ENT_TO_CN(ent);
    gclient_t *cl = ent->client;
    int        i, ready, total;
    gclient_t *other;

    if (!cl) {
        return qtrue;
    }

    if (!tvt_matchMode.integer) {
        G_TvT_Printf(cn, "Match mode is not enabled.\n");
        return qtrue;
    }

    if (level.tvt.match.matchInProgress) {
        G_TvT_Printf(cn, "A match is already in progress.\n");
        return qtrue;
    }

    if (cl->sess.sessionTeam != TEAM_RED && cl->sess.sessionTeam != TEAM_BLUE) {
        G_TvT_Printf(cn, "You must be on a team to ready up.\n");
        return qtrue;
    }

    if (cn < 16) {
        level.tvt.match.readyMask ^= (1 << cn);
    }

    for (i = 0; i < g_maxclients.integer; i++) {
        other = level.clients + i;
        if (other->pers.connected == CON_CONNECTED) {
            other->ps.stats[STAT_CLIENTS_READY] = level.tvt.match.readyMask;
        }
    }

    ready = 0;
    total = 0;
    for (i = 0; i < g_maxclients.integer; i++) {
        other = level.clients + i;
        if (other->pers.connected != CON_CONNECTED) {
            continue;
        }
        if (g_entities[i].r.svFlags & SVF_BOT) {
            continue;
        }
        if (other->sess.sessionTeam != TEAM_RED && other->sess.sessionTeam != TEAM_BLUE) {
            continue;
        }
        total++;
        if (i < 16 && (level.tvt.match.readyMask & (1 << i))) {
            ready++;
        }
    }

    trap_SendServerCommand(-1, va("print \"%s ^7is %s ^7(%d/%d ready)\n\"",
                                  cl->pers.netname,
                                  (cn < 16 && (level.tvt.match.readyMask & (1 << cn))) ? "^2ready" : "^1not ready",
                                  ready, total));
    return qtrue;
}

static qboolean G_TvT_Cmd_Abort(gentity_t *ent) {
    level.tvt.match.matchInProgress = qfalse;
    level.tvt.match.restartPending = qfalse;
    level.tvt.match.readyMask = 0;
    G_TvT_SyncReadyMask();
    trap_SetConfigstring(CS_WARMUP, va("%i", -1));
    trap_SendServerCommand(-1, "cp \"Match aborted. Type ^2/ready^7 to start.\n\"");
    return qtrue;
}

static qboolean G_TvT_ValidateAbort(gentity_t *ent) {
    int cn = TVT_ENT_TO_CN(ent);

    if (!tvt_matchMode.integer) {
        G_TvT_Printf(cn, "Match mode is not enabled.\n");
        return qfalse;
    }

    if (!level.tvt.match.matchInProgress) {
        G_TvT_Printf(cn, "No match is in progress.\n");
        return qfalse;
    }

    return qtrue;
}

static qboolean G_TvT_Cmd_ListCommands(gentity_t *ent);

static qboolean G_TvT_ValidateShuffle(gentity_t *ent) {
    int cn = TVT_ENT_TO_CN(ent);
    int count, i;

    if (g_gametype.integer < GT_TEAM) {
        G_TvT_Printf(cn, "This command is only allowed in team based gametypes.\n");
        return qfalse;
    }

    count = 0;
    for (i = 0; i < level.maxclients; i++) {
        if (G_TvT_IsEligible(&level.clients[i])) {
            count++;
        }
    }

    if (count < 3) {
        G_TvT_Printf(cn, "Not enough players to shuffle.\n");
        return qfalse;
    }

    return qtrue;
}

static table_t *G_TvT_BuildCreditsTable(void) {
    static table_t *creditsTable = NULL;
    tableRow_t     *row;

    if (creditsTable) {
        return creditsTable;
    }

    creditsTable = TvT_Table_Create();
    TvT_Table_AddCol(creditsTable, "Contributor", ALIGN_LEFT);
    TvT_Table_AddCol(creditsTable, "Description", ALIGN_LEFT);

    row = TvT_Table_AddRow(creditsTable);
    TvT_Table_SetCell(creditsTable, row, 0, "TomArrow");
    TvT_Table_SetCell(creditsTable, row, 1, "tvt_specAllEnts cvar");

    G_TvT_RegisterCachedTable(&creditsTable);
    return creditsTable;
}

static qboolean G_TvT_Cmd_Credits(gentity_t *ent) {
    int      cn = TVT_ENT_TO_CN(ent);
    table_t *t  = G_TvT_BuildCreditsTable();

    G_TvT_Printf(cn, "^%c/^7 2V2MOD ^%c/^7\nAuthor: ^6/^7god^6/ ^7(Alereon)\nBuilt: " __DATE__ " " __TIME__ "\n", level.tvt.colorChar, level.tvt.colorChar);
    G_TvT_TablePrint(t, cn);

    return qtrue;
}

static qboolean G_TvT_Cmd_SubsApi(gentity_t *ent) {
    int               cn  = TVT_ENT_TO_CN(ent);
    const tvt_Cvar_t *ttl = G_TvT_FindCvar("tvt_subTtl");

    G_TvT_Printf(cn,
        "^%c--- Subscriber API ---^7\n"
        "External services can subscribe to receive endgame stats via\n"
        "UDP connectionless packets (OOB).\n"
        "\n"
        "^%cWire format:^7 \\xff\\xff\\xff\\xff mvapi <command>\n"
        "\n"
        "^%cCommands (client -> server):^7\n"
        "  subscribe    Register as a subscriber.\n"
        "  unsubscribe  Remove subscription.\n"
        "  ping         Refresh TTL, server replies with pong.\n"
        "\n"
        "^%cResponses (server -> client):^7\n"
        "  subscribed           Confirms subscription or TTL refresh.\n"
        "  pong subscribed      Reply to ping, subscription is active.\n"
        "  pong unsubscribed    Reply to ping, not subscribed.\n"
        "  <type> N M data  Chunked response, chunk N of M.\n"
        "                   Currently: ^3stats^7 (endgame JSON).\n"
        "\n"
        "Subscriptions expire after ^3tvt_subTtl^7 %d seconds (default: %s).\n"
        "Send subscribe or ping before expiry to refresh.\n"
        "Subscriptions persist across map restarts.\n",
        level.tvt.colorChar,
        level.tvt.colorChar,
        level.tvt.colorChar,
        level.tvt.colorChar,
        tvt_subTtl.integer,
        ttl ? ttl->defaultString : "120");

    return qtrue;
}

static const tvt_Cmd_t tvt_info_subcmds[] = {
    {"credits", "Show mod credits", "info credits", G_TvT_Cmd_Credits, NULL, NULL, CMD_CONTEXT_ALL, 0, 0, qfalse, NULL},
    {"api", "Show subscriber API documentation", "info api", G_TvT_Cmd_SubsApi, NULL, NULL, CMD_CONTEXT_ALL, 0, 0, qfalse, NULL},
    {"cvars", "Show mod cvar settings", "info cvars [filter]", G_TvT_Cmd_ModCvars, NULL, NULL, CMD_CONTEXT_ALL, 0, 1, qfalse, NULL},
    {"cmds", "List available commands", "info cmds [filter]", G_TvT_Cmd_ListCommands, NULL, NULL, CMD_CONTEXT_ALL, 0, 1, qfalse, NULL},
    {"votes", "Show voteable items", "info votes [filter]", G_TvT_Cmd_VoteList, NULL, NULL, CMD_CONTEXT_ALL, 0, 1, qfalse, NULL},
    {NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, qfalse, NULL}};

static const tvt_Cmd_t tvt_commands[] = {
    {"abort", "Abort the current match", "abort", G_TvT_Cmd_Abort, G_TvT_ValidateAbort, NULL, CMD_CONTEXT_SERVER, 0, 0, qtrue, NULL},
    {"info", "Show mod information", "info <cvars|cmds|votes>", NULL, NULL, tvt_info_subcmds, CMD_CONTEXT_ALL, 0, 0, qfalse, NULL},
    {"mem_stats", "Show memory pool statistics", "mem_stats", G_TvT_Cmd_MemStats, NULL, NULL, CMD_CONTEXT_SERVER, 0, 0, qfalse, NULL},
    {"shuffle", "Shuffle players between teams", "shuffle", G_TvT_Cmd_Shuffle, G_TvT_ValidateShuffle, NULL, CMD_CONTEXT_SERVER, 0, 0, qtrue, NULL},
    {"pstats", "Show player statistics", "pstats", G_TvT_Cmd_Stats, NULL, NULL, CMD_CONTEXT_ALL, 0, 0, qfalse, NULL},
    {"queue", "Toggle queue status", "queue", G_TvT_Cmd_Queue, NULL, NULL, CMD_CONTEXT_CLIENT, 0, 0, qfalse, NULL},
    {"ready", "Toggle ready status", "ready", G_TvT_Cmd_Ready, NULL, NULL, CMD_CONTEXT_CLIENT, 0, 0, qfalse, NULL},
    {"players", "Show player list and queue status", "players", G_TvT_Cmd_Players, NULL, NULL, CMD_CONTEXT_ALL, 0, 0, qfalse, NULL},
    {NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, qfalse, NULL}};

static void G_TvT_Cmd_ListSubCommands(int clientNum, const char *parentName,
                                      const tvt_Cmd_t *subs, cmdContext_t context) {
    const tvt_Cmd_t *s;
    table_t         *t;
    tableRow_t      *row;

    G_TvT_Printf(clientNum, "Available sub-commands for '%s':\n", parentName);

    t = TvT_Table_Create();
    TvT_Table_AddCol(t, "Sub-command", ALIGN_LEFT);
    TvT_Table_AddCol(t, "Description", ALIGN_LEFT);
    TvT_Table_AddCol(t, "Usage", ALIGN_LEFT);

    for (s = subs; s->name; s++) {
        if (s->context & context) {
            row = TvT_Table_AddRow(t);
            TvT_Table_SetCell(t, row, 0, s->name);
            TvT_Table_SetCell(t, row, 1, s->description);
            TvT_Table_SetCell(t, row, 2, s->usage);
        }
    }

    G_TvT_TablePrint(t, clientNum);
    TvT_Table_Destroy(t);
}

static qboolean G_TvT_FilterCmdTable(table_t *t, tableRow_t *row, void *ctx) {
    tvt_CmdFilterCtx_t *filter = (tvt_CmdFilterCtx_t *)ctx;
    int                 col = TvT_Table_FindCol(t, "Context");
    const char         *val;

    if (col >= 0) {
        val = row->cells[col].text;
        if (val && !(atoi(val) & filter->context)) {
            return qfalse;
        }
    }

    if (filter->search) {
        return TvT_Table_FilterSubstring(t, row, filter->search);
    }

    return qtrue;
}

static table_t *G_TvT_BuildCmdDisplayTable(void) {
    static table_t  *cmdTable = NULL;
    const tvt_Cmd_t *c;
    const tvt_Cmd_t *s;
    tableRow_t      *row;
    int              ctxCol;

    if (cmdTable) {
        return cmdTable;
    }

    cmdTable = TvT_Table_Create();
    TvT_Table_AddCol(cmdTable, "Command", ALIGN_LEFT);
    TvT_Table_AddCol(cmdTable, "Description", ALIGN_LEFT);
    TvT_Table_AddCol(cmdTable, "Usage", ALIGN_LEFT);
    TvT_Table_AddCol(cmdTable, "Context", ALIGN_LEFT);
    TvT_Table_HideCol(cmdTable, "Context", qtrue);

    ctxCol = TvT_Table_FindCol(cmdTable, "Context");

    for (c = tvt_commands; c->name; c++) {
        if (c->subCommands) {
            for (s = c->subCommands; s->name; s++) {
                row = TvT_Table_AddRow(cmdTable);
                TvT_Table_SetCell(cmdTable, row, 0, va("%s %s", c->name, s->name));
                TvT_Table_SetCell(cmdTable, row, 1, s->description);
                TvT_Table_SetCell(cmdTable, row, 2, s->usage);
                TvT_Table_SetCell(cmdTable, row, ctxCol, va("%d", s->context));
            }
        }
        else {
            row = TvT_Table_AddRow(cmdTable);
            TvT_Table_SetCell(cmdTable, row, 0, c->name);
            TvT_Table_SetCell(cmdTable, row, 1, c->description);
            TvT_Table_SetCell(cmdTable, row, 2, c->usage);
            TvT_Table_SetCell(cmdTable, row, ctxCol, va("%d", c->context));
        }
    }

    TvT_Table_Sort(cmdTable, "Command", qtrue);
    G_TvT_RegisterCachedTable(&cmdTable);
    return cmdTable;
}

static qboolean G_TvT_Cmd_ListCommands(gentity_t *ent) {
    int                cn = TVT_ENT_TO_CN(ent);
    table_t           *t = G_TvT_BuildCmdDisplayTable();
    tvt_CmdFilterCtx_t cmdFilter;
    tvt_FilterCtx_t    searchFilter;
    char               search[MAX_TOKEN_CHARS];

    cmdFilter.context = ent ? CMD_CONTEXT_CLIENT : CMD_CONTEXT_SERVER;
    cmdFilter.search = NULL;

    if (trap_Argc() > 2) {
        trap_Argv(2, search, sizeof(search));
        searchFilter.colName = "Command";
        searchFilter.search = search;
        cmdFilter.search = &searchFilter;
    }

    TvT_Table_Filter(t, G_TvT_FilterCmdTable, &cmdFilter);
    G_TvT_TablePrint(t, cn);
    TvT_Table_Filter(t, NULL, NULL);

    return qtrue;
}

static qboolean G_TvT_Cmd_Execute(gentity_t *ent, const char *cmd, cmdContext_t context) {
    const tvt_Cmd_t   *c;
    unsigned int argc;

    for (c = tvt_commands; c->name; c++) {
        if (Q_stricmp(cmd, c->name)) {
            continue;
        }

        if (!(c->context & context)) {
            if (context == CMD_CONTEXT_CLIENT) {
                G_TvT_Printf(TVT_ENT_TO_CN(ent), "Command '%s' can only be used by the server.\n", cmd);
            }
            else {
                G_TvT_Printf(TVT_ENT_TO_CN(ent), "Command '%s' can only be used by clients.\n", cmd);
            }
            return qtrue;
        }

        if (c->subCommands) {
            char             subcmd[MAX_TOKEN_CHARS];
            const tvt_Cmd_t *s;
            int              cn = TVT_ENT_TO_CN(ent);

            if (trap_Argc() < 2) {
                G_TvT_Cmd_ListSubCommands(cn, c->name, c->subCommands, context);
                return qtrue;
            }

            trap_Argv(1, subcmd, sizeof(subcmd));

            for (s = c->subCommands; s->name; s++) {
                if (Q_stricmp(subcmd, s->name)) {
                    continue;
                }

                if (!(s->context & context)) {
                    if (context == CMD_CONTEXT_CLIENT) {
                        G_TvT_Printf(cn, "Command '%s %s' can only be used by the server.\n", cmd, subcmd);
                    }
                    else {
                        G_TvT_Printf(cn, "Command '%s %s' can only be used by clients.\n", cmd, subcmd);
                    }
                    return qtrue;
                }

                argc = (unsigned int)(trap_Argc() - 2);

                if (argc < s->minArgs || (s->maxArgs > 0 && argc > s->maxArgs)) {
                    G_TvT_Printf(cn, "Usage: %s\n", s->usage);
                    return qtrue;
                }

                return s->execute(ent);
            }

            G_TvT_Printf(cn, "Unknown sub-command '%s'.\n", subcmd);
            G_TvT_Cmd_ListSubCommands(cn, c->name, c->subCommands, context);
            return qtrue;
        }

        argc = (unsigned int)(trap_Argc() - 1);

        if (argc < c->minArgs || (c->maxArgs > 0 && argc > c->maxArgs)) {
            G_TvT_Printf(TVT_ENT_TO_CN(ent), "Usage: %s\n", c->usage);
            return qtrue;
        }

        if (c->validate && !c->validate(ent)) {
            return qtrue;
        }

        return c->execute(ent);
    }

    return qfalse;
}

qboolean G_TvT_ClientCommand(gentity_t *ent, const char *cmd) {
    return G_TvT_Cmd_Execute(ent, cmd, CMD_CONTEXT_CLIENT);
}

qboolean G_TvT_ConsoleCommand(const char *cmd) {
    return G_TvT_Cmd_Execute(NULL, cmd, CMD_CONTEXT_SERVER);
}

const tvt_Cmd_t *G_TvT_GetCmdTable(void) {
    return tvt_commands;
}
