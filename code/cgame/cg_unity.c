// cg_unity.c -- custom functions used in UnityMod.

#include "cg_local.h"
#include "../ui/ui_shared.h"

unityMod_t unity;

/*
==============
HUD Functions.
==============
*/

// Draws a clock on the screen.
void Uni_CG_DrawClock( void )
{
	char	*time;
	qtime_t	t;

	trap_RealTime(&t);

	if (Uni_drawClock.integer == 1)
	{
		time = va("%02d%s%s%02d%s%s%02d", t.tm_hour, (t.tm_sec % 2) ? UNI_SYMBOL_COLOR ":" : " ", UNI_TEXT_COLOR, t.tm_min, (t.tm_sec % 2) ? UNI_SYMBOL_COLOR ":" : " ", UNI_TEXT_COLOR, t.tm_sec);

		CG_Text_Paint(cgs.screenWidth - CG_Text_Width(time, Uni_drawClockScale.value, FONT_SMALL) - Uni_drawClockX.integer, Uni_drawClockY.integer, Uni_drawClockScale.value, colorWhite, time, 0, 0, 0, FONT_SMALL);
	}
	else if (Uni_drawClock.integer >= 2)
	{
		char *pm;
		int h;

		if (t.tm_hour > 12)
		{
			h = t.tm_hour - 12;
			pm = UNI_SYMBOL_COLOR "PM";
		}
		else
		{
			if (t.tm_hour == 0)
			{
				h = 12;
			}
			pm = UNI_SYMBOL_COLOR "AM";
		}

		time = va("%d%s%s%02d %s", h, (t.tm_sec % 2) ? UNI_SYMBOL_COLOR ":" : " ", UNI_TEXT_COLOR, t.tm_min, pm);

		CG_Text_Paint(cgs.screenWidth - CG_Text_Width(time, Uni_drawClockScale.value, FONT_SMALL) - Uni_drawClockX.integer, Uni_drawClockY.integer, Uni_drawClockScale.value, colorWhite, time, 0, 0, 0, FONT_SMALL);
	}

	trap_R_SetColor(NULL);
}

// Draw inventory items live on the HUD.
void Uni_CG_DrawItemsOnHud( void )
{
	int count = -1;
	int iconSize = Uni_drawItemsOnHudScale.integer;
	int i;

	if (!Uni_drawItemsOnHud.integer)
	{
		return;
	}

	if (iconSize > 50)
	{
		iconSize = 50;
	}

	for (i = 0; i < HI_NUM_HOLDABLE; i++)
	{
		if (cg.snap->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << i))
		{
			if (i == HI_DATAPAD || i == HI_NONE)
			{
				continue;
			}
			count++;
			CG_DrawPic(cgs.screenWidth - iconSize - Uni_drawItemsOnHudX.integer, Uni_drawItemsOnHudY.integer + (32 * count), iconSize, iconSize, cgs.media.invenIcons[i]);
		}
	}

	trap_R_SetColor(NULL);
}

/*
=================
Strafe Functions.
=================
*/

//Calculate current speed, max speed and average speed.
void Uni_CG_CalculateSpeed(void)
{
	unityPlayer_t *player;
	vec3_t speed_vel;

	player = &unity.player[cg.snap->ps.clientNum];

	VectorCopy(cg.snap->ps.velocity, speed_vel);
	player->strafe.currentSpeed = (int)sqrt(speed_vel[0] * speed_vel[0] + speed_vel[1] * speed_vel[1]);

	if (player->strafe.currentSpeed > player->strafe.maxSpeed)
	{
		player->strafe.maxSpeed = player->strafe.currentSpeed;
	}

	player->strafe.avgSpeed += player->strafe.currentSpeed;
	player->strafe.avgSpeedSamp++;
}

