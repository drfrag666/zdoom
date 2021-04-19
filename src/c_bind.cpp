/*
** c_bind.cpp
** Functions for using and maintaining key bindings
**
**---------------------------------------------------------------------------
** Copyright 1998-2006 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include "doomtype.h"
#include "doomdef.h"
#include "cmdlib.h"
#include "c_dispatch.h"
#include "c_bind.h"
#include "g_level.h"
#include "hu_stuff.h"
#include "gi.h"
#include "configfile.h"
#include "i_system.h"
#include "d_event.h"
#include "w_wad.h"

#include <math.h>
#include <stdlib.h>

const char *KeyNames[NUM_KEYS] =
{
	// This array is dependant on the particular keyboard input
	// codes generated in i_input.c. If they change there, they
	// also need to change here. In this case, we use the
	// DirectInput codes and assume a qwerty keyboard layout.
	// See <dinput.h> for the DIK_* codes

	NULL,		"escape",	"1",		"2",		"3",		"4",		"5",		"6",		//00
	"7",		"8",		"9",		"0",		"-",		"=",		"backspace","tab",		//08
	"q",		"w",		"e",		"r",		"t",		"y",		"u",		"i",		//10
	"o",		"p",		"[",		"]",		"enter",	"ctrl",		"a",		"s",		//18
	"d",		"f",		"g",		"h",		"j",		"k",		"l",		";",		//20
	"'",		"`",		"shift",	"\\",		"z",		"x",		"c",		"v",		//28
	"b",		"n",		"m",		",",		".",		"/",		"rshift",	"kp*",		//30
	"alt",		"space",	"capslock",	"f1",		"f2",		"f3",		"f4",		"f5",		//38
	"f6",		"f7",		"f8",		"f9",		"f10",		"numlock",	"scroll",	"kp7",		//40
	"kp8",		"kp9",		"kp-",		"kp4",		"kp5",		"kp6",		"kp+",		"kp1",		//48
	"kp2",		"kp3",		"kp0",		"kp.",		NULL,		NULL,		"oem102",	"f11",		//50
	"f12",		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		//58
	NULL,		NULL,		NULL,		NULL,		"f13",		"f14",		"f15",		"f16",		//60
	NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		//68
	"kana",		NULL,		NULL,		"abnt_c1",	NULL,		NULL,		NULL,		NULL,		//70
	NULL,		"convert",	NULL,		"noconvert",NULL,		"yen",		"abnt_c2",	NULL,		//78
	NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		//80
	NULL,		NULL,		NULL,		NULL,		NULL,		"kp=",		NULL,		NULL,		//88
	"circumflex","@",		":",		"_",		"kanji",	"stop",		"ax",		"unlabeled",//90
	NULL,		"prevtrack",NULL,		NULL,		"kp-enter",	"rctrl",	NULL,		NULL,		//98
	"mute",		"calculator","play",	NULL,		"stop",		NULL,		NULL,		NULL,		//A0
	NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		"voldown",	NULL,		//A8
	"volup",	NULL,		"webhome",	"kp,",		NULL,		"kp/",		NULL,		"sysrq",	//B0
	"ralt",		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		//B8
	NULL,		NULL,		NULL,		NULL,		NULL,		"pause",	NULL,		"home",		//C0
	"uparrow",	"pgup",		NULL,		"leftarrow",NULL,		"rightarrow",NULL,		"end",		//C8
	"downarrow","pgdn",		"ins",		"del",		NULL,		NULL,		NULL,		NULL,		//D0
#ifdef __APPLE__
	NULL,		NULL,		NULL,		"command",	NULL,		"apps",		"power",	"sleep",	//D8
#else // !__APPLE__
	NULL,		NULL,		NULL,		"lwin",		"rwin",		"apps",		"power",	"sleep",	//D8
#endif // __APPLE__
	NULL,		NULL,		NULL,		"wake",		NULL,		"search",	"favorites","refresh",	//E0
	"webstop",	"webforward","webback",	"mycomputer","mail",	"mediaselect",NULL,		NULL,		//E8
	NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		//F0
	NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		//F8

	// non-keyboard buttons that can be bound
	"mouse1",	"mouse2",	"mouse3",	"mouse4",		// 8 mouse buttons
	"mouse5",	"mouse6",	"mouse7",	"mouse8",

	"joy1",		"joy2",		"joy3",		"joy4",			// 128 joystick buttons!
	"joy5",		"joy6",		"joy7",		"joy8",
	"joy9",		"joy10",	"joy11",	"joy12",
	"joy13",	"joy14",	"joy15",	"joy16",
	"joy17",	"joy18",	"joy19",	"joy20",
	"joy21",	"joy22",	"joy23",	"joy24",
	"joy25",	"joy26",	"joy27",	"joy28",
	"joy29",	"joy30",	"joy31",	"joy32",
	"joy2_1",	"joy2_2",	"joy2_3",	"joy2_4",
	"joy2_5",	"joy2_6",	"joy2_7",	"joy2_8",
	"joy2_9",	"joy2_10",	"joy2_11",	"joy2_12",
	"joy2_13",	"joy2_14",	"joy2_15",	"joy2_16",
	"joy2_17",	"joy2_18",	"joy2_19",	"joy2_20",
	"joy2_21",	"joy2_22",	"joy2_23",	"joy2_24",
	"joy2_25",	"joy2_26",	"joy2_27",	"joy2_28",
	"joy2_29",	"joy2_30",	"joy2_31",	"joy2_32",
	"joy3_1",	"joy3_2",	"joy3_3",	"joy3_4",
	"joy3_5",	"joy3_6",	"joy3_7",	"joy3_8",
	"joy3_9",	"joy3_10",	"joy3_11",	"joy3_12",
	"joy3_13",	"joy3_14",	"joy3_15",	"joy3_16",
	"joy3_17",	"joy3_18",	"joy3_19",	"joy3_20",
	"joy3_21",	"joy3_22",	"joy3_23",	"joy3_24",
	"joy3_25",	"joy3_26",	"joy3_27",	"joy3_28",
	"joy3_29",	"joy3_30",	"joy3_31",	"joy3_32",
	"joy4_1",	"joy4_2",	"joy4_3",	"joy4_4",
	"joy4_5",	"joy4_6",	"joy4_7",	"joy4_8",
	"joy4_9",	"joy4_10",	"joy4_11",	"joy4_12",
	"joy4_13",	"joy4_14",	"joy4_15",	"joy4_16",
	"joy4_17",	"joy4_18",	"joy4_19",	"joy4_20",
	"joy4_21",	"joy4_22",	"joy4_23",	"joy4_24",
	"joy4_25",	"joy4_26",	"joy4_27",	"joy4_28",
	"joy4_29",	"joy4_30",	"joy4_31",	"joy4_32",

	"pov1up",	"pov1right","pov1down",	"pov1left",		// First POV hat
	"pov2up",	"pov2right","pov2down",	"pov2left",		// Second POV hat
	"pov3up",	"pov3right","pov3down",	"pov3left",		// Third POV hat
	"pov4up",	"pov4right","pov4down",	"pov4left",		// Fourth POV hat

	"mwheelup",	"mwheeldown",							// the mouse wheel
	"mwheelright", "mwheelleft",

	"axis1plus","axis1minus","axis2plus","axis2minus",	// joystick axes as buttons
	"axis3plus","axis3minus","axis4plus","axis4minus",
	"axis5plus","axis5minus","axis6plus","axis6minus",
	"axis7plus","axis7minus","axis8plus","axis8minus",

	"lstickright","lstickleft","lstickdown","lstickup",			// Gamepad axis-based buttons
	"rstickright","rstickleft","rstickdown","rstickup",

	"dpadup","dpaddown","dpadleft","dpadright",	// Gamepad buttons
	"pad_start","pad_back","lthumb","rthumb",
	"lshoulder","rshoulder","ltrigger","rtrigger",
	"pad_a", "pad_b", "pad_x", "pad_y",

	"pov21up",	"pov21right","pov21down",	"pov21left",		// First POV hat
	"pov22up",	"pov22right","pov22down",	"pov22left",		// Second POV hat
	"pov23up",	"pov23right","pov23down",	"pov23left",		// Third POV hat
	"pov24up",	"pov24right","pov24down",	"pov24left",		// Fourth POV hat

	"pov31up",	"pov31right","pov31down",	"pov31left",		// First POV hat
	"pov32up",	"pov32right","pov32down",	"pov32left",		// Second POV hat
	"pov33up",	"pov33right","pov33down",	"pov33left",		// Third POV hat
	"pov34up",	"pov34right","pov34down",	"pov34left",		// Fourth POV hat

	"pov41up",	"pov41right","pov41down",	"pov41left",		// First POV hat
	"pov42up",	"pov42right","pov42down",	"pov42left",		// Second POV hat
	"pov43up",	"pov43right","pov43down",	"pov43left",		// Third POV hat
	"pov44up",	"pov44right","pov44down",	"pov44left",		// Fourth POV hat

	"axis21plus","axis21minus","axis22plus","axis22minus",	// joystick axes as buttons
	"axis23plus","axis23minus","axis24plus","axis24minus",
	"axis25plus","axis25minus","axis26plus","axis26minus",
	"axis27plus","axis27minus","axis28plus","axis28minus",

	"axis31plus","axis31minus","axis32plus","axis32minus",	// joystick axes as buttons
	"axis33plus","axis33minus","axis34plus","axis34minus",
	"axis35plus","axis35minus","axis36plus","axis36minus",
	"axis37plus","axis37minus","axis38plus","axis38minus",

	"axis41plus","axis41minus","axis42plus","axis42minus",	// joystick axes as buttons
	"axis43plus","axis43minus","axis44plus","axis44minus",
	"axis45plus","axis45minus","axis46plus","axis46minus",
	"axis47plus","axis47minus","axis48plus","axis48minus",

	"lstick2right","lstick2left","lstick2down","lstick2up",			// Gamepad axis-based buttons
	"rstick2right","rstick2left","rstick2down","rstick2up",

	"dpad2up","dpad2down","dpad2left","dpad2right",	// Gamepad buttons
	"pad2_start","pad2_back","lthumb2","rthumb2",
	"lshoulder2","rshoulder2","ltrigger2","rtrigger2",
	"pad2_a", "pad2_b", "pad2_x", "pad2_y",

	"lstick3right","lstick3left","lstick3down","lstick3up",			// Gamepad axis-based buttons
	"rstick3right","rstick3left","rstick3down","rstick3up",

	"dpad3up","dpad3down","dpad3left","dpad3right",	// Gamepad buttons
	"pad3_start","pad3_back","lthumb3","rthumb3",
	"lshoulder3","rshoulder3","ltrigger3","rtrigger3",
	"pad3_a", "pad3_b", "pad3_x", "pad3_y",

	"lstick4right","lstick4left","lstick4down","lstick4up",			// Gamepad axis-based buttons
	"rstick4right","rstick4left","rstick4down","rstick4up",

	"dpad4up","dpad4down","dpad4left","dpad4right",	// Gamepad buttons
	"pad4_start","pad4_back","lthumb4","rthumb4",
	"lshoulder4","rshoulder4","ltrigger4","rtrigger4",
	"pad4_a", "pad4_b", "pad4_x", "pad4_y"
};

FKeyBindings Bindings;
FKeyBindings DoubleBindings;
FKeyBindings AutomapBindings;

static unsigned int DClickTime[NUM_KEYS];
static BYTE DClicked[(NUM_KEYS+7)/8];

//=============================================================================
//
//
//
//=============================================================================

static int GetKeyFromName (const char *name)
{
	int i;

	// Names of the form #xxx are translated to key xxx automatically
	if (name[0] == '#' && name[1] != 0)
	{
		return atoi (name + 1);
	}

	// Otherwise, we scan the KeyNames[] array for a matching name
	for (i = 0; i < NUM_KEYS; i++)
	{
		if (KeyNames[i] && !stricmp (KeyNames[i], name))
			return i;
	}
	return 0;
}

//=============================================================================
//
//
//
//=============================================================================

static int GetConfigKeyFromName (const char *key)
{
	int keynum = GetKeyFromName(key);
	if (keynum == 0)
	{
		if (stricmp (key, "LeftBracket") == 0)
		{
			keynum = GetKeyFromName ("[");
		}
		else if (stricmp (key, "qqBracket") == 0)
		{
			keynum = GetKeyFromName ("]");
		}
		else if (stricmp (key, "Equals") == 0)
		{
			keynum = GetKeyFromName ("=");
		}
		else if (stricmp (key, "KP-Equals") == 0)
		{
			keynum = GetKeyFromName ("kp=");
		}
	}
	return keynum;
}

//=============================================================================
//
//
//
//=============================================================================

static const char *KeyName (int key)
{
	static char name[5];

	if (KeyNames[key])
		return KeyNames[key];

	mysnprintf (name, countof(name), "Key_%d", key);
	return name;
}

//=============================================================================
//
//
//
//=============================================================================

static const char *ConfigKeyName(int keynum)
{
	const char *name = KeyName(keynum);
	if (name[1] == 0)	// Make sure given name is config-safe
	{
		if (name[0] == '[')
			return "LeftBracket";
		else if (name[0] == ']')
			return "RightBracket";
		else if (name[0] == '=')
			return "Equals";
		else if (strcmp (name, "kp=") == 0)
			return "KP-Equals";
	}
	return name;
}

//=============================================================================
//
//
//
//=============================================================================

void C_NameKeys (char *str, int first, int second)
{
	int c = 0;

	*str = 0;
	if (first)
	{
		c++;
		strcpy (str, KeyName (first));
		if (second)
			strcat (str, " or ");
	}

	if (second)
	{
		c++;
		strcat (str, KeyName (second));
	}

	if (!c)
		*str = '\0';
}

//=============================================================================
//
//
//
//=============================================================================

void FKeyBindings::DoBind (const char *key, const char *bind)
{
	int keynum = GetConfigKeyFromName (key);
	if (keynum != 0)
	{
		Binds[keynum] = bind;
	}
}

//=============================================================================
//
//
//
//=============================================================================

void FKeyBindings::UnbindAll ()
{
	for (int i = 0; i < NUM_KEYS; ++i)
	{
		Binds[i] = "";
	}
}

//=============================================================================
//
//
//
//=============================================================================

void FKeyBindings::UnbindKey(const char *key)
{
	int i;

	if ( (i = GetKeyFromName (key)) )
	{
		Binds[i] = "";
	}
	else
	{
		Printf ("Unknown key \"%s\"\n", key);
		return;
	}
}

//=============================================================================
//
//
//
//=============================================================================

void FKeyBindings::PerformBind(FCommandLine &argv, const char *msg)
{
	int i;

	if (argv.argc() > 1)
	{
		i = GetKeyFromName (argv[1]);
		if (!i)
		{
			Printf ("Unknown key \"%s\"\n", argv[1]);
			return;
		}
		if (argv.argc() == 2)
		{
			Printf ("\"%s\" = \"%s\"\n", argv[1], Binds[i].GetChars());
		}
		else
		{
			Binds[i] = argv[2];
		}
	}
	else
	{
		Printf ("%s:\n", msg);
		
		for (i = 0; i < NUM_KEYS; i++)
		{
			if (!Binds[i].IsEmpty())
				Printf ("%s \"%s\"\n", KeyName (i), Binds[i].GetChars());
		}
	}
}


//=============================================================================
//
// This function is first called for functions in custom key sections.
// In this case, matchcmd is non-NULL, and only keys bound to that command
// are stored. If a match is found, its binding is set to "\1".
// After all custom key sections are saved, it is called one more for the
// normal Bindings and DoubleBindings sections for this game. In this case
// matchcmd is NULL and all keys will be stored. The config section was not
// previously cleared, so all old bindings are still in place. If the binding
// for a key is empty, the corresponding key in the config is removed as well.
// If a binding is "\1", then the binding itself is cleared, but nothing
// happens to the entry in the config.
//
//=============================================================================

void FKeyBindings::ArchiveBindings(FConfigFile *f, const char *matchcmd)
{
	int i;

	for (i = 0; i < NUM_KEYS; i++)
	{
		if (Binds[i].IsEmpty())
		{
			if (matchcmd == NULL)
			{
				f->ClearKey(ConfigKeyName(i));
			}
		}
		else if (matchcmd == NULL || stricmp(Binds[i], matchcmd) == 0)
		{
			if (Binds[i][0] == '\1')
			{
				Binds[i] = "";
				continue;
			}
			f->SetValueForKey(ConfigKeyName(i), Binds[i]);
			if (matchcmd != NULL)
			{ // If saving a specific command, set a marker so that
			  // it does not get saved in the general binding list.
				Binds[i] = "\1";
			}
		}
	}
}

//=============================================================================
//
//
//
//=============================================================================

int FKeyBindings::GetKeysForCommand (const char *cmd, int *first, int *second)
{
	int c, i;

	*first = *second = c = i = 0;

	if (cmd[0] == '\0')
	{
		return 0;
	}

	while (i < NUM_KEYS && c < 2)
	{
		if (stricmp (cmd, Binds[i]) == 0)
		{
			if (c++ == 0)
				*first = i;
			else
				*second = i;
		}
		i++;
	}
	return c;
}

//=============================================================================
//
//
//
//=============================================================================

void FKeyBindings::UnbindACommand (const char *str)
{
	int i;

	for (i = 0; i < NUM_KEYS; i++)
	{
		if (!stricmp (str, Binds[i]))
		{
			Binds[i] = "";
		}
	}
}

//=============================================================================
//
//
//
//=============================================================================

void FKeyBindings::DefaultBind(const char *keyname, const char *cmd)
{
	int key = GetKeyFromName (keyname);
	if (key == 0)
	{
		Printf ("Unknown key \"%s\"\n", keyname);
		return;
	}
	if (!Binds[key].IsEmpty())
	{ // This key is already bound.
		return;
	}
	for (int i = 0; i < NUM_KEYS; ++i)
	{
		if (!Binds[i].IsEmpty() && stricmp (Binds[i], cmd) == 0)
		{ // This command is already bound to a key.
			return;
		}
	}
	// It is safe to do the bind, so do it.
	Binds[key] = cmd;
}

//=============================================================================
//
//
//
//=============================================================================

void C_UnbindAll ()
{
	Bindings.UnbindAll();
	DoubleBindings.UnbindAll();
	AutomapBindings.UnbindAll();
}

UNSAFE_CCMD (unbindall)
{
	C_UnbindAll ();
}

//=============================================================================
//
//
//
//=============================================================================

CCMD (unbind)
{
	if (argv.argc() > 1)
	{
		Bindings.UnbindKey(argv[1]);
	}
}

CCMD (undoublebind)
{
	if (argv.argc() > 1)
	{
		DoubleBindings.UnbindKey(argv[1]);
	}
}

CCMD (unmapbind)
{
	if (argv.argc() > 1)
	{
		AutomapBindings.UnbindKey(argv[1]);
	}
}

//=============================================================================
//
//
//
//=============================================================================

CCMD (bind)
{
	Bindings.PerformBind(argv, "Current key bindings");
}

CCMD (doublebind)
{
	DoubleBindings.PerformBind(argv, "Current key doublebindings");
}

CCMD (mapbind)
{
	AutomapBindings.PerformBind(argv, "Current automap key bindings");
}

//==========================================================================
//
// CCMD defaultbind
//
// Binds a command to a key if that key is not already bound and if
// that command is not already bound to another key.
//
//==========================================================================

CCMD (defaultbind)
{
	if (argv.argc() < 3)
	{
		Printf ("Usage: defaultbind <key> <command>\n");
	}
	else
	{
		Bindings.DefaultBind(argv[1], argv[2]);
	}
}

//=============================================================================
//
//
//
//=============================================================================

CCMD (rebind)
{
	FKeyBindings *bindings;

	if (key == 0)
	{
		Printf ("Rebind cannot be used from the console\n");
		return;
	}

	if (key & KEY_DBLCLICKED)
	{
		bindings = &DoubleBindings;
		key &= KEY_DBLCLICKED-1;
	}
	else
	{
		bindings = &Bindings;
	}

	if (argv.argc() > 1)
	{
		bindings->SetBind(key, argv[1]);
	}
}

//=============================================================================
//
//
//
//=============================================================================

void C_BindDefaults ()
{
	int lump, lastlump = 0;

	while ((lump = Wads.FindLump("DEFBINDS", &lastlump)) != -1)
	{
		FScanner sc(lump);

		while (sc.GetString())
		{
			FKeyBindings *dest = &Bindings;
			int key;

			// bind destination is optional and is the same as the console command
			if (sc.Compare("bind"))
			{
				sc.MustGetString();
			}
			else if (sc.Compare("doublebind"))
			{
				dest = &DoubleBindings;
				sc.MustGetString();
			}
			else if (sc.Compare("mapbind"))
			{
				dest = &AutomapBindings;
				sc.MustGetString();
			}
			key = GetConfigKeyFromName(sc.String);
			sc.MustGetString();
			dest->SetBind(key, sc.String);
		}
	}
}

CCMD(binddefaults)
{
	C_BindDefaults ();
}

void C_SetDefaultBindings ()
{
	C_UnbindAll ();
	C_BindDefaults ();
}

//=============================================================================
//
//
//
//=============================================================================

bool C_DoKey (event_t *ev, FKeyBindings *binds, FKeyBindings *doublebinds)
{
	FString binding;
	bool dclick;
	int dclickspot;
	BYTE dclickmask;
	unsigned int nowtime;

	if (ev->type != EV_KeyDown && ev->type != EV_KeyUp)
		return false;

	if ((unsigned int)ev->data1 >= NUM_KEYS)
		return false;

	dclickspot = ev->data1 >> 3;
	dclickmask = 1 << (ev->data1 & 7);
	dclick = false;

	// This used level.time which didn't work outside a level.
	nowtime = I_MSTime();
	if (doublebinds != NULL && DClickTime[ev->data1] > nowtime && ev->type == EV_KeyDown)
	{
		// Key pressed for a double click
		binding = doublebinds->GetBinding(ev->data1);
		DClicked[dclickspot] |= dclickmask;
		dclick = true;
	}
	else
	{
		if (ev->type == EV_KeyDown)
		{ // Key pressed for a normal press
			binding = binds->GetBinding(ev->data1);
			DClickTime[ev->data1] = nowtime + 571;
		}
		else if (doublebinds != NULL && DClicked[dclickspot] & dclickmask)
		{ // Key released from a double click
			binding = doublebinds->GetBinding(ev->data1);
			DClicked[dclickspot] &= ~dclickmask;
			DClickTime[ev->data1] = 0;
			dclick = true;
		}
		else
		{ // Key released from a normal press
			binding = binds->GetBinding(ev->data1);
		}
	}


	if (binding.IsEmpty())
	{
		binding = binds->GetBinding(ev->data1);
		dclick = false;
	}

	if (!binding.IsEmpty() && (chatmodeon == 0 || ev->data1 < 256))
	{
		if (ev->type == EV_KeyUp && binding[0] != '+')
		{
			return false;
		}

		char *copy = binding.LockBuffer();

		if (ev->type == EV_KeyUp)
		{
			copy[0] = '-';
		}

		AddCommandString (copy, dclick ? ev->data1 | KEY_DBLCLICKED : ev->data1);
		return true;
	}
	return false;
}

