//[Alereon /] - cg_unity.c -- custom functions used in UnityMod.

#include "cg_local.h"

unityMod_t unity;

/*
=================
HUD Functions.
=================
*/

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
	unityMod_t *unity;

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
	unity = &cgs.clientinfo[cg.clientNum].unityMod;

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

	unity->player.buddies ^= (1 << buddy);

	CG_Printf("%s%s %s%i%s) %s%s %sis %s %s%s\n",
		UNI_SYMBOL_COLOR,
		UNI_START_SYMBOL,
		UNI_TEXT_COLOR,
		buddy,
		UNI_SYMBOL_COLOR,
		UNI_TEXT_COLOR,
		cgs.clientinfo[buddy].name,
		UNI_TEXT_COLOR,
		(unity->player.buddies & (1 << buddy)) ? "now your buddy" : "no longer your buddy",
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
		if (cgs.clientinfo[cg.clientNum].unityMod.player.buddies & (1 << i))
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

//[Kevin /] - Draw inventory items live on the HUD.
void Uni_CG_DrawItemsOnHud( void )
{
	int count    = -1;
	int invsteps = 32;
	int iconSize = Uni_itemsOnHudScale.integer;

	trap_R_SetColor(NULL);

	if (iconSize > 50)
	{
		iconSize = 50;
	}

	if (Uni_itemsOnHud.integer)
	{
		if ((cg.snap->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_MEDPAC)))
		{
			count++;
			CG_DrawPic(Uni_itemsOnHudX.integer - iconSize, Uni_itemsOnHudY.integer + (invsteps * count), iconSize, iconSize, trap_R_RegisterShaderNoMip("gfx/hud/i_icon_bacta"));
		}
		if ((cg.snap->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SENTRY_GUN)))
		{
			count++;
			CG_DrawPic(Uni_itemsOnHudX.integer - iconSize, Uni_itemsOnHudY.integer + (invsteps * count), iconSize, iconSize, trap_R_RegisterShaderNoMip("gfx/hud/i_icon_sentrygun"));
		}
		if ((cg.snap->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SHIELD)))
		{
			count++;
			CG_DrawPic(Uni_itemsOnHudX.integer - iconSize, Uni_itemsOnHudY.integer + (invsteps * count), iconSize, iconSize, trap_R_RegisterShaderNoMip("gfx/hud/i_icon_shieldwall"));
		}
		if ((cg.snap->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SEEKER)))
		{
			count++;
			CG_DrawPic(Uni_itemsOnHudX.integer - iconSize, Uni_itemsOnHudY.integer + (invsteps * count), iconSize, iconSize, trap_R_RegisterShaderNoMip("gfx/hud/i_icon_seeker"));
		}
	}
}
