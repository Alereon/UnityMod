#include "../g_local.h"

static const char *G_TvT_Subs_AddrStr(const mvaddr_t *addr) {
    static char buf[32];

    Com_sprintf(buf, sizeof(buf), "%d.%d.%d.%d:%d",
                addr->ip.v4[0], addr->ip.v4[1], addr->ip.v4[2], addr->ip.v4[3], addr->port);
    return buf;
}

static qboolean G_TvT_Subs_AddrEqual(const mvaddr_t *a, const mvaddr_t *b) {
    if (a->type != b->type || a->port != b->port) {
        return qfalse;
    }
    return a->ip.v4[0] == b->ip.v4[0] && a->ip.v4[1] == b->ip.v4[1] &&
           a->ip.v4[2] == b->ip.v4[2] && a->ip.v4[3] == b->ip.v4[3];
}

static tvt_Subscriber_t *G_TvT_Subs_Find(const mvaddr_t *addr) {
    tvt_Subscriber_t *sub;

    for (sub = level.tvt.subs; sub; sub = sub->next) {
        if (G_TvT_Subs_AddrEqual(&sub->addr, addr)) {
            return sub;
        }
    }
    return NULL;
}

static void G_TvT_Subs_Remove(tvt_Subscriber_t *sub) {
    tvt_Subscriber_t **pp;

    for (pp = &level.tvt.subs; *pp; pp = &(*pp)->next) {
        if (*pp == sub) {
            *pp = sub->next;
            free(sub);
            return;
        }
    }
}

static void G_TvT_Subs_PruneExpired(void) {
    tvt_Subscriber_t *sub;
    tvt_Subscriber_t *next;

    for (sub = level.tvt.subs; sub; sub = next) {
        next = sub->next;
        if (sub->expireTime < level.time) {
            G_TvT_Subs_Remove(sub);
        }
    }
}

static void G_TvT_Subs_Subscribe(const mvaddr_t *addr) {
    tvt_Subscriber_t *sub;

    G_TvT_Subs_PruneExpired();

    sub = G_TvT_Subs_Find(addr);
    if (sub) {
        sub->expireTime = level.time + tvt_subTtl.integer * 1000;
        trap_MVAPI_SendConnectionlessPacket(addr, "subscribed");
        return;
    }

    sub = malloc(sizeof(tvt_Subscriber_t));
    if (!sub) {
        return;
    }

    sub->addr = *addr;
    sub->expireTime = level.time + tvt_subTtl.integer * 1000;
    sub->next = level.tvt.subs;
    level.tvt.subs = sub;

    G_TvT_DebugPrintf("Subs: %s subscribed\n", G_TvT_Subs_AddrStr(addr));
    trap_MVAPI_SendConnectionlessPacket(addr, "subscribed");
}

static void G_TvT_Subs_Unsubscribe(const mvaddr_t *addr) {
    tvt_Subscriber_t *sub;

    sub = G_TvT_Subs_Find(addr);
    if (sub) {
        G_TvT_DebugPrintf("Subs: %s unsubscribed\n", G_TvT_Subs_AddrStr(addr));
        G_TvT_Subs_Remove(sub);
    }
}

static void G_TvT_Subs_Ping(const mvaddr_t *addr) {
    tvt_Subscriber_t *sub;

    sub = G_TvT_Subs_Find(addr);
    if (sub) {
        sub->expireTime = level.time + tvt_subTtl.integer * 1000;
    }
    trap_MVAPI_SendConnectionlessPacket(addr, "pong");
}

static void G_TvT_Subs_SendChunked(const mvaddr_t *addr, const char *type, const char *data, int dataLen) {
    int totalChunks;
    int seq;
    int offset;

    totalChunks = (dataLen + TVT_CHUNK_DATA_SIZE - 1) / TVT_CHUNK_DATA_SIZE;

    offset = 0;
    for (seq = 1; seq <= totalChunks; seq++) {
        char msg[TVT_MAX_MSGLEN];
        int  prefixLen;
        int  chunkLen;

        Com_sprintf(msg, sizeof(msg), "%s %d %d ", type, seq, totalChunks);
        prefixLen = strlen(msg);

        chunkLen = dataLen - offset;
        if (chunkLen > TVT_CHUNK_DATA_SIZE) {
            chunkLen = TVT_CHUNK_DATA_SIZE;
        }
        if (prefixLen + chunkLen >= TVT_MAX_MSGLEN) {
            chunkLen = TVT_MAX_MSGLEN - prefixLen - 1;
        }

        memcpy(msg + prefixLen, data + offset, chunkLen);
        msg[prefixLen + chunkLen] = '\0';

        trap_MVAPI_SendConnectionlessPacket(addr, msg);
        offset += chunkLen;
    }
}