// Movement keys.
void Uni_CG_DrawMovementKeys( void )
{
	usercmd_t cmd = { 0 };
	char str1[32] = { 0 }, str2[32] = { 0 };
	float scale = Uni_drawMovementKeysScale.value;

	if (!Uni_drawMovementKeys.integer || !cg.snap)
	{
		return;
	}

	if (cg.clientNum == cg.predictedPlayerState.clientNum && !cg.demoPlayback)
	{
		trap_GetUserCmd(trap_GetCurrentCmdNumber(), &cmd);
	}
	else 
	{
		unityPlayer_t *player;
		int moveDir = cg.snap->ps.movementDir;

		player = &unity.player[cg.snap->ps.clientNum];

		if ((cg.snap->ps.pm_flags & PMF_JUMP_HELD))
		{
			cmd.upmove = 1;
		}
		else if ((cg.snap->ps.pm_flags & PMF_DUCKED))
		{
			cmd.upmove = -1;
		}

		if (player->strafe.currentSpeed < 10)
		{
			moveDir = -1;
		}

		switch (moveDir)
		{
			case 0: // W
				cmd.forwardmove = 1;
				break;
			case 1: // WA
				cmd.forwardmove = 1;
				cmd.rightmove = -1;
				break;
			case 2: // A
				cmd.rightmove = -1;
				break;
			case 3: // AS
				cmd.rightmove = -1;
				cmd.forwardmove = -1;
				break;
			case 4: // S
				cmd.forwardmove = -1;
				break;
			case 5: // SD
				cmd.forwardmove = -1;
				cmd.rightmove = 1;
				break;
			case 6: // D
				cmd.rightmove = 1;
				break;
			case 7: // DW
				cmd.rightmove = 1;
				cmd.forwardmove = 1;
				break;
			default:
				break;
		}
	}

	Com_sprintf(str1, sizeof(str1), va("^%cc ^%cW ^%cj", (cmd.upmove < 0) ? COLOR_RED : COLOR_WHITE,
		(cmd.forwardmove > 0) ? COLOR_RED : COLOR_WHITE, (cmd.upmove > 0) ? COLOR_RED : COLOR_WHITE));
	Com_sprintf(str2, sizeof(str2), va("^%cA ^%cS ^%cD", (cmd.rightmove < 0) ? COLOR_RED : COLOR_WHITE,
		(cmd.forwardmove < 0) ? COLOR_RED : COLOR_WHITE, (cmd.rightmove > 0) ? COLOR_RED : COLOR_WHITE));

	CG_Text_Paint(MAX(cgs.screenWidth - CG_Text_Width("c W j", scale, 0), cgs.screenWidth - CG_Text_Width("A S D", scale, 0)) - Uni_drawMovementKeysX.integer, Uni_drawMovementKeysY.integer + scale * BIGCHAR_HEIGHT, scale, colorWhite, str1, 0.5, 0, 0, FONT_NONE);
	CG_Text_Paint(MAX(cgs.screenWidth - CG_Text_Width("c W j", scale, 0), cgs.screenWidth - CG_Text_Width("A S D", scale, 0)) - Uni_drawMovementKeysX.integer, Uni_drawMovementKeysY.integer + scale * BIGCHAR_HEIGHT + CG_Text_Height("A S D c W j", scale, 0) + scale * BIGCHAR_HEIGHT, scale, colorWhite, str2, 0.5, 0, 0, FONT_NONE);
}

char *Uni_CG_ReturnColorForAccel(void)
{
	const float			currentSpeed = unity.player[cg.snap->ps.clientNum].strafe.currentSpeed;
	static float		lastSpeed = 0, previousAccels[ACCEL_SAMPLES];
	const float			accel = currentSpeed - lastSpeed;
	float				total, avgAccel;
	int					t, i;
	static unsigned int index;
	static int			lastupdate;

	t = trap_Milliseconds();

	lastSpeed = currentSpeed;

	if (t - lastupdate > 5)
	{
		lastupdate = t;
		previousAccels[index % ACCEL_SAMPLES] = accel;
		index++;
	}

	total = 0;
	for (i = 0; i < ACCEL_SAMPLES; i++)
	{
		total += previousAccels[i];
	}

	if (!total)
		total = 1;

	avgAccel = total / (float)ACCEL_SAMPLES - 0.0625f;

	if (avgAccel > 0.0f)
	{
		return S_COLOR_GREEN;
	}
	else if (avgAccel < 0.0f)
	{
		return S_COLOR_RED;
	}
	else
	{
		return S_COLOR_WHITE;
	}
}

