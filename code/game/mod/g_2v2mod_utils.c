#include "../g_local.h"

void G_TvT_MakeNameUnique(int clientNum, char *name) {
    char     stripped[MAX_NETNAME];
    char     otherStripped[MAX_NETNAME];
    char     baseName[MAX_NETNAME];
    char     suffix[16];
    int      i, suffixNum, baseMaxLen;
    qboolean duplicate;

    if (!tvt_uniqueNames.integer) {
        return;
    }

    Q_strncpyz(baseName, name, sizeof(baseName));

    suffixNum = 1;
    while (1) {
        Q_strncpyz(stripped, name, sizeof(stripped));
        Q_CleanStr(stripped, (qboolean)(jk2startversion == VERSION_1_02));

        duplicate = qfalse;
        for (i = 0; i < level.maxclients; i++) {
            if (i == clientNum) {
                continue;
            }
            if (level.clients[i].pers.connected == CON_DISCONNECTED) {
                continue;
            }

            Q_strncpyz(otherStripped, level.clients[i].pers.netname, sizeof(otherStripped));
            Q_CleanStr(otherStripped, (qboolean)(jk2startversion == VERSION_1_02));

            if (!Q_stricmp(stripped, otherStripped)) {
                duplicate = qtrue;
                break;
            }
        }

        if (!duplicate) {
            break;
        }

        Com_sprintf(suffix, sizeof(suffix), "^7(%d)", suffixNum);

        baseMaxLen = MAX_NETNAME - 1 - strlen(suffix);

        Q_strncpyz(name, baseName, baseMaxLen + 1);
        Q_strcat(name, MAX_NETNAME, suffix);

        suffixNum++;
    }
}

void G_TvT_DebugPrintf(const char *fmt, ...) {
#ifdef DEBUG
    char    buf[1024];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    G_Printf("^3[TvT DEBUG]^7: %s", buf);
#else
    (void)fmt;
#endif
}

void G_TvT_Printf(int clientNum, const char *fmt, ...) {
    char    buf[1024];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (clientNum == TVT_PRINT_CONSOLE) {
        G_Printf("%s", buf);
    }
    else {
        trap_SendServerCommand(clientNum, va("print \"%s\"", buf));
    }
}

// The engine drops clients at 128 unacknowledged reliable commands.
// cgame only drains commands at snapshot transitions, not every frame,
// so this is the tighter limit.
//
// With sv_dynamicSnapshots, if reliable commands fill too much of the
// 16KB snapshot message, entity states are omitted for that frame.
//
// Small prints flush immediately. Large ones queue and drain per frame,
// only sending when the client has acked.
static char G_TvT_FindLastColor(const char *text, int len) {
    qboolean is102 = (qboolean)(jk2startversion == VERSION_1_02);
    int      i;

    for (i = len - 1; i > 0; i--) {
        if (is102 ? Q_IsColorString_1_02(text + i - 1) : Q_IsColorString(text + i - 1)) {
            return text[i];
        }
    }
    return 0;
}

static void G_TvT_SendConsoleChunk(int clientNum, const char *chunk) {
    (void)clientNum;
    trap_Printf(chunk);
}

static void G_TvT_SendClientChunk(int clientNum, const char *chunk) {
    trap_SendServerCommand(clientNum, va("print \"%s\"", chunk));
}

// Send one chunk (up to TVT_PRINT_BUF_SIZE) from the job's current offset.
// Returns qtrue if the job is finished.
static qboolean G_TvT_SendNextChunk(tvt_printJob_t *job) {
    char        buf[TVT_PRINT_BUF_SIZE + 1];
    const char *p = job->text + job->offset;
    int         remaining = strlen(p);
    int         prefixLen = 0;
    int         maxPayload = TVT_PRINT_BUF_SIZE;
    int         chunkLen;
    char        chunkColor;

    if (job->lastColor) {
        buf[0] = Q_COLOR_ESCAPE;
        buf[1] = job->lastColor;
        prefixLen = 2;
        maxPayload -= 2;
    }

    chunkLen = (remaining > maxPayload) ? maxPayload : remaining;

    if (chunkLen < remaining && p[chunkLen - 1] == Q_COLOR_ESCAPE) {
        chunkLen--;
    }

    memcpy(buf + prefixLen, p, chunkLen);
    buf[prefixLen + chunkLen] = '\0';

    chunkColor = G_TvT_FindLastColor(p, chunkLen);
    if (chunkColor) {
        job->lastColor = chunkColor;
    }

    G_TvT_DebugPrintf("SendNextChunk: client %d, offset %d, chunkLen %d, color ^%c\n",
                      job->clientNum, job->offset, chunkLen, job->lastColor ? job->lastColor : '7');

    G_TvT_SendClientChunk(job->clientNum, buf);

    job->offset += chunkLen;
    return (qboolean)(job->text[job->offset] == '\0');
}

