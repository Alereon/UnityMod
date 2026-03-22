#ifndef G_2V2MOD_CMDS_H
#define G_2V2MOD_CMDS_H

typedef enum {
    CMD_CONTEXT_CLIENT = (1 << 0),
    CMD_CONTEXT_SERVER = (1 << 1),
    CMD_CONTEXT_ALL = (CMD_CONTEXT_CLIENT | CMD_CONTEXT_SERVER)
} cmdContext_t;

typedef struct tvt_Cmd_s {
    const char *name;
    const char *description;
    const char *usage;
    qboolean (*execute)(gentity_t *ent);
    qboolean (*validate)(gentity_t *ent);
    const struct tvt_Cmd_s *subCommands;
    cmdContext_t            context;
    unsigned int            minArgs;
    unsigned int            maxArgs;
    qboolean                votable;
    const char             *voteAlias;
} tvt_Cmd_t;

typedef struct {
    cmdContext_t     context;
    tvt_FilterCtx_t *search;
} tvt_CmdFilterCtx_t;

qboolean   G_TvT_ClientCommand(gentity_t *ent, const char *cmd);
qboolean   G_TvT_ConsoleCommand(const char *cmd);
const tvt_Cmd_t *G_TvT_GetCmdTable(void);

#endif