// Speedometer.
void Uni_CG_Speedometer( void )
{
	unityPlayer_t		*player;
	char				speedStr[2][32] = { 0 };
	float				scale			= Uni_drawSpeedometerScale.value;
	float				currentSpeed;

	if (!Uni_drawSpeedometer.integer)
		return;

	player = &unity.player[cg.snap->ps.clientNum];

	currentSpeed = player->strafe.currentSpeed;

	switch (Uni_drawSpeedometer.integer)
	{
	case 1:
		Com_sprintf(speedStr[0], sizeof(speedStr[0]), "%sµ:^7 %.0f| %.0f", Uni_CG_ReturnColorForAccel(), Q_floorf(player->strafe.currentSpeed + 0.5f), Q_floorf(player->strafe.maxSpeed + 0.5f));
		CG_Text_Paint(MAX(cgs.screenWidth - CG_Text_Width(speedStr[0], scale, 0), cgs.screenWidth - CG_Text_Width(speedStr[0], scale, 0)) - Uni_drawSpeedometerX.integer, Uni_drawSpeedometerY.integer + scale * BIGCHAR_HEIGHT + CG_Text_Height(speedStr[0], scale, 0), scale, colorWhite, speedStr[0], 0.5, 0, 0, FONT_NONE);

		if (Uni_drawAvgSpeed.integer)
		{
			Com_sprintf(speedStr[1], sizeof(speedStr[1]), "Avg µ: %.0f", player->strafe.avgSpeed / player->strafe.avgSpeedSamp);
			CG_Text_Paint(MAX(cgs.screenWidth - CG_Text_Width(speedStr[1], scale, 0), cgs.screenWidth - CG_Text_Width(speedStr[0], scale, 0)) - Uni_drawSpeedometerX.integer, Uni_drawSpeedometerY.integer + scale * BIGCHAR_HEIGHT, scale, colorWhite, speedStr[1], 0.5, 0, 0, FONT_NONE);
		}
		break;
		
	case 2:
		Com_sprintf(speedStr[0], sizeof(speedStr[0]), "%sk:^7 %.1f| %.1f", Uni_CG_ReturnColorForAccel(), currentSpeed * 0.05, player->strafe.maxSpeed * 0.05);
		CG_Text_Paint(MAX(cgs.screenWidth - CG_Text_Width(speedStr[0], scale, 0), cgs.screenWidth - CG_Text_Width(speedStr[0], scale, 0)) - Uni_drawSpeedometerX.integer, Uni_drawSpeedometerY.integer + scale * BIGCHAR_HEIGHT + CG_Text_Height(speedStr[0], scale, 0), scale, colorWhite, speedStr[0], 0.5, 0, 0, FONT_NONE);

		if (Uni_drawAvgSpeed.integer)
		{
			Com_sprintf(speedStr[1], sizeof(speedStr[1]), "Avg k: %.1f", player->strafe.avgSpeed * 0.05 / player->strafe.avgSpeedSamp);
			CG_Text_Paint(MAX(cgs.screenWidth - CG_Text_Width(speedStr[1], scale, 0), cgs.screenWidth - CG_Text_Width(speedStr[0], scale, 0)) - Uni_drawSpeedometerX.integer, Uni_drawSpeedometerY.integer + scale * BIGCHAR_HEIGHT, scale, colorWhite, speedStr[1], 0.5, 0, 0, FONT_NONE);
		}
		break;

	case 3:
	default:
		Com_sprintf(speedStr[0], sizeof(speedStr[0]), "%sm:^7 %.1f| %.1f", Uni_CG_ReturnColorForAccel(), currentSpeed * 0.03106855, player->strafe.maxSpeed * 0.03106855);
		CG_Text_Paint(MAX(cgs.screenWidth - CG_Text_Width(speedStr[0], scale, 0), cgs.screenWidth - CG_Text_Width(speedStr[0], scale, 0)) - Uni_drawSpeedometerX.integer, Uni_drawSpeedometerY.integer + scale * BIGCHAR_HEIGHT + CG_Text_Height(speedStr[0], scale, 0), scale, colorWhite, speedStr[0], 0.5, 0, 0, FONT_NONE);

		if (Uni_drawAvgSpeed.integer)
		{
			Com_sprintf(speedStr[1], sizeof(speedStr[1]), "Avg m: %.1f", player->strafe.avgSpeed * 0.03106855 / player->strafe.avgSpeedSamp);
			CG_Text_Paint(MAX(cgs.screenWidth - CG_Text_Width(speedStr[1], scale, 0), cgs.screenWidth - CG_Text_Width(speedStr[0], scale, 0)) - Uni_drawSpeedometerX.integer, Uni_drawSpeedometerY.integer + scale * BIGCHAR_HEIGHT, scale, colorWhite, speedStr[1], 0.5, 0, 0, FONT_NONE);
		}

		break;
	}

	trap_R_SetColor(NULL);
}