// Send all chunks from text to clientNum immediately, no queuing.
static void G_TvT_FlushAllChunks(int clientNum, const char *text, int bufSize,
                                 void (*send)(int clientNum, const char *chunk)) {
    char buf[TVT_CONSOLE_BUF_SIZE + 1];
    char lastColor = 0;
    int  remaining, chunkLen, prefixLen, maxPayload;
    char chunkColor;
    int  chunkNum = 0;

    while (*text) {
        prefixLen = 0;
        maxPayload = bufSize;

        if (lastColor) {
            buf[0] = Q_COLOR_ESCAPE;
            buf[1] = lastColor;
            prefixLen = 2;
            maxPayload -= 2;
        }

        remaining = strlen(text);
        chunkLen = (remaining > maxPayload) ? maxPayload : remaining;

        // Don't split in the middle of a color code.
        if (chunkLen < remaining && text[chunkLen - 1] == Q_COLOR_ESCAPE) {
            G_TvT_DebugPrintf("FlushAllChunks: color split backup at chunk %d\n", chunkNum);
            chunkLen--;
        }

        memcpy(buf + prefixLen, text, chunkLen);
        buf[prefixLen + chunkLen] = '\0';

        chunkColor = G_TvT_FindLastColor(text, chunkLen);
        if (chunkColor) {
            lastColor = chunkColor;
        }

        G_TvT_DebugPrintf("FlushAllChunks: chunk %d, %d+%d bytes, %d remaining, ^%ccolor\n",
                          chunkNum, chunkLen, prefixLen, remaining - chunkLen, lastColor ? lastColor : '7');

        send(clientNum, buf);
        text += chunkLen;
        chunkNum++;
    }
}

// Send a long string to a client or console, chunked to fit engine limits.
// Small client prints are sent immediately, large ones are queued across frames.
void G_TvT_LongPrint(int clientNum, const char *text) {
    tvt_printJob_t *job;
    int             len;
    int             estChunks;

    if (!text || !*text) {
        return;
    }

    // Console: chunk to fit the server engine's printf buffer MAXPRINTMSG (4096), no rate limit.
    if (clientNum == TVT_PRINT_CONSOLE) {
        G_TvT_DebugPrintf("LongPrint: console, len %d, bufSize %d\n", (int)strlen(text), TVT_CONSOLE_BUF_SIZE);
        G_TvT_FlushAllChunks(clientNum, text, TVT_CONSOLE_BUF_SIZE, G_TvT_SendConsoleChunk);
        return;
    }

    // Client: if small enough, send all chunks now to avoid interleaving.
    // sv_dynamicSnapshots (JK2MV default) lets us burst more safely.
    len = strlen(text);
    estChunks = (len + TVT_PRINT_BUF_SIZE - 1) / TVT_PRINT_BUF_SIZE;

    if (estChunks <= (trap_Cvar_VariableIntegerValue("sv_dynamicSnapshots") ? TVT_PRINT_FLUSH_DYNAMIC : TVT_PRINT_FLUSH_CLASSIC)) {
        G_TvT_DebugPrintf("LongPrint: client %d, len %d, %d chunks (immediate flush)\n", clientNum, len, estChunks);
        G_TvT_FlushAllChunks(clientNum, text, TVT_PRINT_BUF_SIZE, G_TvT_SendClientChunk);
        return;
    }

    // Large print: queue for frame-based draining.
    if (level.tvt.numPrintJobs >= TVT_PRINT_MAX_JOBS) {
        G_Printf("G_TvT_LongPrint: job queue full, dropping print\n");
        return;
    }

    job = &level.tvt.printJobs[level.tvt.numPrintJobs];
    job->clientNum = clientNum;
    job->text = malloc(len + 1);
    if (!job->text) {
        Com_Error(ERR_FATAL, "G_TvT_LongPrint: out of memory");
    }
    memcpy(job->text, text, len + 1);
    job->offset = 0;
    job->lastColor = 0;

    level.tvt.numPrintJobs++;
    G_TvT_DebugPrintf("LongPrint: client %d, len %d, %d chunks (queued, job %d)\n", clientNum, len, estChunks, level.tvt.numPrintJobs);
}

