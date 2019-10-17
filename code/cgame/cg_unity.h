// cg_unity.h -- header file for UnityMod.

/*
=================
UnityMod structs.
=================
*/

// Defines for mod symbols, colours, etc.
#define UNI_START_SYMBOL "::"
#define UNI_END_SYMBOL   "::"
#define UNI_SEPARATOR    "|"
#define UNI_TEXT_COLOR   "^7"
#define UNI_SYMBOL_COLOR "^1"

#define UNI_AUTHORS      "Alereon" UNI_SYMBOL_COLOR " & " UNI_TEXT_COLOR "Kevin"
#define UNI_CGAMENAME    "UnityMod"
#define UNI_CGAMEVERSION UNI_SYMBOL_COLOR UNI_START_SYMBOL " " UNI_TEXT_COLOR UNI_CGAMENAME " " UNI_SYMBOL_COLOR UNI_END_SYMBOL

// Struct for tracking shader remaps.
#define MAX_SHADER_REMAPS 256 // Classic game module only does 128 remaps, mods might increase this, but I am not aware of any that do, so doubling it should be enough...
typedef struct {
	char oldShader[MAX_QPATH];
	char newShader[MAX_QPATH];
	float timeOffset;
	qboolean priority;
	qboolean recentlyChanged;
} unityShaderRemap_t;

typedef enum {
	REMAP_REGULAR,
	REMAP_PRIORITY
} unityShaderRemapType_t;

// Main struct for everything UnityMod related.
typedef struct {
	// Local.
	int					buddies;
	qboolean			mapChange;

	// Remaps.
	qboolean			remapsUpdated;
	unityShaderRemap_t	remaps[MAX_SHADER_REMAPS];
	int					remapsCount;
} unityMod_t;

extern unityMod_t unity;

/*
=================
UnityMod function prototypes.
=================
*/

// HUD.
void Uni_CG_DrawClock( void );
void Uni_CG_DrawItemsOnHud( void );

// Strafe.
void Uni_CG_DrawMovementKeys( void );

// Console commands.
void Uni_CG_Buddies( void );
void Uni_CG_BuddyList( void );
void Uni_CG_ListBlockedRemaps( void );
void Uni_CG_ListPriorityRemaps( void );

// Remaps.
void Uni_CG_ShaderRemaps( void );
int Uni_CG_HandleRemap( char *originalShader, char *newShader, float timeOffset, qboolean priority );
void Uni_CG_HandleChangedRemaps( unityShaderRemapType_t mode );
void Uni_CG_ListRemaps( unityShaderRemapType_t type );
int Uni_CG_CountRemaps( unityShaderRemapType_t type );