/*
==========================
Console command Functions.
==========================
*/

// Add/remove buddies.
void Uni_CG_Buddies( void )
{
	int buddy;
	clientInfo_t *ci;

	if (trap_Argc() < 2)
	{
		Uni_CG_Printf("Provide the clientnumber of your buddy\n");
		return;
	}

	buddy = atoi(CG_Argv(1));
	ci    = cgs.clientinfo + buddy;

	if (buddy < 0 || buddy > MAX_CLIENTS)
	{
		Uni_CG_Printf("%i Provide the clientnumber of your buddy\n", buddy);
		return;
	}

	if (buddy == cg.clientNum)
	{
		Uni_CG_Printf("You cannot make yourself a buddy\n");
		return;
	}

	if (!ci->infoValid)
	{
		Uni_CG_Printf("Client %s'%s%i%s' %sis not a valid target\n",
			UNI_SYMBOL_COLOR,
			UNI_TEXT_COLOR,
			buddy,
			UNI_SYMBOL_COLOR,
			UNI_TEXT_COLOR);
		return;
	}

	unity.buddies ^= (1 << buddy);

	Uni_CG_Printf("%i%s) %s%s %sis %s\n",
		buddy,
		UNI_SYMBOL_COLOR,
		UNI_TEXT_COLOR,
		cgs.clientinfo[buddy].name,
		UNI_TEXT_COLOR,
		(unity.buddies & (1 << buddy)) ? "now your buddy" : "no longer your buddy");
}

// Print a list of your current buddies.
void Uni_CG_BuddyList( void )
{
	int i;
	int j = 0;

	Uni_CG_Printf("Buddy List\n");

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (unity.buddies & (1 << i))
		{
			CG_Printf("%s%i%s) %s%s%s\n", 
				UNI_TEXT_COLOR,
				i,
				UNI_SYMBOL_COLOR,
				UNI_TEXT_COLOR,
				cgs.clientinfo[i].name,
				UNI_TEXT_COLOR);

			j++;
		}
	}

	if (!j)
	{
		CG_Printf("%sYou have no buddies at the moment%s.\n", 
			UNI_TEXT_COLOR,
			UNI_SYMBOL_COLOR);
	}
}

// Prints a list of current blocked regular remaps.
void Uni_CG_ListBlockedRemaps( void )
{
	if ( !Uni_blockShaderRemaps.integer )
	{
		CG_Printf( "\nCurrently got no regular remaps%s.\n", UNI_SYMBOL_COLOR );
		return;
	}
	Uni_CG_ListRemaps( REMAP_REGULAR );
}

// Prints a list of current priority remaps.
void Uni_CG_ListPriorityRemaps( void )
{
	if ( !Uni_blockShaderRemaps.integer )
	{
		CG_Printf( "\nCurrently got no priority remaps%s.\n", UNI_SYMBOL_COLOR);
		return;
	}
	Uni_CG_ListRemaps( REMAP_PRIORITY );
}