static const tvt_SubsCmd_t g_subsCmds[] = {
    {"subscribe", G_TvT_Subs_Subscribe},
    {"unsubscribe", G_TvT_Subs_Unsubscribe},
    {"ping", G_TvT_Subs_Ping},
    {NULL, NULL}};

void G_TvT_Subs_HandlePacket(void) {
    mvaddr_t             addr;
    char                 buf[MAX_STRING_CHARS];
    const tvt_SubsCmd_t *cmd;

    if (trap_MVAPI_GetConnectionlessPacket(&addr, buf, sizeof(buf))) {
        return;
    }

    // Connectionless packets are prefixed with mvapi, so let's strip that first.
    if (Q_stricmpn(buf, "mvapi ", 6) == 0) {
        memmove(buf, buf + 6, strlen(buf + 6) + 1);
    }

    G_TvT_DebugPrintf("Subs: packet from %s: \"%s\"\n", G_TvT_Subs_AddrStr(&addr), buf);

    for (cmd = g_subsCmds; cmd->name; cmd++) {
        if (Q_stricmp(buf, cmd->name) == 0) {
            cmd->handler(&addr);
            return;
        }
    }
}

void G_TvT_Subs_SendStats(JSON_t *match) {
    tvt_Subscriber_t *sub;
    char             *json;
    int               jsonLen;

    if (!level.tvt.subs) {
        return;
    }

    G_TvT_Subs_PruneExpired();

    if (!level.tvt.subs) {
        return;
    }

    json = TvT_JSON_Serialize(match, qfalse, NULL);
    if (!json) {
        return;
    }

    jsonLen = strlen(json);
    G_TvT_DebugPrintf("Subs: sending stats (%d bytes) to subscribers\n", jsonLen);
    for (sub = level.tvt.subs; sub; sub = sub->next) {
        G_TvT_DebugPrintf("Subs: sending to %s\n", G_TvT_Subs_AddrStr(&sub->addr));
        G_TvT_Subs_SendChunked(&sub->addr, "stats", json, jsonLen);
    }

    free(json);
}

void G_TvT_Subs_Save(void) {
    tvt_Subscriber_t *sub;
    fileHandle_t      f;

    trap_FS_FOpenFile(TVT_SUBS_FILE, &f, FS_WRITE);
    if (!f) {
        return;
    }

    for (sub = level.tvt.subs; sub; sub = sub->next) {
        int saved;

        saved = sub->expireTime;
        sub->expireTime = saved - level.time;
        if (sub->expireTime > 0) {
            trap_FS_Write(sub, TVT_SUB_RECORD_SIZE, f);
        }
        sub->expireTime = saved;
    }

    trap_FS_FCloseFile(f);
    G_TvT_DebugPrintf("Subs: saved subscribers to %s.\n", TVT_SUBS_FILE);
}

void G_TvT_Subs_Load(void) {
    fileHandle_t f;
    int          fileLen;
    char        *buf;
    int          count;
    int          offset;

    fileLen = trap_FS_FOpenFile(TVT_SUBS_FILE, &f, FS_READ);
    if (!f || fileLen <= 0) {
        if (f) {
            trap_FS_FCloseFile(f);
        }
        return;
    }

    buf = malloc(fileLen);
    if (!buf) {
        trap_FS_FCloseFile(f);
        return;
    }

    trap_FS_Read(buf, fileLen, f);
    trap_FS_FCloseFile(f);

    count = 0;
    for (offset = 0; offset + TVT_SUB_RECORD_SIZE <= fileLen; offset += TVT_SUB_RECORD_SIZE) {
        tvt_Subscriber_t *sub;

        sub = malloc(sizeof(tvt_Subscriber_t));
        if (!sub) {
            break;
        }

        memcpy(sub, buf + offset, TVT_SUB_RECORD_SIZE);

        if (sub->expireTime <= 0) {
            free(sub);
            continue;
        }

        sub->expireTime += level.time;
        sub->next = level.tvt.subs;
        level.tvt.subs = sub;
        count++;
    }

    free(buf);
    G_Printf("Subs: loaded %d subscribers from %s.\n", count, TVT_SUBS_FILE);
}
