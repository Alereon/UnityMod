//[Alereon /] - cg_unity.h --  header file for UnityMod.

/*
=================
UnityMod structs.
=================
*/

//[Kevin /] - Defines for mod symbols, colours, etc.
#define UNI_START_SYMBOL "::"
#define UNI_END_SYMBOL   "::"
#define UNI_SEPARATOR    "|"
#define UNI_TEXT_COLOR   "^7"
#define UNI_SYMBOL_COLOR "^1"

#define UNI_AUTHORS      "Alereon" UNI_SYMBOL_COLOR " & " UNI_TEXT_COLOR "Kevin"
#define UNI_CGAMENAME    "UnityMod"
#define UNI_CGAMEVERSION UNI_SYMBOL_COLOR UNI_START_SYMBOL " " UNI_TEXT_COLOR UNI_CGAMENAME " " UNI_SYMBOL_COLOR UNI_END_SYMBOL

//[Alereon /] - Struct related for everything regarding players.
typedef struct {
	int		buddies;

} unityPlayer_t;

//[Alereon /] - Main struct for everything UnityMod related.
typedef struct {
	unityPlayer_t player;

	qboolean mapChange;
} unityMod_t;

extern unityMod_t unity;

/*
=================
UnityMod function prototypes.
=================
*/

//[Alereon /] - HUD.
void Uni_CG_DrawClock( void );


//[Alereon /] - Console commands.
void Uni_CG_Buddies( void );
void Uni_CG_BuddyList( void );