// Handles priority/normal shader remaps.
void Uni_CG_ShaderRemaps( void )
{
	// Regular remaps.
	CG_ShaderStateChanged(CG_ConfigString(CS_SHADERSTATE), qfalse);

	// Priority remaps (calling this second for the special case of more than MAX_SHADER_REMAPS being used on a server, so priority can override regular).
	if ( (cgs.unity_svFlags & UNITY_SVFLAG_PRIORITY_SHADERSTATE) && Uni_blockShaderRemaps.integer )
		CG_ShaderStateChanged(CG_ConfigString(CS_PRIORITY_SHADERSTATE), qtrue);

	// Apply changes.
	Uni_CG_HandleChangedRemaps(REMAP_REGULAR);
	Uni_CG_HandleChangedRemaps(REMAP_PRIORITY);

	unity.remapsUpdated = qfalse;
}

// Shader remap handling.
int Uni_CG_HandleRemap( char *originalShader, char *newShader, float timeOffset, qboolean priority )
{
	unityShaderRemap_t *dst = NULL;
	int i;

	for ( i = 0; i < unity.remapsCount; i++ )
	{
		// Try to find an existing remap for the originalShader.
		if ( !Q_stricmp(unity.remaps[i].oldShader, originalShader) )
		{
			// Already got it
			dst = &unity.remaps[i];
			break;
		}
	}

	if ( i == MAX_SHADER_REMAPS )
	{
		// Too much
		CG_Printf( "^3Warning: Can't keep track of shader remaps (exceeding maximum: %i)\n", MAX_SHADER_REMAPS );
		return !Uni_blockShaderRemaps.integer; // Do default action now as we can't delay it
	}

	if ( dst && ((!Q_stricmp(dst->newShader, newShader) && dst->timeOffset == timeOffset && dst->priority == priority)
		|| (dst->priority && !priority)) )
	{
		// Nothing changed or priority remap exists and overrides this one.
		return 0;
	}

	if ( i == unity.remapsCount )
	{
		// It's a new one.
		dst = &unity.remaps[unity.remapsCount++];
		dst->priority = qfalse;
	}

	// Remember the remap.
	Q_strncpyz( dst->oldShader, originalShader, sizeof(unity.remaps[0].oldShader) );
	Q_strncpyz( dst->newShader, newShader, sizeof(unity.remaps[0].newShader) );
	dst->timeOffset = timeOffset;
	dst->recentlyChanged = qtrue;

	// Mark as priority if it isn't, yet. Don't unmark priority remaps.
	if ( priority ) dst->priority = priority;

	// Don't remap directly, let Uni_CG_HandleChangedRemaps take of it.
	return 0;
}

void Uni_CG_HandleChangedRemaps( unityShaderRemapType_t mode )
{
	static int wasBlocked;
	unityShaderRemap_t *dst;
	int counter = 0;
	int i;

	Uni_Table_Create(unity.remapsCount, 2, va("Newly %s shader remaps", (dst->priority ? "prioritized" : "blocked")));
	Uni_Table_AddRow("Old shader"UD"New shader");

	for ( i = 0, dst = &unity.remaps[0]; i < unity.remapsCount; i++, dst++ )
	{
		if ( dst->recentlyChanged || (!dst->priority && wasBlocked != Uni_blockShaderRemaps.integer) )
		{
			if (mode == REMAP_REGULAR && dst->priority)
				continue;
			else if (mode == REMAP_PRIORITY && !dst->priority)
				continue;

			// Recently modified or the block settings changed.
			dst->recentlyChanged = qfalse;

			// Allow the remap if it's priority, otherwise consult the settings.
			if ( dst->priority || !Uni_blockShaderRemaps.integer )
				trap_R_RemapShader( dst->oldShader, dst->newShader, va("%f", dst->timeOffset) );

			// Only print message for priority and blocked remaps, not for normal non-blocked ones.
			if ( (Uni_blockShaderRemaps.integer && ((Uni_printShaderInformation.integer & 1) && !dst->priority) || ((Uni_printShaderInformation.integer & 2) && dst->priority)) )
			{
				Uni_Table_AddRow("%s"UD"%s", dst->oldShader, dst->newShader);
				counter++;
			}
		}
	}

	Uni_Table_Print();

	if (counter) CG_Printf("\nCurrently %s %s%i %sremaps in total%s.\n%sUse %s/%s%s to get a full list%s.\n", (mode == REMAP_PRIORITY ? "prioritizing" : "blocking"), UNI_SYMBOL_COLOR, counter, UNI_TEXT_COLOR, UNI_SYMBOL_COLOR, UNI_TEXT_COLOR, UNI_SYMBOL_COLOR, UNI_TEXT_COLOR, (mode == REMAP_PRIORITY ? "priorityRemaps" : "blockedRemaps"), UNI_SYMBOL_COLOR);

	// Remember if remaps were blocked
	wasBlocked = Uni_blockShaderRemaps.integer;
}

