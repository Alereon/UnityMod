#ifndef G_2V2MOD_UTILS_H
#define G_2V2MOD_UTILS_H

#define TVT_PRINT_ALL -1
#define TVT_PRINT_CONSOLE -2

#define TVT_ENT_TO_CN(ent) ((ent) ? (int)((ent) - g_entities) : TVT_PRINT_CONSOLE)

// Generic print function
void G_TvT_Printf(int clientNum, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

void G_TvT_DebugPrintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

void G_TvT_MakeNameUnique(int clientNum, char *name);

// Force push/pull items
void G_TvT_ForcePushItem(gentity_t *item, qboolean pull, vec3_t forward);

// Spawn loadout
void G_TvT_SetLoadout(gclient_t *client, qboolean firstSpawn);

// Set every spectator client in the bitmask for r.broadcastClients of entities when tvt_specAllEnts is 1
void G_TvT_SetSpecAllEntsBroadcasts(int broadcastClients[2]);

// Update broadcasting client to free-floating spectators
void G_TvT_UpdateSpecAllEntsBroadcasts(gentity_t* self);

// Buffered print to send big strings to clients in chunks.
// SV_SendServerCommand silently truncates messages over 1022 bytes (MAX_STRING_CHARS - 2).
// print command format: print "content\n" — 7 prefix + 2 suffix = 9 overhead.
#define TVT_SV_CMD_MAX 1022
#define TVT_PRINT_OVERHEAD 9
#define TVT_PRINT_BUF_SIZE (TVT_SV_CMD_MAX - TVT_PRINT_OVERHEAD)
#define TVT_CONSOLE_BUF_SIZE 4095 // Engine Com_Printf MAXPRINTMSG (4096) minus null terminator
#define TVT_PRINT_PER_FRAME 4
// Max chunks to flush immediately (no interleaving).
// With sv_dynamicSnapshots the engine handles partial reliable command delivery
// across snapshots, so we can safely burst more. Without it, classic engines
// send broken snapshots if the netchan message exceeds 16k (~16 full commands).
#define TVT_PRINT_FLUSH_CLASSIC 6  // ~6KB, safe for classic engines
#define TVT_PRINT_FLUSH_DYNAMIC 12 // 2x classic, sv_dynamicSnapshots handles the rest, i hope
void G_TvT_LongPrint(int clientNum, const char *text);
void G_TvT_LongPrint_Frame(void);

void G_TvT_TablePrint(table_t *t, int clientNum);

// Utilities
void     G_TvT_FisherYatesShuffle(int *array, int n);
qboolean G_TvT_IsNumericString(const char *s);

#endif
