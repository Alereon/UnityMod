#include "../g_local.h"

static tvt_VoteItem_t *tvt_voteItems;
static int             tvt_voteItemCount = 0;
static int             tvt_voteItemCap;

static void G_TvT_AddVoteItem(const char *name, const tvt_Cmd_t *cmd, const tvt_Cvar_t *cvar) {
    if (!tvt_voteItems) {
        tvt_voteItems = malloc(sizeof(tvt_VoteItem_t) * 8);
        if (!tvt_voteItems) {
            Com_Error(ERR_FATAL, "G_TvT_AddVoteItem malloc: out of memory");
        }
    }

    if (tvt_voteItemCount >= tvt_voteItemCap) {
        tvt_voteItemCap = tvt_voteItemCap ? tvt_voteItemCap * 2 : 8;
        tvt_voteItems = realloc(tvt_voteItems, sizeof(tvt_VoteItem_t) * tvt_voteItemCap);
        if (!tvt_voteItems) {
            Com_Error(ERR_FATAL, "G_TvT_AddVoteItem realloc: out of memory");
        }
    }
    tvt_voteItems[tvt_voteItemCount].name = name;
    tvt_voteItems[tvt_voteItemCount].cmd = cmd;
    tvt_voteItems[tvt_voteItemCount].cvar = cvar;
    tvt_voteItemCount++;
}

void G_TvT_Vote_Init(void) {
    const tvt_Cmd_t  *cmd;
    const tvt_Cvar_t *cv;

    tvt_voteItemCount = 0;

    for (cmd = G_TvT_GetCmdTable(); cmd->name; cmd++) {
        if (cmd->votable) {
            G_TvT_AddVoteItem(cmd->name, cmd, NULL);
        }
    }

    for (cv = G_TvT_GetCvarTable(); cv->cvarName; cv++) {
        if (cv->votable) {
            G_TvT_AddVoteItem(cv->cvarName, NULL, cv);
        }
    }

    G_Printf("Vote system: %d voteable items registered\n", tvt_voteItemCount);
}

static tvt_VoteItem_t *G_TvT_FindVoteItem(const char *name) {
    int         i;
    const char *alias;

    for (i = 0; i < tvt_voteItemCount; i++) {
        if (!Q_stricmp(name, tvt_voteItems[i].name)) {
            return &tvt_voteItems[i];
        }
        alias = tvt_voteItems[i].cmd ? tvt_voteItems[i].cmd->voteAlias : tvt_voteItems[i].cvar ? tvt_voteItems[i].cvar->voteAlias
                                                                                               : NULL;
        if (alias && !Q_stricmp(name, alias)) {
            return &tvt_voteItems[i];
        }
    }
    return NULL;
}

int G_TvT_CallVote(gentity_t *ent, const char *arg1, const char *args) {
    int             cn = ent - g_entities;
    tvt_VoteItem_t *item;

    item = G_TvT_FindVoteItem(arg1);
    if (!item) {
        return 0;
    }

    if (item->cmd) {
        if (args[0]) {
            G_TvT_Printf(cn, "'%s' does not take arguments.", item->name);
            return -1;
        }
        if (item->cmd->validate && !item->cmd->validate(ent)) {
            return -1;
        }
        Com_sprintf(level.voteString, sizeof(level.voteString), "%s", item->name);
        Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "%s", item->name);
        return 1;
    }

    if (item->cvar) {
        if (!args[0]) {
            G_TvT_Printf(cn, "Usage: callvote %s <value>\n%s\nCurrent: %s (default: %s)\n",
                                item->name, item->cvar->description, item->cvar->vmCvar->string, item->cvar->defaultString);
            return -1;
        }
        if (strlen(args) >= MAX_CVAR_VALUE_STRING) {
            G_TvT_Printf(cn, "The specified value is too long.\n");
            return -1;
        }
        if (item->cvar->validate && !item->cvar->validate(args)) {
            G_TvT_Printf(cn, "Invalid value for %s.\n", item->name);
            return -1;
        }
        Com_sprintf(level.voteString, sizeof(level.voteString), "%s \"%s\"", item->name, args);
        Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "%s %s", item->name, args);
        return 1;
    }

    return 0;
}

qboolean G_TvT_Cmd_VoteList(gentity_t *ent) {
    int         cn = TVT_ENT_TO_CN(ent);
    table_t    *t;
    tableRow_t *row;
    int         i;
    char        search[MAX_TOKEN_CHARS];
    const char *alias;

    t = TvT_Table_Create();
    TvT_Table_AddCol(t, "Vote", ALIGN_LEFT);
    TvT_Table_AddCol(t, "Alias", ALIGN_LEFT);

    for (i = 0; i < tvt_voteItemCount; i++) {
        tvt_VoteItem_t *item = &tvt_voteItems[i];

        row = TvT_Table_AddRow(t);
        alias = item->cmd ? item->cmd->voteAlias : item->cvar ? item->cvar->voteAlias
                                                              : NULL;

        if (item->cmd) {
            TvT_Table_SetCell(t, row, 0, item->name);
        }
        else if (item->cvar) {
            TvT_Table_SetCell(t, row, 0, va("%s <value>", item->name));
        }
        TvT_Table_SetCell(t, row, 1, alias ? alias : "");
    }

    if (trap_Argc() > 2) {
        tvt_FilterCtx_t filter;
        trap_Argv(2, search, sizeof(search));
        filter.colName = "Vote";
        filter.search = search;
        TvT_Table_Filter(t, TvT_Table_FilterSubstring, &filter);
    }

    TvT_Table_Sort(t, "Vote", qtrue);
    G_TvT_TablePrint(t, cn);
    TvT_Table_Destroy(t);

    return qtrue;
}