void Uni_CG_ListRemaps( unityShaderRemapType_t type )
{
	unityShaderRemap_t *dst;
	int counter = 0;
	int i = 0;

	Uni_Table_Create(unity.remapsCount, 2, va("%s shader remaps", (type == REMAP_PRIORITY ? "Priority" : "Regular")));
	Uni_Table_AddRow("Old shader"UD"New shader");

	for ( i = 0, dst = &unity.remaps[0]; i < unity.remapsCount; i++, dst++ )
	{
		// Only print message for priority and blocked remaps, not for normal non-blocked ones.
		if ( (type == REMAP_REGULAR && !dst->priority) || (type == REMAP_PRIORITY && dst->priority) )
		{

			Uni_Table_AddRow("%s"UD"%s", dst->oldShader, dst->newShader);
			counter++;
		}
	}

	Uni_Table_Print();

	CG_Printf( "\nCurrently got %s%i %s%s remaps%s.\n", UNI_SYMBOL_COLOR, counter, UNI_TEXT_COLOR, (type == REMAP_PRIORITY ? "priority" : "regular"), UNI_SYMBOL_COLOR );
}

int Uni_CG_CountRemaps( unityShaderRemapType_t type )
{
	int i, count = 0;

	for ( i = 0; i < unity.remapsCount; i++ )
		if ( (type == REMAP_REGULAR && !unity.remaps[i].priority) || (type == REMAP_PRIORITY && unity.remaps[i].priority) )
			count++;

	return count;
}

#if 0
void Uni_CG_RemoveRemap( char *originalShader )
{
	// NOTE: We don't really need to remove remaps. Theoretically a server can remove remaps from its configstrings, but
	//       they still stay active till the next video restart or until the server overrides them again. So even if a
	//       server remaps a shader with itself and then removes it from the configstrings it can technically still be in
	//       effect if it wasn't blocked at the time. So we don't need to clean anything up, as vid_restart and reconnect
	//       cause our VM to get reloaded and thus clear all shader remaps we had stored.
}
#endif

void Uni_CG_CleanRemaps( void )
{
	// NOTE: We can simply cleanup all remaps that are no longer in the configstrings by pretending we don't know any
	//       remaps and rebuilding our array using the configstrings. However if the server removes remaps from its
	//       configstrings those remaps might still be active and we would loose track of them. So this should only be
	//       used if we're willing to loose track of some remaps.

	unity.remapsCount = 0;
	Uni_CG_ShaderRemaps();
}

// Reset maximum speed.
void Uni_CG_ResetMaxSpeed( void )
{
	unity.player[cg.snap->ps.clientNum].strafe.maxSpeed = 0;
}

// Reset average speed.
void Uni_CG_ResetAverageSpeed( void )
{
	unity.player[cg.snap->ps.clientNum].strafe.avgSpeed = 0;
	unity.player[cg.snap->ps.clientNum].strafe.avgSpeedSamp = 0;
}

/*
==========================
Misc functions.
==========================
*/
float Q_floorf( float x )
{
	if (x < 0)
	{
		x -= 1;
		return (int)x;
	}
	return (int)x;
}

//Returns a string stripped of color codes. (does NOT modify the input string.)
char *Uni_StripColors(char *str)
{
	static char buf[MAX_STRING_CHARS] = { 0 };
	int i = 0;

	while (*str)
	{
		if ((jk2startversion == VERSION_1_02 && Q_IsColorString_1_02(str)) || (jk2startversion != VERSION_1_02 && Q_IsColorString(str)))
		{
			str += 2;
			continue;
		}

		buf[i++] = *str;

		if (i >= MAX_STRING_CHARS - 1)
		{
			break;
		}
		str++;
	}

	buf[i] = 0;

	return buf;
}

