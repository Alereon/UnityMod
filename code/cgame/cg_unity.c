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

	if (Uni_DrawClock.integer == 1)
	{
		time = va("%02d%s^7%02d%s^7%02d", t.tm_hour, (t.tm_sec % 2) ? "^1:" : " ", t.tm_min, (t.tm_sec % 2) ? "^1:" : " ", t.tm_sec);

		CG_Text_Paint(cgs.screenWidth - CG_Text_Width(time, Uni_DrawClockScale.value, FONT_SMALL) - Uni_DrawClockX.integer, Uni_DrawClockY.integer, Uni_DrawClockScale.value, colorWhite, time, 0, 0, 0, FONT_SMALL);
	}
	else if (Uni_DrawClock.integer >= 2)
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

		CG_Text_Paint(cgs.screenWidth - CG_Text_Width(time, Uni_DrawClockScale.value, FONT_SMALL) - Uni_DrawClockX.integer, Uni_DrawClockY.integer, Uni_DrawClockScale.value, colorWhite, time, 0, 0, 0, FONT_SMALL);
	}

	trap_R_SetColor(NULL);
}