// Drain the print queue. Call once per server frame from G_RunFrame.
void G_TvT_LongPrint_Frame(void) {
    int i;
    int sent;

    if (!level.tvt.numPrintJobs) {
        return;
    }

    sent = 0;
    i = 0;
    G_TvT_DebugPrintf("LongPrint_Frame: %d jobs queued\n", level.tvt.numPrintJobs);

    while (i < level.tvt.numPrintJobs && sent < TVT_PRINT_PER_FRAME) {
        // Only send if the client has sent a packet recently
        gclient_t *cl = &level.clients[level.tvt.printJobs[i].clientNum];
        if (cl->lastCmdTime < level.previousTime) {
            i++;
            continue;
        }

        if (G_TvT_SendNextChunk(&level.tvt.printJobs[i])) {
            int j;

            G_TvT_DebugPrintf("LongPrint_Frame: job %d finished (client %d)\n", i, level.tvt.printJobs[i].clientNum);
            free(level.tvt.printJobs[i].text);

            for (j = i; j < level.tvt.numPrintJobs - 1; j++) {
                level.tvt.printJobs[j] = level.tvt.printJobs[j + 1];
            }
            level.tvt.numPrintJobs--;
        }
        else {
            i++;
        }
        sent++;
    }
}

void G_TvT_TablePrint(table_t *t, int clientNum) {
    char *output;
    output = TvT_Table_ToString(t);
    if (!output) {
        return;
    }

    G_TvT_LongPrint(clientNum, output);
    free(output);
}

static void G_TvT_ResetItem(gentity_t *ent) {
    G_SetOrigin(ent, ent->pos1);
    RespawnItem(ent);
}

static void G_TvT_TouchItem(gentity_t *ent, gentity_t *other, trace_t *trace) {
    Touch_Item(ent, other, trace);

    // Use a wrapper for respawn item to make sure it spawns at its original position
    if (ent->think == RespawnItem) {
        ent->think = G_TvT_ResetItem;
    }
}

void G_TvT_ForcePushItem(gentity_t *item, qboolean pull, vec3_t forward) {
    float speed = pull ? -650.0f : 650.0f;

    // Skip items that have already been picked up and are waiting to respawn.
    if (!item->r.contents) {
        return;
    }

    if (!(item->flags & FL_DROPPED_ITEM)) {
        if (item->touch == Touch_Item) {
            // pos1 is unused for items, so we can store the original spawn position in this field
            VectorCopy(item->r.currentOrigin, item->pos1);
            item->touch = G_TvT_TouchItem;
        }

        item->nextthink = level.time + 30000;
        item->think = G_TvT_ResetItem;
    }

    item->s.pos.trType = TR_GRAVITY;
    item->s.pos.trTime = level.time;
    VectorCopy(item->r.currentOrigin, item->s.pos.trBase);
    VectorScale(forward, speed, item->s.pos.trDelta);

    item->s.groundEntityNum = ENTITYNUM_NONE;
}

void G_TvT_SetLoadout(gclient_t *client, qboolean firstSpawn) {
    tvt_ModState_t *tvt = &level.tvt;
    int             spawnType = firstSpawn ? TVT_FIRST_SPAWN : TVT_SPAWN;

    // Set the first spawn & respawn, shield & holdable value for players
    client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_HEALTH] * tvt->spawnArmor[spawnType] / 100;
    client->ps.stats[STAT_HOLDABLE_ITEMS] |= tvt->spawnItems[spawnType];
}

void G_TvT_FisherYatesShuffle(int *array, int n) {
    // From https://stackoverflow.com/questions/42321370/fisher-yates-shuffling-algorithm-in-c/42322025#42322025
    int i, j, tmp;

    for (i = n - 1; i > 0; i--) {
        j = rand() % (i + 1);
        tmp = array[j];
        array[j] = array[i];
        array[i] = tmp;
    }
}

qboolean G_TvT_IsNumericString(const char *s) {
    if (!s || !*s)
        return qfalse;
    if (*s == '-')
        s++;
    if (!*s)
        return qfalse;
    while (*s) {
        if (*s < '0' || *s > '9')
            return qfalse;
        s++;
    }
    return qtrue;
}


void G_TvT_SetSpecAllEntsBroadcasts(int broadcastClients[2])
{
	int i;

	if (!tvt_specAllEnts.integer) {
		// if tvt_specAllEnts is off, we don't need this
		return;
	}

	// Any clients that are free floating spectators should see this entity
	// Technically followers would be good too, but vanilla/jk2mv engines check by playerstate clientnum, not actual recipient client num.
	for (i = 0; i < level.numConnectedClients; i++)
	{
		gentity_t* ent = &g_entities[level.sortedClients[i]];

		if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) { // don't create a wallhack for non-spectators
			continue;
		}

		if (!ent->client->tvt.isHeadlessClient) { // only send all ents to headless clients, to avoid potentially reaching max snap entities for normal clients
			continue;
		}

		// Turn on the broadcast bit for the master and since there is only one
		// master we are done
		broadcastClients[ent->s.number / 32] |= (1 << (ent->s.number % 32));
	}
}

void G_TvT_UpdateSpecAllEntsBroadcasts(gentity_t* self)
{
	G_TvT_SetSpecAllEntsBroadcasts(self->r.broadcastClients);
	self->r.broadcastClients[self->s.number / 32] &= ~(1 << (self->s.number % 32));
}