void Uni_CG_Printf(const char* msg, ...)
{
	va_list		argptr;
	char		text[1024];
	char		*ptr;

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	ptr = text + strlen(text) - 1;

	while (*ptr)
	{
		if (*ptr == '\n')
		{
			*ptr = 0;
			break;
		}
		ptr--;
	}

	Com_sprintf(text, sizeof(text), "%s%s %s%s %s%s\n", UNI_SYMBOL_COLOR, UNI_SYMBOL, UNI_TEXT_COLOR, text, UNI_SYMBOL_COLOR, UNI_SYMBOL);

	trap_Print(text);
}

/*
==========================
Table functions.
==========================
*/
static char Uni_Table_memoryPool[UNI_MEM_POOL];
static int  uni_Mem_PoolSize;
static unityTable_t uni_Table = { 0 };

void *Uni_Mem_Alloc(int chunk)
{
	if (uni_Mem_PoolSize + chunk >= sizeof(Uni_Table_memoryPool))
	{
		CG_Printf("The requested amount of memory is not available.\n");
		return NULL;
	}

	uni_Mem_PoolSize += chunk;

	return &Uni_Table_memoryPool[uni_Mem_PoolSize - chunk];
}

void Uni_Mem_Free(void)
{
	uni_Mem_PoolSize = 0;
	memset(&uni_Table, 0, sizeof(uni_Table));
}

void Uni_Table_Create(int rows, int columns, const char *name)
{
	int i;

	//Allocate memory for the rows and initialize them.
	uni_Table.row = (unityRow_t*)Uni_Mem_Alloc(sizeof(unityRow_t) * (rows + 1));
	memset(uni_Table.row, 0, sizeof(unityColumn_t) * (rows + 1));

	//Allocate memory for the columns in each row and initialize them.
	for (i = 0; i < rows; i++)
	{
		uni_Table.row[i].column = (unityColumn_t*)Uni_Mem_Alloc(sizeof(unityColumn_t) * columns);
		memset(uni_Table.row[i].column, 0, sizeof(unityColumn_t) * columns);
	}
	
	//Allocate memory to store the length of each column's longest content.
	uni_Table.longestCon = (int*)Uni_Mem_Alloc(sizeof(int) * columns);
	memset(uni_Table.longestCon, 0, sizeof(int) * columns);

	uni_Table.name = (char*)Uni_Mem_Alloc(sizeof(char) * (strlen(name) + 1));
	strcpy(uni_Table.name, name);
	uni_Table.name[strlen(name) + 1] = 0;
	uni_Table.nameLen = strlen(Uni_StripColors(uni_Table.name));

	uni_Table.rows = rows;
	uni_Table.columns = columns;

	uni_Table.minCellLen = (int)uni_Table.nameLen / uni_Table.columns;
}


void Uni_Table_AddRow(const char *content, ...)
{
	va_list va;
	unityColumn_t *cell;
	char buf[MAX_STRING_CHARS] = { 0 };
	char field[MAX_STRING_CHARS] = { 0 };
	int i, j = 0, len, col = 0;

	va_start(va, content);
	Q_vsnprintf(buf, sizeof(buf), content, va);
	va_end(va);

	for (i = 0; i <= strlen(buf); i++)
	{
		if (buf[i] == '\x18' || i == strlen(buf))
		{
			field[j] = 0;
			len = strlen(field);

			cell = (unityColumn_t*)&uni_Table.row[uni_Table.currentRow].column[col];

			cell->content = (char*)Uni_Mem_Alloc(sizeof(char) * (len + 1));
			strcpy(cell->content, field);

			cell->content[len + 1] = 0;

			cell->len = strlen(Uni_StripColors(cell->content));

			if (uni_Table.longestCon[col] < cell->len)
			{
				uni_Table.longestCon[col] = cell->len;
			}

			memset(field, 0, sizeof(field));

			j = 0;
			col++;
			continue;
		}

		field[j++] = buf[i];
	}

	uni_Table.row[uni_Table.currentRow].used = qtrue;
	uni_Table.currentRow++;
}

