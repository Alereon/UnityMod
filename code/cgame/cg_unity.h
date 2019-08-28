//[/Alereon] - cg_unity.h --  header file for UnityMod.

/*
=================
UnityMod structs.
=================
*/

//[/Alereon] - Struct related for everything regarding players.
typedef struct {
	int		buddies;

} unityPlayer_t;

//[/Alereon] - Main struct for everything UnityMod related.
typedef struct {
	unityPlayer_t player;

} unityMod_t;

/*
=================
UnityMod function prototypes.
=================
*/

//[/Alereon] - HUD.
void Uni_CG_DrawClock(void);


//[/Alereon] - Console commands.
void Uni_CG_Buddies(void);
void Uni_CG_BuddyList(void);

