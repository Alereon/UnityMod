//[Alereon /] - cg_unity.c -- custom functions used in UnityMod.

#include "cg_local.h"

unityMod_t unity;

/*
=================
HUD Functions.
=================
*/

//[Alereon /] - Draws a clock on the screen.
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

//[Kevin /] - Draw inventory items live on the HUD.
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
			count++;
			CG_DrawPic(cgs.screenWidth - iconSize - Uni_drawItemsOnHudX.integer, Uni_drawItemsOnHudY.integer + (32 * count), iconSize, iconSize, cgs.media.invenIcons[i]);
		}
	}

	trap_R_SetColor(NULL);
}

/*
=================
Console command Functions.
=================
*/

//[Alereon /] - Add/remove buddies.
void Uni_CG_Buddies( void )
{
	int buddy;
	clientInfo_t *ci;

	if (trap_Argc() < 2)
	{
		CG_Printf("%s%s %sProvide the clientnumber of your buddy %s%s\n", 
			UNI_SYMBOL_COLOR,
			UNI_START_SYMBOL,
			UNI_TEXT_COLOR,
			UNI_SYMBOL_COLOR,
			UNI_END_SYMBOL);
		return;
	}

	buddy = atoi(CG_Argv(1));
	ci    = cgs.clientinfo + buddy;

	if (buddy < 0 || buddy > MAX_CLIENTS)
	{
		CG_Printf("%s%s '%s%i%s' %sis not a valid clientNum %s%s\n", 
			UNI_SYMBOL_COLOR,
			UNI_START_SYMBOL,
			UNI_TEXT_COLOR,
			buddy,
			UNI_SYMBOL_COLOR,
			UNI_TEXT_COLOR,
			UNI_SYMBOL_COLOR,
			UNI_END_SYMBOL);
		return;
	}

	if (buddy == cg.clientNum)
	{
		CG_Printf("%s%s %sYou cannot make yourself a buddy %s%s\n", 
			UNI_SYMBOL_COLOR,
			UNI_START_SYMBOL,
			UNI_TEXT_COLOR,
			UNI_SYMBOL_COLOR,
			UNI_END_SYMBOL);
		return;
	}

	if (!ci->infoValid)
	{
		CG_Printf("%s%s %sClient %s'%s%i%s' %sis not a valid target %s%s\n",
			UNI_SYMBOL_COLOR,
			UNI_START_SYMBOL,
			UNI_TEXT_COLOR,
			UNI_SYMBOL_COLOR,
			UNI_TEXT_COLOR,
			buddy,
			UNI_SYMBOL_COLOR,
			UNI_TEXT_COLOR,
			UNI_SYMBOL_COLOR,
			UNI_END_SYMBOL);
		return;
	}

	unity.buddies ^= (1 << buddy);

	CG_Printf("%s%s %s%i%s) %s%s %sis %s %s%s\n",
		UNI_SYMBOL_COLOR,
		UNI_START_SYMBOL,
		UNI_TEXT_COLOR,
		buddy,
		UNI_SYMBOL_COLOR,
		UNI_TEXT_COLOR,
		cgs.clientinfo[buddy].name,
		UNI_TEXT_COLOR,
		(unity.buddies & (1 << buddy)) ? "now your buddy" : "no longer your buddy",
		UNI_SYMBOL_COLOR,
		UNI_END_SYMBOL);
}