void Uni_Table_Print_Sepline(void)
{
	int i;
	char *sepLine;

	sepLine = (char*)Uni_Mem_Alloc(sizeof(char) * (uni_Table.rowLen + 2));

	Com_sprintf(sepLine, sizeof(sepLine), "%s", UNI_SYMBOL_COLOR);

	for (i = 2; i < uni_Table.rowLen + 2; i++)
	{ 
		sepLine[i] = '-';
	}

	sepLine[i] = 0;

	CG_Printf("%s\n", sepLine);
}

int Uni_Table_Get_minCellLen(void)
{
	int i, j;
	int len = uni_Table.minCellLen;

	for (i = 0; i < uni_Table.rows; i++)
	{
		if (uni_Table.row[i].used)
		{
			for (j = 0; j < uni_Table.columns; j++)
			{
				if (uni_Table.longestCon[j] > len)
				{
					len = uni_Table.longestCon[j];
				}
			}
		}
	}
	return len;
}

void Uni_Table_Print(void)
{
	unityColumn_t *cell;
	int i, j, count, len;
	int minLen = Uni_Table_Get_minCellLen();
	char row[MAX_STRING_CHARS] = { 0 };
	char separator[MAX_STRING_CHARS] = { 0 };

	for (i = 0; i < uni_Table.rows; i++)
	{
		if (uni_Table.row[i].used)
		{
			for (j = 0; j < uni_Table.columns; j++)
			{
				cell = (unityColumn_t*)&uni_Table.row[i].column[j];

				len = minLen - cell->len;

				for (count = 0; count < (len / 2); count++)
				{
					separator[count] = ' ';
				}
				separator[count] = 0;

				if (j == 0) // We add the initial separator characters to the first column only.
				{
					Com_sprintf(row, sizeof(row), "%s%c %s%s%s%s%s %s%c", UNI_SYMBOL_COLOR, UNI_SEPARATOR, separator, UNI_TEXT_COLOR, cell->content, separator, (len % 2 ? " " : ""), UNI_SYMBOL_COLOR, UNI_SEPARATOR);
				}
				else //Every other column only gets a separator char at the end.
				{
					Com_sprintf(row, sizeof(row), "%s %s%s%s%s%s %s%c", row, separator, UNI_TEXT_COLOR, cell->content, separator, (len % 2 ? " " : ""), UNI_SYMBOL_COLOR, UNI_SEPARATOR);
				}

			}

			uni_Table.rowLen = strlen(Uni_StripColors(row));

			if (i == 0)
			{
				memset(separator, 0, sizeof(separator));
				Uni_Table_Print_Sepline();

				len = (uni_Table.rowLen - uni_Table.nameLen) - 4;
				//4 is the length of the color codes + seperators at the start and end of the table name string.
				for (count = 0; count < (len / 2); count++)
				{
					separator[count] = ' ';
				}
				count = 0;

				CG_Printf("%s%c %s%s%s%s%s %s%c\n", UNI_SYMBOL_COLOR, UNI_SEPARATOR, separator, UNI_TEXT_COLOR, uni_Table.name, separator, (len % 2 ? " " : ""), UNI_SYMBOL_COLOR, UNI_SEPARATOR);
				Uni_Table_Print_Sepline();
				CG_Printf("%s\n", row);
				Uni_Table_Print_Sepline();
			}
			else if ( !uni_Table.row[i + 1].used)
			{
				CG_Printf("%s\n", row);
				Uni_Table_Print_Sepline();
			}
			else
			{
				CG_Printf("%s\n", row);
			}

			memset(separator, 0, sizeof(separator));
			memset(row, 0, sizeof(row));
		}
	}

	Uni_Mem_Free();
}

void Uni_CG_ClearPlayerData(int num)
{
	unityPlayer_t *player;

	player = &unity.player[num];
	//Clear the client's player data.
	memset(player, 0, sizeof(player));

	//Clear our own information we had of the player.
	if (unity.buddies & (1 << num))
	{
		unity.buddies ^= (1 << num);
	}
}
