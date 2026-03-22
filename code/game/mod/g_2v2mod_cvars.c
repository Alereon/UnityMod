#include "../g_local.h"

#define TVT_CVAR(name, defaultValue, description, flags, trackChange, validate, update, votable, voteAlias) vmCvar_t name;
TVT_CVAR_LIST
#undef TVT_CVAR

#define TVT_CVAR(name, defaultValue, description, flags, trackChange, validate, update, votable, voteAlias) \
    {&name, #name, defaultValue, description, flags, 0, trackChange, validate, update, votable, voteAlias},
static tvt_Cvar_t tvtCvarTable[] = {
    TVT_CVAR_LIST{NULL, NULL, NULL, NULL, 0, 0, qfalse, NULL, NULL, qfalse, NULL}};
#undef TVT_CVAR

qboolean G_TvT_ValidateBool(const char *value) {
    return (!strcmp(value, "0") || !strcmp(value, "1"));
}

qboolean G_TvT_ValidateIntPair(const char *value) {
    int i;

    TvT_TokenizeString(value);

    if (TvT_Argc() != 2) {
        return qfalse;
    }
    for (i = 0; i < TvT_Argc(); i++) {
        if (!G_TvT_IsNumericString(TvT_Argv(i))) {
            return qfalse;
        }
    }
    return qtrue;
}

void G_TvT_RegisterCvars(void) {
    tvt_Cvar_t *cv;

    for (cv = tvtCvarTable; cv->cvarName; cv++) {
        trap_Cvar_Register(cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags);
        if (cv->vmCvar) {
            cv->modificationCount = cv->vmCvar->modificationCount;
        }
        if (cv->onChanged) {
            cv->onChanged();
        }
    }
}

void G_TvT_UpdateCvars(void) {
    tvt_Cvar_t *cv;

    for (cv = tvtCvarTable; cv->cvarName; cv++) {
        if (cv->vmCvar) {
            trap_Cvar_Update(cv->vmCvar);

            if (cv->modificationCount != cv->vmCvar->modificationCount) {
                cv->modificationCount = cv->vmCvar->modificationCount;

                if (cv->validate && !cv->validate(cv->vmCvar->string)) {
                    G_Printf("%s: invalid value '%s^7', reverting\n", cv->cvarName, cv->vmCvar->string);
                    trap_Cvar_Set(cv->cvarName, cv->defaultString);
                    trap_Cvar_Update(cv->vmCvar);
                    cv->modificationCount = cv->vmCvar->modificationCount;
                    continue;
                }

                if (cv->onChanged && !cv->onChanged()) {
                    trap_Cvar_Update(cv->vmCvar);
                    cv->modificationCount = cv->vmCvar->modificationCount;
                    continue;
                }

                if (cv->trackChange) {
                    trap_SendServerCommand(-1,
                                           va("print \"Server: %s changed to %s^7\n\"", cv->cvarName, cv->vmCvar->string));
                }
            }
        }
    }
}

const tvt_Cvar_t *G_TvT_GetCvarTable(void) {
    return tvtCvarTable;
}

const tvt_Cvar_t *G_TvT_FindCvar(const char *name) {
    tvt_Cvar_t *cv;

    for (cv = tvtCvarTable; cv->cvarName; cv++) {
        if (Q_stricmp(cv->cvarName, name) == 0) {
            return cv;
        }
    }
    return NULL;
}

qboolean G_TvT_ValidateColor(const char *value) {
    if (strlen(value) != 1) {
        return qfalse;
    }
    if (jk2version == VERSION_1_02) {
        return (value[0] != Q_COLOR_ESCAPE);
    }
    return (value[0] >= '0' && value[0] <= '7');
}

qboolean G_TvT_UpdateColor(void) {
    level.tvt.colorChar = tvt_color.string[0];
    tvt_defaultAccentColor[0] = '^';
    tvt_defaultAccentColor[1] = level.tvt.colorChar;
    tvt_defaultAccentColor[2] = '\0';
    trap_Cvar_Set("gamename", va("^%c/^7 2V2MOD ^%c/", level.tvt.colorChar, level.tvt.colorChar));
    G_TvT_UpdateCachedTables();
    return qtrue;
}

qboolean G_TvT_UpdateSpawnArmor(void) {
    tvt_ModState_t *tvt = &level.tvt;
    int             armor[2];

    TvT_TokenizeString(tvt_spawnArmor.string);

    armor[TVT_SPAWN] = Com_Clampi(0, 999, atoi(TvT_Argv(0)));
    armor[TVT_FIRST_SPAWN] = Com_Clampi(0, 999, atoi(TvT_Argv(1)));

    tvt->spawnArmor[TVT_SPAWN] = armor[TVT_SPAWN];
    tvt->spawnArmor[TVT_FIRST_SPAWN] = armor[TVT_FIRST_SPAWN];
    return qtrue;
}

qboolean G_TvT_UpdateSpawnItems(void) {
    tvt_ModState_t *tvt = &level.tvt;
    int             items[2];
    int             validItemMask = 0;
    int             i;

    TvT_TokenizeString(tvt_spawnItems.string);

    items[TVT_SPAWN] = atoi(TvT_Argv(0));
    items[TVT_FIRST_SPAWN] = atoi(TvT_Argv(1));

    for (i = HI_SEEKER; i < HI_NUM_HOLDABLE; i++) {
        validItemMask |= (1 << i);
    }
    if (items[TVT_SPAWN] & ~validItemMask || items[TVT_FIRST_SPAWN] & ~validItemMask) {
        G_Printf("tvt_spawnItems: invalid item bits (valid mask: %d)\n", validItemMask);
        trap_Cvar_Set("tvt_spawnItems", va("%d %d", tvt->spawnItems[TVT_SPAWN], tvt->spawnItems[TVT_FIRST_SPAWN]));
        return qfalse;
    }

    tvt->spawnItems[TVT_SPAWN] = items[TVT_SPAWN];
    tvt->spawnItems[TVT_FIRST_SPAWN] = items[TVT_FIRST_SPAWN];
    return qtrue;
}

qboolean G_TvT_ValidatePhysicsFps(const char *value) {
    int fps = atoi(value);

    return (fps == 0 || (fps >= 1 && fps <= 333));
}

qboolean G_TvT_UpdatePhysicsFps(void) {
    int fps = tvt_physicsFps.integer;

    if (fps == 0) {
        level.tvt.physicsMsec = 0;
    }
    else {
        level.tvt.physicsMsec = 1000 / fps;
    }
    return qtrue;
}

qboolean G_TvT_UpdateMatchMode(void) {
    if (!tvt_matchMode.integer) {
        level.tvt.match.matchInProgress = qfalse;
        level.tvt.match.restartPending = qfalse;
        level.tvt.match.readyMask = 0;
        G_TvT_SyncReadyMask();
        trap_SetConfigstring(CS_WARMUP, "");
    }
    return qtrue;
}
