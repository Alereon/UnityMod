//[/Alereon] - cg_unity.c -- custom functions used in UnityMod.

#include "cg_local.h"

/*
=================
HUD Functions.
=================
*/

void Uni_CG_DrawClock(void)
{
	char	*time;
	qtime_t	t;

	trap_RealTime(&t);

	if (Uni_drawClock.integer == 1)
	{
		time = va("%02d%s^7%02d%s^7%02d", t.tm_hour, (t.tm_sec % 2) ? "^1:" : " ", t.tm_min, (t.tm_sec % 2) ? "^1:" : " ", t.tm_sec);

		CG_Text_Paint(cgs.screenWidth - CG_Text_Width(time, Uni_drawClockScale.value, FONT_SMALL) - Uni_drawClockX.integer, Uni_drawClockY.integer, Uni_drawClockScale.value, colorWhite, time, 0, 0, 0, FONT_SMALL);
	}
	else if (Uni_drawClock.integer >= 2)
	{
		char *pm;
		int h;

		if (t.tm_hour > 12)
		{
			h = t.tm_hour - 12;
			pm = "^1PM";
		}
		else
		{
			if (t.tm_hour == 0)
			{
				h = 12;
			}
			pm = "^1AM";
		}

		time = va("%d%s^7%02d %s", h, (t.tm_sec % 2) ? "^1:" : " ", t.tm_min, pm);

		CG_Text_Paint(cgs.screenWidth - CG_Text_Width(time, Uni_drawClockScale.value, FONT_SMALL) - Uni_drawClockX.integer, Uni_drawClockY.integer, Uni_drawClockScale.value, colorWhite, time, 0, 0, 0, FONT_SMALL);
	}

	trap_R_SetColor(NULL);
}

/*
=================
Console command Functions.
=================
*/

//[/Alereon] - Add/remove buddies.
void Uni_CG_Buddies(void)
{
	int buddy;
	clientInfo_t *ci;
	unityMod_t *unity;

	if (trap_Argc() < 2)
	{
		CG_Printf("^1:: ^7Provide the clientnumber of your buddy. ^1::\n");
		return;
	}

	buddy = atoi(CG_Argv(1));
	ci = cgs.clientinfo + buddy;
	unity = &cgs.clientinfo[cg.clientNum].unityMod;

	if (buddy < 0 || buddy > MAX_CLIENTS)
	{
		CG_Printf("^1:: ^7%i^1) ^7is not a valid clientNum. ^1::\n", buddy);
		return;
	}

	if (buddy == cg.clientNum)
	{
		CG_Printf("^1:: ^7You cannot make yourself a buddy. ^1::\n", buddy);
		return;
	}

	if (!ci->infoValid)
	{
		CG_Printf("^1:: ^7Client ^1'^7%i^1' ^7is not a valid target. ^1::\n", buddy);
		return;
	}

	unity->player.buddies ^= (1 << buddy);

	CG_Printf("^1:: ^7%i^1) ^7%s ^7is %s ^1::\n",
		buddy,
		cgs.clientinfo[buddy].name,
		(unity->player.buddies & (1 << buddy)) ? "now your buddy." : "no longer your buddy.");
}

//[/Alereon] - Print a list of your current buddies.
void Uni_CG_BuddyList(void)
{
	int i;
	int j = 0;

	CG_Printf("^1:: ^7Buddy List ^1::\n");

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (cgs.clientinfo[cg.clientNum].unityMod.player.buddies & (1 << i))
		{
			CG_Printf("^7%i^1) ^7%s^7\n", i, cgs.clientinfo[i].name);
		}

		j++;
	}

	if (!j)
	{
		CG_Printf("^7You have no buddies at the moment^1.\n");
	}
}
