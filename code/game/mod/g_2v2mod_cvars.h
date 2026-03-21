#ifndef G_2V2MOD_CVARS_H
#define G_2V2MOD_CVARS_H

#include "../q_shared.h"

typedef struct {
    vmCvar_t   *vmCvar;
    char       *cvarName;
    char       *defaultString;
    const char *description;
    int         cvarFlags;
    int         modificationCount;
    qboolean    trackChange;
    qboolean (*validate)(const char *value);
    qboolean (*onChanged)(void);
    qboolean    votable;
    const char *voteAlias;
} tvt_Cvar_t;

qboolean G_TvT_ValidateBool(const char *value);
qboolean G_TvT_ValidateColor(const char *value);
qboolean G_TvT_ValidateIntPair(const char *value);
qboolean G_TvT_ValidatePhysicsFps(const char *value);
qboolean G_TvT_UpdateColor(void);
qboolean G_TvT_UpdateSpawnArmor(void);
qboolean G_TvT_UpdateSpawnItems(void);
qboolean G_TvT_UpdateMatchMode(void);
qboolean G_TvT_UpdatePhysicsFps(void);

#define TVT_CVAR_LIST                                                                                                                                                                                        \
    TVT_CVAR(tvt_allowBlackNames, "1", "Allow players to use the colour black in their names", CVAR_ARCHIVE, qtrue, G_TvT_ValidateBool, NULL, qtrue, "allowBlackNames")                                    \
    TVT_CVAR(tvt_uniqueNames, "1", "Append a numeric suffix to non unique player names", CVAR_ARCHIVE, qtrue, G_TvT_ValidateBool, NULL, qtrue, "uniqueNames")                                                \
    TVT_CVAR(tvt_forcePushItems, "1", "Allow force push and pull to move spawned items", CVAR_ARCHIVE, qtrue, G_TvT_ValidateBool, NULL, qtrue, "forcePushItems")                                              \
    TVT_CVAR(tvt_teamForceRules, "1", "Allow force push, pull and drain to affect teammates", CVAR_ARCHIVE, qtrue, G_TvT_ValidateBool, NULL, qtrue, "teamForceRules")                                        \
    TVT_CVAR(tvt_teamSuicideScoring, "1", "Teamkill or suicide awards +1 score to the opposing team instead of -1 score to self", CVAR_ARCHIVE, qtrue, G_TvT_ValidateBool, NULL, qtrue, "teamSuicideScoring") \
    TVT_CVAR(tvt_spawnArmor, "25 25", "Armor value on spawn, Two values for 'respawn firstSpawn' (e.g. '25 200')", CVAR_ARCHIVE, qtrue, G_TvT_ValidateIntPair, G_TvT_UpdateSpawnArmor, qtrue, "spawnArmor")          \
    TVT_CVAR(tvt_spawnItems, "0 0", "Holdable item bitmask on spawn, Two values for 'respawn firstSpawn' (e.g. '0 8')", CVAR_ARCHIVE, qtrue, G_TvT_ValidateIntPair, G_TvT_UpdateSpawnItems, qtrue, "spawnItems")     \
    TVT_CVAR(tvt_teamSize, "0", "Max players per team, 0 = no limit", CVAR_ARCHIVE, qtrue, NULL, NULL, qtrue, "teamSize")                                                                                          \
    TVT_CVAR(tvt_specAllEnts, "1", "Broadcast all entities without vis-check to free-floating spectators.", CVAR_ARCHIVE|CVAR_SERVERINFO, qtrue, G_TvT_ValidateBool, NULL, qfalse, NULL)                                                                                          \
    TVT_CVAR(tvt_allowSuicide, "1", "Allow players to use the kill command in team based gametypes.", CVAR_ARCHIVE, qtrue, G_TvT_ValidateBool, NULL, qtrue, "allowSuicide")                                  \
    TVT_CVAR(tvt_color, "6", "Primary accent color for mod output (0-7)", CVAR_ARCHIVE, qtrue, G_TvT_ValidateColor, G_TvT_UpdateColor, qfalse, NULL)                                                        \
    TVT_CVAR(tvt_matchMode, "0", "Require all team players to /ready after a shuffle before the match restarts", CVAR_ARCHIVE, qtrue, G_TvT_ValidateBool, G_TvT_UpdateMatchMode, qtrue, "matchMode")                \
    TVT_CVAR(tvt_specPrio, "0", "Queued spectators get priority over last-round players when shuffling teams", CVAR_ARCHIVE, qtrue, G_TvT_ValidateBool, NULL, qtrue, "specPrio")                              \
    TVT_CVAR(tvt_physicsFps, "0", "Force fixed physics FPS for all players (0 = off, 1-333 = fixed)", CVAR_ARCHIVE, qtrue, G_TvT_ValidatePhysicsFps, G_TvT_UpdatePhysicsFps, qtrue, "physicsFps") \

#define TVT_CVAR(name, defaultValue, description, flags, trackChange, validate, update, votable, voteAlias) extern vmCvar_t name;
TVT_CVAR_LIST
#undef TVT_CVAR

void        G_TvT_RegisterCvars(void);
void        G_TvT_UpdateCvars(void);
tvt_Cvar_t *G_TvT_GetCvarTable(void);

#endif