//[Alereon /] - Print a list of your current buddies.
void Uni_CG_BuddyList( void )
{
	int i;
	int j = 0;

	CG_Printf("%s%s %sBuddy List %s%s\n", 
		UNI_SYMBOL_COLOR,
		UNI_START_SYMBOL,
		UNI_TEXT_COLOR,
		UNI_SYMBOL_COLOR,
		UNI_END_SYMBOL);

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

//[Kevin /] - Prints a list of current blocked regular remaps.
void Uni_CG_ListBlockedRemaps( void )
{
	if ( !Uni_blockShaderRemaps.integer )
	{
		CG_Printf( "\nCurrently got no regular remaps%s.\n", UNI_SYMBOL_COLOR );
		return;
	}
	Uni_CG_ListRemaps( REMAP_REGULAR );
}

//[Kevin /] - Prints a list of current priority remaps.
void Uni_CG_ListPriorityRemaps( void )
{
	if ( !Uni_blockShaderRemaps.integer )
	{
		CG_Printf( "\nCurrently got no priority remaps%s.\n", UNI_SYMBOL_COLOR);
		return;
	}
	Uni_CG_ListRemaps( REMAP_PRIORITY );
}

//[Alereon /] - Handles priority/normal shader remaps.
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

//[Daggolin /] - Shader remap handling.
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
				if ( !counter )
				{
					CG_Printf( "%s%s %sNewly %s shader remaps %s%s\n",
						UNI_SYMBOL_COLOR,
						UNI_START_SYMBOL,
						UNI_TEXT_COLOR,
						(dst->priority ? "prioritized" : "blocked"),
						UNI_SYMBOL_COLOR,
						UNI_END_SYMBOL );
				}

				CG_Printf( "%s- %s%s %s%s %s%s\n",
					UNI_SYMBOL_COLOR,
					UNI_TEXT_COLOR,
					dst->oldShader, 
					UNI_SYMBOL_COLOR,
					UNI_SEPARATOR,
					UNI_TEXT_COLOR,
					dst->newShader );

				counter++;
			}
		}
	}

	if (counter) CG_Printf("\nCurrently %s %s%i %sremaps in total%s.\n%sUse %s/%s%s to get a full list%s.\n", (mode == REMAP_PRIORITY ? "prioritizing" : "blocking"), UNI_SYMBOL_COLOR, counter, UNI_TEXT_COLOR, UNI_SYMBOL_COLOR, UNI_TEXT_COLOR, UNI_SYMBOL_COLOR, UNI_TEXT_COLOR, (mode == REMAP_PRIORITY ? "priorityRemaps" : "blockedRemaps"), UNI_SYMBOL_COLOR);

	// Remember if remaps were blocked
	wasBlocked = Uni_blockShaderRemaps.integer;
}

void Uni_CG_ListRemaps( unityShaderRemapType_t type )
{
	unityShaderRemap_t *dst;
	int counter = 0;
	int i = 0;

	for ( i = 0, dst = &unity.remaps[0]; i < unity.remapsCount; i++, dst++ )
	{
		// Only print message for priority and blocked remaps, not for normal non-blocked ones.
		if ( (type == REMAP_REGULAR && !dst->priority) || (type == REMAP_PRIORITY && dst->priority) )
		{
			if ( !counter )
			{
				CG_Printf( "%s%s %s%s shader remaps %s%s\n",
					UNI_SYMBOL_COLOR,
					UNI_START_SYMBOL,
					UNI_TEXT_COLOR,
					(type == REMAP_PRIORITY ? "Priority" : "Regular"),
					UNI_SYMBOL_COLOR,
					UNI_END_SYMBOL );
			}

			CG_Printf( "%s- %s%s %s%s %s%s\n",
				UNI_SYMBOL_COLOR,
				UNI_TEXT_COLOR,
				dst->oldShader,
				UNI_SYMBOL_COLOR,
				UNI_SEPARATOR,
				UNI_TEXT_COLOR,
				dst->newShader );

			counter++;
		}
	}

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
void Uni_CG_RemoveRemap(char *originalShader)
{
	// NOTE: We don't really need to remove remaps. Theoretically a server can remove remaps from its configstrings, but
	//       they still stay active till the next video restart or until the server overrides them again. So even if a
	//       server remaps a shader with itself and then removes it from the configstrings it can technically still be in
	//       effect if it wasn't blocked at the time. So we don't need to clean anything up, as vid_restart and reconnect
	//       cause our VM to get reloaded and thus clear all shader remaps we had stored.
}
#endif

void Uni_CG_CleanRemaps(void)
{
	// NOTE: We can simply cleanup all remaps that are no longer in the configstrings by pretending we don't know any
	//       remaps and rebuilding our array using the configstrings. However if the server removes remaps from its
	//       configstrings those remaps might still be active and we would loose track of them. So this should only be
	//       used if we're willing to loose track of some remaps.

	unity.remapsCount = 0;
	Uni_CG_ShaderRemaps();
}
