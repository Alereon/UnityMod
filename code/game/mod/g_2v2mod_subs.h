#ifndef G_2V2MOD_SUBS_H
#define G_2V2MOD_SUBS_H

// Connectionless packet subscriber system.
// External services (Discord bots, stat trackers, etc) can subscribe to receive
// endgame match stats or other potential other forms of data in the future via UDP connectionless packets.

#define TVT_MAX_MSGLEN      16384 // This is size of the packet the engine sends.
#define TVT_CHUNK_DATA_SIZE 16000 // Leave a bit of room for the command type prefix, this should be enough.
#define TVT_SUBS_FILE       "tvt_subs.dat"

// Size of subscriber data to persist (everything except the next pointer).
#define TVT_SUB_RECORD_SIZE ((size_t)&(((tvt_Subscriber_t *)0)->next))

typedef struct {
    const char *name;
    void       (*handler)(const mvaddr_t *addr);
} tvt_SubsCmd_t;

void G_TvT_Subs_HandlePacket(void);
void G_TvT_Subs_SendStats(JSON_t *match);
void G_TvT_Subs_Load(void);
void G_TvT_Subs_Save(void);

#endif
