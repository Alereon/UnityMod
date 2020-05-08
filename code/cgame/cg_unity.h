// cg_unity.h -- header file for UnityMod.

/*
=================
UnityMod structs.
=================
*/

// Defines for mod symbols, colours, etc.
#define UNI_SYMBOL		 "::"
#define UNI_SEPARATOR    '|'
#define UNI_TEXT_COLOR   "^7"
#define UNI_SYMBOL_COLOR "^1"

#define UNI_AUTHORS      "Alereon" UNI_SYMBOL_COLOR " & " UNI_TEXT_COLOR "Kevin"
#define UNI_CGAMENAME    "UnityMod"
#define UNI_CGAMEVERSION UNI_SYMBOL_COLOR UNI_START_SYMBOL " " UNI_TEXT_COLOR UNI_CGAMENAME " " UNI_SYMBOL_COLOR UNI_END_SYMBOL

// Struct for tracking shader remaps.
#define MAX_SHADER_REMAPS 256 // Classic game module only does 128 remaps, mods might increase this, but I am not aware of any that do, so doubling it should be enough...
typedef struct {
	char				oldShader[MAX_QPATH];
	char				newShader[MAX_QPATH];
	float				timeOffset;
	qboolean			priority;
	qboolean			recentlyChanged;
} unityShaderRemap_t;

typedef enum {
	REMAP_REGULAR,
	REMAP_PRIORITY
} unityShaderRemapType_t;

// Struct for anything related to strafing.
#define ACCEL_SAMPLES 16
typedef struct {
	float				currentSpeed;
	float				maxSpeed;
	float				avgSpeed;
	float				avgSpeedSamp;
} unityStrafe_t;

// Player struct to act as hub for any player related information.
typedef struct {
	unityStrafe_t strafe;
} unityPlayer_t;

// Main struct for everything UnityMod related.
typedef struct {
	// Local.
	int					buddies;
	qboolean			mapChange;

	// Player.
	unityPlayer_t		player[MAX_CLIENTS];

	// Remaps.
	qboolean			remapsUpdated;
	unityShaderRemap_t	remaps[MAX_SHADER_REMAPS];
	int					remapsCount;
} unityMod_t;

extern unityMod_t unity;

#define UNI_MEM_POOL  (256 * 1024)
#define UD "\x18" //Magic character to know when a column ends;

typedef struct {
	char				*content;		//content of the cell.
	int					len;			//string length of the cell.
} unityColumn_t;

typedef struct {
	unityColumn_t		*column;		// The amount of columns each row should have.
	qboolean			used;			//Mark row as being used if the cells of the row are filled with content.
} unityRow_t;

typedef struct {
	unityRow_t			*row;			//The amount of rows the table should have.
	char				*name;			//Name of the table.
	int					nameLen;		//Length of the name of the table.
	int					rows;			//The amount of rows the table has.
	int					columns;		//The amount of columns the table has.
	int					currentRow;		//currently selected row which gets filled when Uni_Table_AddRoW is getting called.
	int					rowLen;			//The length of the rows.
	int					*longestCon;	//stores the longest cell content of each column.
} unityTable_t;

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
void Uni_CG_Speedometer( void );
void Uni_CG_CalculateSpeed( void );
char *Uni_CG_ReturnColorForAccel( void );

// Console commands.
void Uni_CG_Buddies( void );
void Uni_CG_BuddyList( void );
void Uni_CG_ListBlockedRemaps( void );
void Uni_CG_ListPriorityRemaps( void );
void Uni_CG_ResetMaxSpeed( void );
void Uni_CG_ResetAverageSpeed( void );

// Remaps.
void Uni_CG_ShaderRemaps( void );
int Uni_CG_HandleRemap( char *originalShader, char *newShader, float timeOffset, qboolean priority );
void Uni_CG_HandleChangedRemaps( unityShaderRemapType_t mode );
void Uni_CG_ListRemaps( unityShaderRemapType_t type );
int Uni_CG_CountRemaps( unityShaderRemapType_t type );

//Tables.
void Uni_Table_Create(int rows, int columns, const char *name);
void Uni_Table_AddRow(const char *content, ...);
void Uni_Table_Print_Sepline(void);
void Uni_Table_Print(void);

//Tables memory handling.
void *Uni_Mem_Alloc(int chunk);
void Uni_Mem_Free(void);


// Common tools.
float Q_floorf( float x );
char *Uni_StripColors(char *str);
void Uni_CG_Printf(const char* msg, ...);
void Uni_CG_ClearPlayerData(int num);
