﻿// Emacs style mode select	 -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//		Moving object handling. Spawn functions.
//
//-----------------------------------------------------------------------------

// HEADER FILES ------------------------------------------------------------
#include <float.h>
#include "templates.h"
#include "i_system.h"
#include "m_random.h"
#include "doomdef.h"
#include "p_local.h"
#include "p_maputl.h"
#include "p_lnspec.h"
#include "p_effect.h"
#include "p_terrain.h"
#include "st_stuff.h"
#include "hu_stuff.h"
#include "s_sound.h"
#include "doomstat.h"
#include "v_video.h"
#include "c_cvars.h"
#include "c_dispatch.h"
#include "b_bot.h"	//Added by MC:
#include "stats.h"
#include "a_sharedglobal.h"
#include "gi.h"
#include "sbar.h"
#include "p_acs.h"
#include "cmdlib.h"
#include "decallib.h"
#include "a_keys.h"
#include "p_conversation.h"
#include "g_game.h"
#include "teaminfo.h"
#include "r_data/r_translate.h"
#include "r_sky.h"
#include "g_level.h"
#include "d_event.h"
#include "colormatcher.h"
#include "v_palette.h"
#include "p_enemy.h"
#include "gstrings.h"
#include "r_data/colormaps.h"
#include "r_renderer.h"
#include "po_man.h"
#include "p_spec.h"
#include "p_checkposition.h"
#include "serializer.h"
#include "r_utility.h"
#include "thingdef.h"
#include "d_player.h"
#include "virtual.h"
#include "g_levellocals.h"
#include "a_morph.h"
#include "events.h"

// MACROS ------------------------------------------------------------------

#define WATER_SINK_FACTOR		0.125
#define WATER_SINK_SMALL_FACTOR	0.25
#define WATER_SINK_SPEED		0.5
#define WATER_JUMP_SPEED		3.5

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

void G_PlayerReborn (int player);

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void PlayerLandedOnThing (AActor *mo, AActor *onmobj);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern int BotWTG;
EXTERN_CVAR (Int,  cl_rockettrails)

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static FRandom pr_explodemissile ("ExplodeMissile");
FRandom pr_bounce ("Bounce");
static FRandom pr_reflect ("Reflect");
static FRandom pr_nightmarerespawn ("NightmareRespawn");
static FRandom pr_botspawnmobj ("BotSpawnActor");
static FRandom pr_spawnmapthing ("SpawnMapThing");
static FRandom pr_spawnpuff ("SpawnPuff");
static FRandom pr_spawnblood ("SpawnBlood");
static FRandom pr_splatter ("BloodSplatter");
static FRandom pr_takedamage ("TakeDamage");
static FRandom pr_splat ("FAxeSplatter");
static FRandom pr_ripperblood ("RipperBlood");
static FRandom pr_chunk ("Chunk");
static FRandom pr_checkmissilespawn ("CheckMissileSpawn");
static FRandom pr_spawnmissile ("SpawnMissile");
static FRandom pr_missiledamage ("MissileDamage");
static FRandom pr_multiclasschoice ("MultiClassChoice");
static FRandom pr_rockettrail("RocketTrail");
static FRandom pr_uniquetid("UniqueTID");

// PUBLIC DATA DEFINITIONS -------------------------------------------------

FRandom pr_spawnmobj ("SpawnActor");

CUSTOM_CVAR (Float, sv_gravity, 800.f, CVAR_SERVERINFO|CVAR_NOSAVE)
{
	level.gravity = self;
}

CVAR (Bool, cl_missiledecals, true, CVAR_ARCHIVE)
CVAR (Bool, addrocketexplosion, false, CVAR_ARCHIVE)
CVAR (Int, cl_pufftype, 0, CVAR_ARCHIVE);
CVAR (Int, cl_bloodtype, 0, CVAR_ARCHIVE);

// CODE --------------------------------------------------------------------

IMPLEMENT_CLASS(AActor, false, true)

IMPLEMENT_POINTERS_START(AActor)
	IMPLEMENT_POINTER(target)
	IMPLEMENT_POINTER(lastenemy)
	IMPLEMENT_POINTER(tracer)
	IMPLEMENT_POINTER(goal)
	IMPLEMENT_POINTER(LastLookActor)
	IMPLEMENT_POINTER(Inventory)
	IMPLEMENT_POINTER(LastHeard)
	IMPLEMENT_POINTER(master)
	IMPLEMENT_POINTER(Poisoner)
	IMPLEMENT_POINTER(alternative)
IMPLEMENT_POINTERS_END

AActor::~AActor ()
{
	// Please avoid calling the destructor directly (or through delete)!
	// Use Destroy() instead.
}

extern FFlagDef InternalActorFlagDefs[];
extern FFlagDef ActorFlagDefs[];

DEFINE_FIELD(AActor, snext)
DEFINE_FIELD(AActor, player)
DEFINE_FIELD_NAMED(AActor, __Pos, pos)
DEFINE_FIELD_NAMED(AActor, __Pos.X, x)
DEFINE_FIELD_NAMED(AActor, __Pos.Y, y)
DEFINE_FIELD_NAMED(AActor, __Pos.Z, z)
DEFINE_FIELD(AActor, Prev)
DEFINE_FIELD(AActor, SpriteAngle)
DEFINE_FIELD(AActor, SpriteRotation)
DEFINE_FIELD(AActor, VisibleStartAngle)
DEFINE_FIELD(AActor, VisibleStartPitch)
DEFINE_FIELD(AActor, VisibleEndAngle)
DEFINE_FIELD(AActor, VisibleEndPitch)
DEFINE_FIELD_NAMED(AActor, Angles.Yaw, angle)
DEFINE_FIELD_NAMED(AActor, Angles.Pitch, pitch)
DEFINE_FIELD_NAMED(AActor, Angles.Roll, roll)
DEFINE_FIELD(AActor, Vel)
DEFINE_FIELD_NAMED(AActor, Vel.X, velx)
DEFINE_FIELD_NAMED(AActor, Vel.Y, vely)
DEFINE_FIELD_NAMED(AActor, Vel.Z, velz)
DEFINE_FIELD_NAMED(AActor, Vel.X, momx)
DEFINE_FIELD_NAMED(AActor, Vel.Y, momy)
DEFINE_FIELD_NAMED(AActor, Vel.Z, momz)
DEFINE_FIELD(AActor, Speed)
DEFINE_FIELD(AActor, FloatSpeed)
DEFINE_FIELD(AActor, sprite)
DEFINE_FIELD(AActor, frame)
DEFINE_FIELD(AActor, Scale)
DEFINE_FIELD_NAMED(AActor, Scale.X, scalex)
DEFINE_FIELD_NAMED(AActor, Scale.Y, scaley)
DEFINE_FIELD(AActor, RenderStyle)
DEFINE_FIELD(AActor, picnum)
DEFINE_FIELD(AActor, Alpha)
DEFINE_FIELD(AActor, fillcolor)
DEFINE_FIELD_NAMED(AActor, Sector, CurSector)	// clashes with type 'sector'.
DEFINE_FIELD(AActor, subsector)
DEFINE_FIELD(AActor, ceilingz)
DEFINE_FIELD(AActor, floorz)
DEFINE_FIELD(AActor, dropoffz)
DEFINE_FIELD(AActor, floorsector)
DEFINE_FIELD(AActor, floorpic)
DEFINE_FIELD(AActor, floorterrain)
DEFINE_FIELD(AActor, ceilingsector)
DEFINE_FIELD(AActor, ceilingpic)
DEFINE_FIELD(AActor, Height)
DEFINE_FIELD(AActor, radius)
DEFINE_FIELD(AActor, renderradius)
DEFINE_FIELD(AActor, projectilepassheight)
DEFINE_FIELD(AActor, tics)
DEFINE_FIELD_NAMED(AActor, state, curstate)		// clashes with type 'state'.
DEFINE_FIELD_NAMED(AActor, DamageVal, Damage)	// name differs for historic reasons
DEFINE_FIELD(AActor, projectileKickback)
DEFINE_FIELD(AActor, VisibleToTeam)
DEFINE_FIELD(AActor, special1)
DEFINE_FIELD(AActor, special2)
DEFINE_FIELD(AActor, specialf1)
DEFINE_FIELD(AActor, specialf2)
DEFINE_FIELD(AActor, weaponspecial)
DEFINE_FIELD(AActor, health)
DEFINE_FIELD(AActor, movedir)
DEFINE_FIELD(AActor, visdir)
DEFINE_FIELD(AActor, movecount)
DEFINE_FIELD(AActor, strafecount)
DEFINE_FIELD(AActor, target)
DEFINE_FIELD(AActor, master)
DEFINE_FIELD(AActor, tracer)
DEFINE_FIELD(AActor, LastHeard)
DEFINE_FIELD(AActor, lastenemy)
DEFINE_FIELD(AActor, LastLookActor)
DEFINE_FIELD(AActor, reactiontime)
DEFINE_FIELD(AActor, threshold)
DEFINE_FIELD(AActor, DefThreshold)
DEFINE_FIELD(AActor, SpawnPoint)
DEFINE_FIELD(AActor, SpawnAngle)
DEFINE_FIELD(AActor, StartHealth)
DEFINE_FIELD(AActor, WeaveIndexXY)
DEFINE_FIELD(AActor, WeaveIndexZ)
DEFINE_FIELD(AActor, skillrespawncount)
DEFINE_FIELD(AActor, args)
DEFINE_FIELD(AActor, Mass)
DEFINE_FIELD(AActor, special)
DEFINE_FIELD(AActor, tid)
DEFINE_FIELD(AActor, TIDtoHate)
DEFINE_FIELD(AActor, waterlevel)
DEFINE_FIELD(AActor, Score)
DEFINE_FIELD(AActor, accuracy)
DEFINE_FIELD(AActor, stamina)
DEFINE_FIELD(AActor, meleerange)
DEFINE_FIELD(AActor, PainThreshold)
DEFINE_FIELD(AActor, Gravity)
DEFINE_FIELD(AActor, Floorclip)
DEFINE_FIELD(AActor, DamageType)
DEFINE_FIELD(AActor, DamageTypeReceived)
DEFINE_FIELD(AActor, FloatBobPhase)
DEFINE_FIELD(AActor, RipperLevel)
DEFINE_FIELD(AActor, RipLevelMin)
DEFINE_FIELD(AActor, RipLevelMax)
DEFINE_FIELD(AActor, Species)
DEFINE_FIELD(AActor, alternative)
DEFINE_FIELD(AActor, goal)
DEFINE_FIELD(AActor, MinMissileChance)
DEFINE_FIELD(AActor, LastLookPlayerNumber)
DEFINE_FIELD(AActor, SpawnFlags)
DEFINE_FIELD(AActor, meleethreshold)
DEFINE_FIELD(AActor, maxtargetrange)
DEFINE_FIELD(AActor, bouncefactor)
DEFINE_FIELD(AActor, wallbouncefactor)
DEFINE_FIELD(AActor, bouncecount)
DEFINE_FIELD(AActor, Friction)
DEFINE_FIELD(AActor, FastChaseStrafeCount)
DEFINE_FIELD(AActor, pushfactor)
DEFINE_FIELD(AActor, lastpush)
DEFINE_FIELD(AActor, activationtype)
DEFINE_FIELD(AActor, lastbump)
DEFINE_FIELD(AActor, DesignatedTeam)
DEFINE_FIELD(AActor, BlockingMobj)
DEFINE_FIELD(AActor, BlockingLine)
DEFINE_FIELD(AActor, PoisonDamage)
DEFINE_FIELD(AActor, PoisonDamageType)
DEFINE_FIELD(AActor, PoisonDuration)
DEFINE_FIELD(AActor, PoisonPeriod)
DEFINE_FIELD(AActor, PoisonDamageReceived)
DEFINE_FIELD(AActor, PoisonDamageTypeReceived)
DEFINE_FIELD(AActor, PoisonDurationReceived)
DEFINE_FIELD(AActor, PoisonPeriodReceived)
DEFINE_FIELD(AActor, Poisoner)
DEFINE_FIELD_NAMED(AActor, Inventory, Inv)		// clashes with type 'Inventory'.
DEFINE_FIELD(AActor, smokecounter)
DEFINE_FIELD(AActor, FriendPlayer)
DEFINE_FIELD(AActor, Translation)
DEFINE_FIELD(AActor, AttackSound)
DEFINE_FIELD(AActor, DeathSound)
DEFINE_FIELD(AActor, SeeSound)
DEFINE_FIELD(AActor, PainSound)
DEFINE_FIELD(AActor, ActiveSound)
DEFINE_FIELD(AActor, UseSound)
DEFINE_FIELD(AActor, BounceSound)
DEFINE_FIELD(AActor, WallBounceSound)
DEFINE_FIELD(AActor, CrushPainSound)
DEFINE_FIELD(AActor, MaxDropOffHeight)
DEFINE_FIELD(AActor, MaxStepHeight)
DEFINE_FIELD(AActor, PainChance)
DEFINE_FIELD(AActor, PainType)
DEFINE_FIELD(AActor, DeathType)
DEFINE_FIELD(AActor, DamageFactor)
DEFINE_FIELD(AActor, DamageMultiply)
DEFINE_FIELD(AActor, TeleFogSourceType)
DEFINE_FIELD(AActor, TeleFogDestType)
DEFINE_FIELD(AActor, SpawnState)
DEFINE_FIELD(AActor, SeeState)
DEFINE_FIELD(AActor, MeleeState)
DEFINE_FIELD(AActor, MissileState)
DEFINE_FIELD(AActor, ConversationRoot)
DEFINE_FIELD(AActor, Conversation)
DEFINE_FIELD(AActor, DecalGenerator)
DEFINE_FIELD(AActor, fountaincolor)
DEFINE_FIELD(AActor, CameraHeight)
DEFINE_FIELD(AActor, RadiusDamageFactor)
DEFINE_FIELD(AActor, SelfDamageFactor)
DEFINE_FIELD(AActor, StealthAlpha)
DEFINE_FIELD(AActor, WoundHealth)
DEFINE_FIELD(AActor, BloodColor)
DEFINE_FIELD(AActor, BloodTranslation)

//==========================================================================
//
// AActor :: Serialize
//
//==========================================================================

#define A(a,b) ((a), (b), def->b)

void AActor::Serialize(FSerializer &arc)
{
	AActor *def = GetDefault();

	Super::Serialize(arc);

	arc
		.Sprite("sprite", sprite, &def->sprite)
		A("pos", __Pos)
		A("angles", Angles)
		A("frame", frame)
		A("scale", Scale)
		A("renderstyle", RenderStyle)
		A("renderflags", renderflags)
		A("picnum", picnum)
		A("floorpic", floorpic)
		A("ceilingpic", ceilingpic)
		A("tidtohate", TIDtoHate)
		A("lastlookpn", LastLookPlayerNumber)
		("lastlookactor", LastLookActor)
		A("effects", effects)
		A("fountaincolor", fountaincolor)
		A("alpha", Alpha)
		A("fillcolor", fillcolor)
		A("sector", Sector)
		A("floorz", floorz)
		A("ceilingz", ceilingz)
		A("dropoffz", dropoffz)
		A("floorsector", floorsector)
		A("ceilingsector", ceilingsector)
		A("radius", radius)
		A("renderradius", renderradius)
		A("height", Height)
		A("ppassheight", projectilepassheight)
		A("vel", Vel)
		A("tics", tics)
		A("state", state)
		A("damage", DamageVal)
		.Terrain("floorterrain", floorterrain, &def->floorterrain)
		A("projectilekickback", projectileKickback)
		A("flags", flags)
		A("flags2", flags2)
		A("flags3", flags3)
		A("flags4", flags4)
		A("flags5", flags5)
		A("flags6", flags6)
		A("flags7", flags7)
		A("flags8", flags8)
		A("weaponspecial", weaponspecial)
		A("special1", special1)
		A("special2", special2)
		A("specialf1", specialf1)
		A("specialf2", specialf2)
		A("health", health)
		A("movedir", movedir)
		A("visdir", visdir)
		A("movecount", movecount)
		A("strafecount", strafecount)
		("target", target)
		("lastenemy", lastenemy)
		("lastheard", LastHeard)
		A("reactiontime", reactiontime)
		A("threshold", threshold)
		A("player", player)
		A("spawnpoint", SpawnPoint)
		A("spawnangle", SpawnAngle)
		A("starthealth", StartHealth)
		A("skillrespawncount", skillrespawncount)
		("tracer", tracer)
		A("floorclip", Floorclip)
		A("tid", tid)
		A("special", special)
		.Args("args", args, def->args, special)
		A("accuracy", accuracy)
		A("stamina", stamina)
		("goal", goal)
		A("waterlevel", waterlevel)
		A("minmissilechance", MinMissileChance)
		A("spawnflags", SpawnFlags)
		("inventory", Inventory)
		A("inventoryid", InventoryID)
		A("floatbobphase", FloatBobPhase)
		A("translation", Translation)
		A("bloodcolor", BloodColor)
		A("bloodtranslation", BloodTranslation)
		A("seesound", SeeSound)
		A("attacksound", AttackSound)
		A("paimsound", PainSound)
		A("deathsound", DeathSound)
		A("activesound", ActiveSound)
		A("usesound", UseSound)
		A("bouncesound", BounceSound)
		A("wallbouncesound", WallBounceSound)
		A("crushpainsound", CrushPainSound)
		A("speed", Speed)
		A("floatspeed", FloatSpeed)
		A("mass", Mass)
		A("painchance", PainChance)
		A("spawnstate", SpawnState)
		A("seestate", SeeState)
		A("meleestate", MeleeState)
		A("missilestate", MissileState)
		A("maxdropoffheight", MaxDropOffHeight)
		A("maxstepheight", MaxStepHeight)
		A("bounceflags", BounceFlags)
		A("bouncefactor", bouncefactor)
		A("wallbouncefactor", wallbouncefactor)
		A("bouncecount", bouncecount)
		A("maxtargetrange", maxtargetrange)
		A("meleethreshold", meleethreshold)
		A("meleerange", meleerange)
		A("damagetype", DamageType)
		A("damagetypereceived", DamageTypeReceived)
		A("paintype", PainType)
		A("deathtype", DeathType)
		A("gravity", Gravity)
		A("fastchasestrafecount", FastChaseStrafeCount)
		("master", master)
		A("smokecounter", smokecounter)
		("blockingmobj", BlockingMobj)
		A("blockingline", BlockingLine)
		A("visibletoteam", VisibleToTeam)
		A("pushfactor", pushfactor)
		A("species", Species)
		A("score", Score)
		A("designatedteam", DesignatedTeam)
		A("lastpush", lastpush)
		A("lastbump", lastbump)
		A("painthreshold", PainThreshold)
		A("damagefactor", DamageFactor)
		A("damagemultiply", DamageMultiply)
		A("waveindexxy", WeaveIndexXY)
		A("weaveindexz", WeaveIndexZ)
		A("pdmgreceived", PoisonDamageReceived)
		A("pdurreceived", PoisonDurationReceived)
		A("ppreceived", PoisonPeriodReceived)
		("poisoner", Poisoner)
		A("posiondamage", PoisonDamage)
		A("poisonduration", PoisonDuration)
		A("poisonperiod", PoisonPeriod)
		A("poisondamagetype", PoisonDamageType)
		A("poisondmgtypereceived", PoisonDamageTypeReceived)
		A("conversationroot", ConversationRoot)
		A("conversation", Conversation)
		A("friendplayer", FriendPlayer)
		A("telefogsourcetype", TeleFogSourceType)
		A("telefogdesttype", TeleFogDestType)
		A("ripperlevel", RipperLevel)
		A("riplevelmin", RipLevelMin)
		A("riplevelmax", RipLevelMax)
		A("devthreshold", DefThreshold)
		A("spriteangle", SpriteAngle)
		A("spriterotation", SpriteRotation)
		("alternative", alternative)
		A("cameraheight", CameraHeight)
		A("tag", Tag)
		A("visiblestartangle",VisibleStartAngle)
		A("visibleendangle",VisibleEndAngle)
		A("visiblestartpitch",VisibleStartPitch)
		A("visibleendpitch",VisibleEndPitch)
		A("woundhealth", WoundHealth)
		A("rdfactor", RadiusDamageFactor)
		A("selfdamagefactor", SelfDamageFactor)
		A("stealthalpha", StealthAlpha);

}

#undef A

//==========================================================================
//
// This must be done after the world is set up.
//
//==========================================================================

void AActor::PostSerialize()
{
	touching_sectorlist = nullptr;
	touching_rendersectors = nullptr;
	LinkToWorld(nullptr, false, Sector);

	AddToHash();
	if (player)
	{
		if (playeringame[player - players] &&
			player->cls != NULL &&
			!(flags4 & MF4_NOSKIN) &&
			state->sprite == GetDefaultByType(player->cls)->SpawnState->sprite)
		{ // Give player back the skin
			sprite = Skins[player->userinfo.GetSkin()].sprite;
		}
		if (Speed == 0)
		{
			Speed = GetDefault()->Speed;
		}
	}
	ClearInterpolation();
	UpdateWaterLevel(false);
}



AActor::AActor () throw()
{
}

AActor::AActor (const AActor &other) throw()
	: DThinker()
{
	memcpy (&snext, &other.snext, (uint8_t *)&this[1] - (uint8_t *)&snext);
}

AActor &AActor::operator= (const AActor &other)
{
	memcpy (&snext, &other.snext, (uint8_t *)&this[1] - (uint8_t *)&snext);
	return *this;
}

//==========================================================================
//
// AActor::InStateSequence
//
// Checks whether the current state is in a contiguous sequence that
// starts with basestate
//
//==========================================================================

bool AActor::InStateSequence(FState * newstate, FState * basestate)
{
	if (basestate == NULL) return false;

	FState * thisstate = basestate;
	do
	{
		if (newstate == thisstate) return true;
		basestate = thisstate;
		thisstate = thisstate->GetNextState();
	}
	while (thisstate == basestate+1);
	return false;
}

DEFINE_ACTION_FUNCTION(AActor, InStateSequence)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_POINTER(newstate, FState);
	PARAM_POINTER(basestate, FState);
	ACTION_RETURN_BOOL(self->InStateSequence(newstate, basestate));
}
//==========================================================================
//
// AActor::GetTics
//
// Get the actual duration of the next state
// We are using a state flag now to indicate a state that should be
// accelerated in Fast mode or slowed in Slow mode.
//
//==========================================================================

int AActor::GetTics(FState * newstate)
{
	int tics = newstate->GetTics();
	if (isFast() && newstate->GetFast())
	{
		return tics - (tics>>1);
	}
	else if (isSlow() && newstate->GetSlow())
	{
		return tics<<1;
	}
	return tics;
}

//==========================================================================
//
// AActor::SetState
//
// Returns true if the mobj is still present.
//
//==========================================================================

bool AActor::SetState (FState *newstate, bool nofunction)
{
	if (debugfile && player && (player->cheats & CF_PREDICTING))
		fprintf (debugfile, "for pl %td: SetState while predicting!\n", player-players);
	do
	{
		if (newstate == NULL)
		{
			state = NULL;
			Destroy ();
			return false;
		}
		int prevsprite, newsprite;

		if (state != NULL)
		{
			prevsprite = state->sprite;
		}
		else
		{
			prevsprite = -1;
		}
		if (!(newstate->UseFlags & SUF_ACTOR))
		{
			auto so = FState::StaticFindStateOwner(newstate);
			Printf(TEXTCOLOR_RED "State %s.%d in %s not flagged for use as an actor sprite\n", so->TypeName.GetChars(), int(newstate - so->OwnedStates), GetClass()->TypeName.GetChars());
			state = nullptr;
			Destroy();
			return false;
		}
		state = newstate;
		tics = GetTics(newstate);
		renderflags = (renderflags & ~RF_FULLBRIGHT) | ActorRenderFlags::FromInt (newstate->GetFullbright());
		newsprite = newstate->sprite;
		if (newsprite != SPR_FIXED)
		{ // okay to change sprite and/or frame
			if (!newstate->GetSameFrame())
			{ // okay to change frame
				frame = newstate->GetFrame();
			}
			if (newsprite != SPR_NOCHANGE)
			{ // okay to change sprite
				if (!(flags4 & MF4_NOSKIN) && newsprite == SpawnState->sprite)
				{ // [RH] If the new sprite is the same as the original sprite, and
				// this actor is attached to a player, use the player's skin's
				// sprite. If a player is not attached, do not change the sprite
				// unless it is different from the previous state's sprite; a
				// player may have been attached, died, and respawned elsewhere,
				// and we do not want to lose the skin on the body. If it wasn't
				// for Dehacked, I would move sprite changing out of the states
				// altogether, since actors rarely change their sprites after
				// spawning.
					if (player != NULL && Skins.Size() > 0)
					{
						sprite = Skins[player->userinfo.GetSkin()].sprite;
					}
					else if (newsprite != prevsprite)
					{
						sprite = newsprite;
					}
				}
				else
				{
					sprite = newsprite;
				}
			}
		}

		if (!nofunction)
		{
			FState *returned_state;
			FStateParamInfo stp = { newstate, STATE_Actor, PSP_WEAPON };
			if (newstate->CallAction(this, this, &stp, &returned_state))
			{
				// Check whether the called action function resulted in destroying the actor
				if (ObjectFlags & OF_EuthanizeMe)
				{
					return false;
				}
				if (returned_state != NULL)
				{ // The action was an A_Jump-style function that wants to change the next state.
					newstate = returned_state;
					tics = 0;		 // make sure we loop and set the new state properly
					continue;
				}
			}
		}
		newstate = newstate->GetNextState();
	} while (tics == 0);

	if (Renderer != NULL)
	{
		Renderer->StateChanged(this);
	}
	return true;
}

DEFINE_ACTION_FUNCTION(AActor, SetState)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_POINTER(state, FState);
	PARAM_BOOL_DEF(nofunction);
	ACTION_RETURN_BOOL(self->SetState(state, nofunction));
};

//============================================================================
//
// AActor :: AddInventory
//
//============================================================================

void AActor::AddInventory (AInventory *item)
{
	// Check if it's already attached to an actor
	if (item->Owner != NULL)
	{
		// Is it attached to us?
		if (item->Owner == this)
			return;

		// No, then remove it from the other actor first
		item->Owner->RemoveInventory (item);
	}

	item->Owner = this;
	item->Inventory = Inventory;
	Inventory = item;

	// Each item receives an unique ID when added to an actor's inventory.
	// This is used by the DEM_INVUSE command to identify the item. Simply
	// using the item's position in the list won't work, because ticcmds get
	// run sometime in the future, so by the time it runs, the inventory
	// might not be in the same state as it was when DEM_INVUSE was sent.
	Inventory->InventoryID = InventoryID++;
}

DEFINE_ACTION_FUNCTION(AActor, AddInventory)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT_NOT_NULL(item, AInventory);
	self->AddInventory(item);
	return 0;
}


//============================================================================
//
// AActor :: GiveInventory
//
//============================================================================

bool AActor::GiveInventory(PClassActor *type, int amount, bool givecheat)
{
	bool result = true;

	if (type == nullptr || !type->IsDescendantOf(RUNTIME_CLASS(AInventory))) return false;

	AWeapon *savedPendingWeap = player != NULL ? player->PendingWeapon : NULL;
	bool hadweap = player != NULL ? player->ReadyWeapon != NULL : true;

	AInventory *item;
	if (!givecheat)
	{
		item = static_cast<AInventory *>(Spawn (type));
	}
	else
	{
		item = static_cast<AInventory *>(Spawn (type, Pos(), NO_REPLACE));
		if (item == NULL) return false;
	}

	// This shouldn't count for the item statistics!
	item->ClearCounters();
	if (!givecheat || amount > 0)
	{
		if (type->IsDescendantOf (PClass::FindActor(NAME_BasicArmorPickup)) || type->IsDescendantOf(PClass::FindActor(NAME_BasicArmorBonus)))
		{
			item->IntVar(NAME_SaveAmount) *= amount;
		}
		else
		{
			if (!givecheat)
				item->Amount = amount;
			else
				item->Amount = MIN (amount, item->MaxAmount);
		}
	}
	if (!item->CallTryPickup (this))
	{
		item->Destroy ();
		result = false;
	}
	// If the item was a weapon, don't bring it up automatically
	// unless the player was not already using a weapon.
	// Don't bring it up automatically if this is called by the give cheat.
	if (!givecheat && player != NULL && savedPendingWeap != NULL && hadweap)
	{
		player->PendingWeapon = savedPendingWeap;
	}
	return result;
}

DEFINE_ACTION_FUNCTION(AActor, GiveInventory)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_CLASS(type, AInventory);
	PARAM_INT(amount);
	PARAM_BOOL_DEF(givecheat);
	ACTION_RETURN_BOOL(self->GiveInventory(type, amount, givecheat));
}


//============================================================================
//
// AActor :: RemoveInventory
//
//============================================================================

void AActor::RemoveInventory(AInventory *item)
{
	AInventory *inv, **invp;

	if (item != NULL && item->Owner != NULL)	// can happen if the owner was destroyed by some action from an item's use state.
	{
		invp = &item->Owner->Inventory;
		for (inv = *invp; inv != NULL; invp = &inv->Inventory, inv = *invp)
		{
			if (inv == item)
			{
				*invp = item->Inventory;

				IFVIRTUALPTR(item, AInventory, DetachFromOwner)
				{
					VMValue params[1] = { item };
					GlobalVMStack.Call(func, params, 1, nullptr, 0, nullptr);
				}

				item->Owner = NULL;
				item->Inventory = NULL;
				break;
			}
		}
	}
}

DEFINE_ACTION_FUNCTION(AActor, RemoveInventory)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT_NOT_NULL(item, AInventory);
	self->RemoveInventory(item);
	return 0;
}


//============================================================================
//
// AActor :: TakeInventory
//
//============================================================================

bool AActor::TakeInventory(PClassActor *itemclass, int amount, bool fromdecorate, bool notakeinfinite)
{
	amount = abs(amount);
	AInventory *item = FindInventory(itemclass);

	if (item == NULL)
		return false;

	if (!fromdecorate)
	{
		item->Amount -= amount;
		if (item->Amount <= 0)
		{
			item->DepleteOrDestroy();
		}
		// It won't be used in non-decorate context, so return false here
		return false;
	}

	bool result = false;
	if (item->Amount > 0)
	{
		result = true;
	}

	// Do not take ammo if the "no take infinite/take as ammo depletion" flag is set
	// and infinite ammo is on
	if (notakeinfinite &&
	((dmflags & DF_INFINITE_AMMO) || (player && FindInventory(NAME_PowerInfiniteAmmo, true))) && item->IsKindOf(NAME_Ammo))
	{
		// Nothing to do here, except maybe res = false;? Would it make sense?
		result = false;
	}
	else if (!amount || amount>=item->Amount)
	{
		item->DepleteOrDestroy();
	}
	else item->Amount-=amount;

	return result;
}

DEFINE_ACTION_FUNCTION(AActor, TakeInventory)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_CLASS_NOT_NULL(item, AInventory);
	PARAM_INT(amount);
	PARAM_BOOL_DEF(fromdecorate);
	PARAM_BOOL_DEF(notakeinfinite);
	ACTION_RETURN_BOOL(self->TakeInventory(item, amount, fromdecorate, notakeinfinite));
}

//============================================================================
//
// AActor :: DestroyAllInventory
//
//============================================================================

void AActor::DestroyAllInventory ()
{
	AInventory *inv = Inventory;
	if (inv != nullptr)
	{
		TArray<AInventory *> toDelete;

		// Delete the list in a two stage approach.
		// This is necessary because an item may destroy another item (e.g. sister weapons)
		// which would break the list and leave parts of it undestroyed, maybe doing bad things later.
		while (inv != nullptr)
		{
			toDelete.Push(inv);
			AInventory *item = inv->Inventory;
			inv->Inventory = nullptr;
			inv->Owner = nullptr;
			inv = item;
		}
		for (auto p : toDelete)
		{
			// the item may already have been deleted by another one, so check this here to avoid problems.
			if (!(p->ObjectFlags & OF_EuthanizeMe))
			{
				p->Destroy();
			}
		}
	}
}

//============================================================================
//
// AActor :: FirstInv
//
// Returns the first item in this actor's inventory that has IF_INVBAR set.
//
//============================================================================

AInventory *AActor::FirstInv ()
{
	if (Inventory == NULL)
	{
		return NULL;
	}
	if (Inventory->ItemFlags & IF_INVBAR)
	{
		return Inventory;
	}
	return Inventory->NextInv ();
}

//============================================================================
//
// AActor :: UseInventory
//
// Attempts to use an item. If the use succeeds, one copy of the item is
// removed from the inventory. If all copies are removed, then the item is
// destroyed.
//
//============================================================================

bool AActor::UseInventory (AInventory *item)
{
	// No using items if you're dead.
	if (health <= 0)
	{
		return false;
	}
	// Don't use it if you don't actually have any of it.
	if (item->Amount <= 0 || (item->ObjectFlags & OF_EuthanizeMe))
	{
		return false;
	}
	if (!item->CallUse (false))
	{
		return false;
	}

	if (dmflags2 & DF2_INFINITE_INVENTORY)
		return true;

	if (--item->Amount <= 0)
	{
		item->DepleteOrDestroy ();
	}
	return true;
}

DEFINE_ACTION_FUNCTION(AActor, UseInventory)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT_NOT_NULL(item, AInventory);
	ACTION_RETURN_BOOL(self->UseInventory(item));
}

//===========================================================================
//
// AActor :: DropInventory
//
// Removes a single copy of an item and throws it out in front of the actor.
//
//===========================================================================

AInventory *AActor::DropInventory (AInventory *item, int amt)
{
	AInventory *drop = nullptr;
	IFVIRTUALPTR(item, AInventory, CreateTossable)
	{
		VMValue params[] = { (DObject*)item, amt };
		VMReturn ret((void**)&drop);
		GlobalVMStack.Call(func, params, countof(params), &ret, 1, nullptr);
	}
	if (drop == nullptr) return NULL;
	drop->SetOrigin(PosPlusZ(10.), false);
	drop->Angles.Yaw = Angles.Yaw;
	drop->VelFromAngle(5.);
	drop->Vel.Z = 1.;
	drop->Vel += Vel;
	drop->flags &= ~MF_NOGRAVITY;	// Don't float
	drop->ClearCounters();	// do not count for statistics again
	return drop;
}

DEFINE_ACTION_FUNCTION(AActor, DropInventory)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT_NOT_NULL(item, AInventory);
	PARAM_INT_DEF(amt);
	ACTION_RETURN_OBJECT(self->DropInventory(item, amt));
}

//============================================================================
//
// AActor :: FindInventory
//
//============================================================================

AInventory *AActor::FindInventory (PClassActor *type, bool subclass)
{
	AInventory *item;

	if (type == NULL)
	{
		return NULL;
	}
	for (item = Inventory; item != NULL; item = item->Inventory)
	{
		if (!subclass)
		{
			if (item->GetClass() == type)
			{
				break;
			}
		}
		else
		{
			if (item->IsKindOf(type))
			{
				break;
			}
		}
	}
	return item;
}

AInventory *AActor::FindInventory (FName type, bool subclass)
{
	return FindInventory(PClass::FindActor(type), subclass);
}

DEFINE_ACTION_FUNCTION(AActor, FindInventory)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_CLASS(type, AInventory);
	PARAM_BOOL_DEF(subclass);
	ACTION_RETURN_OBJECT(self->FindInventory(type, subclass));
}

//============================================================================
//
// AActor :: GiveInventoryType
//
//============================================================================

AInventory *AActor::GiveInventoryType (PClassActor *type)
{
	AInventory *item = NULL;

	if (type != NULL)
	{
		item = static_cast<AInventory *>(Spawn (type));
		if (!item->CallTryPickup (this))
		{
			item->Destroy ();
			return NULL;
		}
	}
	return item;
}

DEFINE_ACTION_FUNCTION(AActor, GiveInventoryType)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_CLASS(type, AInventory);
	ACTION_RETURN_OBJECT(self->GiveInventoryType(type));
}

//============================================================================
//
// AActor :: GiveAmmo
//
// Returns true if the ammo was added, false if not.
//
//============================================================================

bool AActor::GiveAmmo (PClassActor *type, int amount)
{
	if (type != NULL)
	{
		if (!type->IsDescendantOf(RUNTIME_CLASS(AInventory))) return false;

		AInventory *item = static_cast<AInventory *>(Spawn (type));
		if (item)
		{
			item->Amount = amount;
			item->flags |= MF_DROPPED;
			if (!item->CallTryPickup (this))
			{
				item->Destroy ();
				return false;
			}
			return true;
		}
	}
	return false;
}

DEFINE_ACTION_FUNCTION(AActor, GiveAmmo)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_CLASS(type, AInventory);
	PARAM_INT(amount);
	ACTION_RETURN_BOOL(self->GiveAmmo(type, amount));
}

//============================================================================
//
// AActor :: ClearInventory
//
// Clears the inventory of a single actor.
//
//============================================================================

void AActor::ClearInventory()
{
	// In case destroying an inventory item causes another to be destroyed
	// (e.g. Weapons destroy their sisters), keep track of the pointer to
	// the next inventory item rather than the next inventory item itself.
	// For example, if a weapon is immediately followed by its sister, the
	// next weapon we had tracked would be to the sister, so it is now
	// invalid and we won't be able to find the complete inventory by
	// following it.
	//
	// When we destroy an item, we leave invp alone, since the destruction
	// process will leave it pointing to the next item we want to check. If
	// we don't destroy an item, then we move invp to point to its Inventory
	// pointer.
	//
	// It should be safe to assume that an item being destroyed will only
	// destroy items further down in the chain, because if it was going to
	// destroy something we already processed, we've already destroyed it,
	// so it won't have anything to destroy.

	AInventory **invp = &Inventory;

	while (*invp != NULL)
	{
		AInventory *inv = *invp;
		if (!(inv->ItemFlags & IF_UNDROPPABLE))
		{
			inv->DepleteOrDestroy();
			if (!(inv->ObjectFlags & OF_EuthanizeMe)) invp = &inv->Inventory;	// was only depleted so advance the pointer manually.
		}
		else
		{
			invp = &inv->Inventory;
		}
	}
	if (player != nullptr)
	{
		player->ReadyWeapon = nullptr;
		player->PendingWeapon = WP_NOCHANGE;
	}
}

DEFINE_ACTION_FUNCTION(AActor, ClearInventory)
{
	PARAM_SELF_PROLOGUE(AActor);
	self->ClearInventory();
	return 0;
}


//============================================================================
//
// AActor :: CopyFriendliness
//
// Makes this actor hate (or like) the same things another actor does.
//
//============================================================================

void AActor::CopyFriendliness (AActor *other, bool changeTarget, bool resetHealth)
{
	level.total_monsters -= CountsAsKill();
	TIDtoHate = other->TIDtoHate;
	LastLookActor = other->LastLookActor;
	LastLookPlayerNumber = other->LastLookPlayerNumber;
	flags  = (flags & ~MF_FRIENDLY) | (other->flags & MF_FRIENDLY);
	flags3 = (flags3 & ~(MF3_NOSIGHTCHECK | MF3_HUNTPLAYERS)) | (other->flags3 & (MF3_NOSIGHTCHECK | MF3_HUNTPLAYERS));
	flags4 = (flags4 & ~(MF4_NOHATEPLAYERS | MF4_BOSSSPAWNED)) | (other->flags4 & (MF4_NOHATEPLAYERS | MF4_BOSSSPAWNED));
	FriendPlayer = other->FriendPlayer;
	DesignatedTeam = other->DesignatedTeam;
	if (changeTarget && other->target != NULL && !(other->target->flags3 & MF3_NOTARGET) && !(other->target->flags7 & MF7_NEVERTARGET))
	{
		// LastHeard must be set as well so that A_Look can react to the new target if called
		LastHeard = target = other->target;
	}	
	if (resetHealth) health = SpawnHealth();	
	level.total_monsters += CountsAsKill();
}

DEFINE_ACTION_FUNCTION(AActor, CopyFriendliness)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT_NOT_NULL(other, AActor);
	PARAM_BOOL_DEF(changetarget);
	PARAM_BOOL_DEF(resethealth);
	self->CopyFriendliness(other, changetarget, resethealth);
	return 0;
}
//============================================================================
//
// AActor :: ObtainInventory
//
// Removes the items from the other actor and puts them in this actor's
// inventory. The actor receiving the inventory must not have any items.
//
//============================================================================

void AActor::ObtainInventory (AActor *other)
{
	assert (Inventory == NULL);

	Inventory = other->Inventory;
	InventoryID = other->InventoryID;
	other->Inventory = NULL;
	other->InventoryID = 0;

	if (other->IsKindOf(RUNTIME_CLASS(APlayerPawn)) && this->IsKindOf(RUNTIME_CLASS(APlayerPawn)))
	{
		APlayerPawn *you = static_cast<APlayerPawn *>(other);
		APlayerPawn *me = static_cast<APlayerPawn *>(this);
		me->InvFirst = you->InvFirst;
		me->InvSel = you->InvSel;
		you->InvFirst = NULL;
		you->InvSel = NULL;
	}

	AInventory *item = Inventory;
	while (item != NULL)
	{
		item->Owner = this;
		item = item->Inventory;
	}
}

DEFINE_ACTION_FUNCTION(AActor, ObtainInventory)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT(other, AActor);
	self->ObtainInventory(other);
	return 0;
}

//---------------------------------------------------------------------------
//
// FUNC P_GiveBody
//
// Returns false if the body isn't needed at all.
//
//---------------------------------------------------------------------------

bool P_GiveBody(AActor *actor, int num, int max)
{
	if (actor->health <= 0 || (actor->player != NULL && actor->player->playerstate == PST_DEAD))
	{ // Do not heal dead things.
		return false;
	}

	player_t *player = actor->player;

	num = clamp(num, -65536, 65536);	// prevent overflows for bad values
	if (player != NULL)
	{
		// Max is 0 by default, preserving default behavior for P_GiveBody()
		// calls while supporting health pickups.
		if (max <= 0)
		{
			max = static_cast<APlayerPawn*>(actor)->GetMaxHealth(true);
			// [MH] First step in predictable generic morph effects
			if (player->morphTics)
			{
				if (player->MorphStyle & MORPH_FULLHEALTH)
				{
					if (!(player->MorphStyle & MORPH_ADDSTAMINA))
					{
						max -= player->mo->stamina + player->mo->BonusHealth;
					}
				}
				else // old health behaviour
				{
					max = MAXMORPHHEALTH;
					if (player->MorphStyle & MORPH_ADDSTAMINA)
					{
						max += player->mo->stamina + player->mo->BonusHealth;
					}
				}
			}
		}
		// [RH] For Strife: A negative body sets you up with a percentage
		// of your full health.
		if (num < 0)
		{
			num = max * -num / 100;
			if (player->health < num)
			{
				player->health = num;
				actor->health = num;
				return true;
			}
		}
		else if (num > 0)
		{
			if (player->health < max)
			{
				num = int(num * G_SkillProperty(SKILLP_HealthFactor));
				if (num < 1) num = 1;
				player->health += num;
				if (player->health > max)
				{
					player->health = max;
				}
				actor->health = player->health;
				return true;
			}
		}
	}
	else
	{
		// Parameter value for max is ignored on monsters, preserving original
		// behaviour of health as well as on existing calls to P_GiveBody().
		max = actor->SpawnHealth();
		if (num < 0)
		{
			num = max * -num / 100;
			if (actor->health < num)
			{
				actor->health = num;
				return true;
			}
		}
		else if (actor->health < max)
		{
			actor->health += num;
			if (actor->health > max)
			{
				actor->health = max;
			}
			return true;
		}
	}
	return false;
}

DEFINE_ACTION_FUNCTION(AActor, GiveBody)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_INT(num);
	PARAM_INT_DEF(max);
	ACTION_RETURN_BOOL(P_GiveBody(self, num, max));
}

//============================================================================
//
// AActor :: CheckLocalView
//
// Returns true if this actor is local for the player. Here, local means the
// player is either looking out this actor's eyes, or this actor is the player
// and the player is looking out the eyes of something non-"sentient."
//
//============================================================================

bool AActor::CheckLocalView (int playernum) const
{
	if (players[playernum].camera == this)
	{
		return true;
	}
	if (players[playernum].mo != this || players[playernum].camera == NULL)
	{
		return false;
	}
	if (players[playernum].camera->player == NULL &&
		!(players[playernum].camera->flags3 & MF3_ISMONSTER))
	{
		return true;
	}
	return false;
}

DEFINE_ACTION_FUNCTION(AActor, CheckLocalView)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_INT(cp);
	ACTION_RETURN_BOOL(self->CheckLocalView(cp));
}

//============================================================================
//
// AActor :: IsInsideVisibleAngles
//
// Returns true if this actor is within viewing angle/pitch visibility. 
//
//============================================================================

bool AActor::IsInsideVisibleAngles() const
{
	// Don't bother masking if not wanted.
	if (!(renderflags & RF_MASKROTATION))
		return true;

	if (players[consoleplayer].camera == nullptr)
		return true;
	
	DAngle anglestart = VisibleStartAngle;
	DAngle angleend = VisibleEndAngle;
	DAngle pitchstart = VisibleStartPitch;
	DAngle pitchend = VisibleEndPitch;
	
	if (anglestart > angleend)
	{
		DAngle temp = anglestart;
		anglestart = angleend;
		angleend = temp;
	}

	if (pitchstart > pitchend)
	{
		DAngle temp = pitchstart;
		pitchstart = pitchend;
		pitchend = temp;
	}
	

	AActor *mo = players[consoleplayer].camera;

	if (mo != nullptr)
	{
		
		DVector3 diffang = ViewPos - Pos();
		DAngle to = diffang.Angle();

		if (!(renderflags & RF_ABSMASKANGLE)) 
			to = deltaangle(Angles.Yaw, to);

		if ((to >= anglestart && to <= angleend))
		{
			to = diffang.Pitch();
			if (!(renderflags & RF_ABSMASKPITCH))
				to = deltaangle(Angles.Pitch, to);

			return !!(to >= pitchstart && to <= pitchend);
		}
		else return false;
	}
	return true;
}

//============================================================================
//
// AActor :: IsVisibleToPlayer
//
// Returns true if this actor should be seen by the console player.
//
//============================================================================

bool AActor::IsVisibleToPlayer() const
{
	// [BB] Safety check. This should never be NULL. Nevertheless, we return true to leave the default ZDoom behavior unaltered.
	if ( players[consoleplayer].camera == NULL )
		return true;
 
	if (VisibleToTeam != 0 && teamplay &&
		(signed)(VisibleToTeam-1) != players[consoleplayer].userinfo.GetTeam() )
		return false;

	const player_t* pPlayer = players[consoleplayer].camera->player;

	if (pPlayer && pPlayer->mo && GetClass()->VisibleToPlayerClass.Size() > 0)
	{
		bool visible = false;
		for(unsigned int i = 0;i < GetClass()->VisibleToPlayerClass.Size();++i)
		{
			auto cls = GetClass()->VisibleToPlayerClass[i];
			if (cls && pPlayer->mo->GetClass()->IsDescendantOf(cls))
			{
				visible = true;
				break;
			}
		}
		if (!visible)
			return false;
	}

	// [BB] Passed all checks.
	return true;
}

//============================================================================
//
// AActor :: ConversationAnimation
//
// Plays a conversation-related animation:
//	 0 = greeting
//   1 = "yes"
//   2 = "no"
//
//============================================================================

void AActor::ConversationAnimation (int animnum)
{
	FState * state = NULL;
	switch (animnum)
	{
	case 0:
		state = FindState(NAME_Greetings);
		break;
	case 1:
		state = FindState(NAME_Yes);
		break;
	case 2:
		state = FindState(NAME_No);
		break;
	}
	if (state != NULL) SetState(state);
}

//============================================================================
//
// AActor :: Touch
//
// Something just touched this actor. Normally used only for inventory items,
// but some Strife monsters also use it.
//
//============================================================================

void AActor::Touch (AActor *toucher)
{
}

DEFINE_ACTION_FUNCTION(AActor, Touch)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT_NOT_NULL(toucher, AActor);
	self->Touch(toucher);
	return 0;
}

void AActor::CallTouch(AActor *toucher)
{
	IFVIRTUAL(AActor, Touch)
	{
		VMValue params[2] = { (DObject*)this, toucher };
		GlobalVMStack.Call(func, params, 2, nullptr, 0, nullptr);
	}
	else Touch(toucher);
}

//============================================================================
//
// AActor :: Grind
//
// Handles the an actor being crushed by a door, crusher or polyobject.
// Originally part of P_DoCrunch(), it has been made into its own actor
// function so that it could be called from a polyobject without hassle.
// Bool items is true if it should destroy() dropped items, false otherwise.
//============================================================================

bool AActor::Grind(bool items)
{
	// crunch bodies to giblets
	if ((flags & MF_CORPSE) && !(flags3 & MF3_DONTGIB) && (health <= 0))
	{
		FState * state = FindState(NAME_Crush);

		// In Heretic and Chex Quest we don't change the actor's sprite, just its size.
		if (state == NULL && gameinfo.dontcrunchcorpses)
		{
			flags &= ~MF_SOLID;
			flags3 |= MF3_DONTGIB;
			Height = 0;
			radius = 0;
			return false;
		}

		bool isgeneric = false;
		// ZDoom behavior differs from standard as crushed corpses cannot be raised.
		// The reason for the change was originally because of a problem with players,
		// see rh_log entry for February 21, 1999. Don't know if it is still relevant.
		if (state == NULL 									// Only use the default crushed state if:
			&& !(flags & MF_NOBLOOD)						// 1. the monster bleeeds,
			&& (i_compatflags & COMPATF_CORPSEGIBS)			// 2. the compat setting is on,
			&& player == NULL)								// 3. and the thing isn't a player.
		{
			isgeneric = true;
			state = FindState(NAME_GenericCrush);
			if (state != NULL && (sprites[state->sprite].numframes <= 0))
				state = NULL; // If one of these tests fails, do not use that state.
		}
		if (state != NULL && !(flags & MF_ICECORPSE))
		{
			if (this->flags4 & MF4_BOSSDEATH) 
			{
				A_BossDeath(this);
			}
			flags &= ~MF_SOLID;
			flags3 |= MF3_DONTGIB;
			Height = 0;
			radius = 0;
			SetState (state);
			if (isgeneric)	// Not a custom crush state, so colorize it appropriately.
			{
				S_Sound (this, CHAN_BODY, "misc/fallingsplat", 1, ATTN_IDLE);
				Translation = BloodTranslation;
			}
			return false;
		}
		if (!(flags & MF_NOBLOOD))
		{
			if (this->flags4 & MF4_BOSSDEATH) 
			{
				A_BossDeath(this);
			}

			PClassActor *i = PClass::FindActor("RealGibs");

			if (i != NULL)
			{
				i = i->GetReplacement();

				const AActor *defaults = GetDefaultByType (i);
				if (defaults->SpawnState == NULL ||
					sprites[defaults->SpawnState->sprite].numframes == 0)
				{ 
					i = NULL;
				}
			}
			if (i == NULL)
			{
				// if there's no gib sprite don't crunch it.
				flags &= ~MF_SOLID;
				flags3 |= MF3_DONTGIB;
				Height = 0;
				radius = 0;
				return false;
			}

			AActor *gib = Spawn (i, Pos(), ALLOW_REPLACE);
			if (gib != NULL)
			{
				gib->RenderStyle = RenderStyle;
				gib->Alpha = Alpha;
				gib->Height = 0;
				gib->radius = 0;
				gib->Translation = BloodTranslation;
			}
			S_Sound (this, CHAN_BODY, "misc/fallingsplat", 1, ATTN_IDLE);
		}
		if (flags & MF_ICECORPSE)
		{
			tics = 1;
			Vel.Zero();
		}
		else if (player)
		{
			flags |= MF_NOCLIP;
			flags3 |= MF3_DONTGIB;
			renderflags |= RF_INVISIBLE;
		}
		else
		{
			Destroy ();
		}
		return false;		// keep checking
	}

	// killough 11/98: kill touchy things immediately
	if (flags6 & MF6_TOUCHY && (flags6 & MF6_ARMED || IsSentient()))
    {
		flags6 &= ~MF6_ARMED; // Disarm
		P_DamageMobj (this, NULL, NULL, health, NAME_Crush, DMG_FORCED);  // kill object
		return true;   // keep checking
    }

	if (!(flags & MF_SOLID) || (flags & MF_NOCLIP))
	{
		return false;
	}

	if (!(flags & MF_SHOOTABLE))
	{
		return false;		// assume it is bloody gibs or something
	}
	return true;
}

//============================================================================
//
// AActor :: Massacre
//
// Called by the massacre cheat to kill monsters. Returns true if the monster
// was killed and false if it was already dead.
//============================================================================

bool AActor::Massacre ()
{
	int prevhealth;

	if (health > 0)
	{
		auto f = flags;
		auto f2 = flags2;

		flags |= MF_SHOOTABLE;
		flags2 &= ~(MF2_DORMANT|MF2_INVULNERABLE);
		do
		{
			prevhealth = health;
			P_DamageMobj (this, NULL, NULL, TELEFRAG_DAMAGE, NAME_Massacre);
		}
		while (health != prevhealth && health > 0);	//abort if the actor wasn't hurt.
		if (health > 0)
		{
			// restore flags if this did not kill the monster.
			flags = f;
			flags2 = f2;
		}
		return health <= 0;
	}
	return false;
}

//----------------------------------------------------------------------------
//
// PROC P_ExplodeMissile
//
//----------------------------------------------------------------------------

void P_ExplodeMissile (AActor *mo, line_t *line, AActor *target, bool onsky)
{
	if (mo->flags3 & MF3_EXPLOCOUNT)
	{
		if (++mo->threshold < mo->DefThreshold)
		{
			return;
		}
	}
	mo->Vel.Zero();
	mo->effects = 0;		// [RH]
	mo->flags &= ~MF_SHOOTABLE;
	
	FState *nextstate = nullptr;
	
	if (target != nullptr)
	{
		if (mo->flags7 & MF7_HITTARGET)	mo->target = target;
		if (mo->flags7 & MF7_HITMASTER)	mo->master = target;
		if (mo->flags7 & MF7_HITTRACER)	mo->tracer = target;
		if ((target->flags & (MF_SHOOTABLE | MF_CORPSE)) || (target->flags6 & MF6_KILLED))
		{
			if (target->flags & MF_NOBLOOD) nextstate = mo->FindState(NAME_Crash);
			if (nextstate == NULL) nextstate = mo->FindState(NAME_Death, NAME_Extreme);
		}
	}
	if (nextstate == NULL) nextstate = mo->FindState(NAME_Death);
	
	if (onsky || (line != NULL && line->special == Line_Horizon))
	{
		if (!(mo->flags3 & MF3_SKYEXPLODE))
		{
			// [RH] Don't explode missiles on horizon lines.
			mo->Destroy();
			return;
		}
		nextstate = mo->FindState(NAME_Death, NAME_Sky);
	}

	if (line != NULL && cl_missiledecals)
	{
		DVector3 pos = mo->PosRelative(line);
		int side = P_PointOnLineSidePrecise (pos, line);
		if (line->sidedef[side] == NULL)
			side ^= 1;
		if (line->sidedef[side] != NULL)
		{
			FDecalBase *base = mo->DecalGenerator;
			if (base != NULL)
			{
				// Find the nearest point on the line, and stick a decal there
				DVector3 linepos;
				double den, frac;

				den = line->Delta().LengthSquared();
				if (den != 0)
				{
					frac = clamp<double>((mo->Pos().XY() - line->v1->fPos()) | line->Delta(), 0, den) / den;

					linepos = DVector3(line->v1->fPos() + line->Delta() * frac, pos.Z);

					F3DFloor * ffloor=NULL;
					if (line->sidedef[side^1] != NULL)
					{
						sector_t * backsector = line->sidedef[side^1]->sector;
						extsector_t::xfloor &xf = backsector->e->XFloor;
						// find a 3D-floor to stick to
						for(unsigned int i=0;i<xf.ffloors.Size();i++)
						{
							F3DFloor * rover=xf.ffloors[i];

							if ((rover->flags&(FF_EXISTS|FF_SOLID|FF_RENDERSIDES))==(FF_EXISTS|FF_SOLID|FF_RENDERSIDES))
							{
								if (pos.Z <= rover->top.plane->ZatPoint(linepos) && pos.Z >= rover->bottom.plane->ZatPoint(linepos))
								{
									ffloor = rover;
									break;
								}
							}
						}
					}

					DImpactDecal::StaticCreate(base->GetDecal(), linepos, line->sidedef[side], ffloor);
				}
			}
		}
	}

	// play the sound before changing the state, so that AActor::OnDestroy can call S_RelinkSounds on it and the death state can override it.
	if (mo->DeathSound)
	{
		S_Sound (mo, CHAN_VOICE, mo->DeathSound, 1,
			(mo->flags3 & MF3_FULLVOLDEATH) ? ATTN_NONE : ATTN_NORM);
	}

	mo->SetState (nextstate);
	if (!(mo->ObjectFlags & OF_EuthanizeMe))
	{
		// The rest only applies if the missile actor still exists.
		// [RH] Change render style of exploding rockets
		if (mo->flags5 & MF5_DEHEXPLOSION)
		{
			if (deh.ExplosionStyle == 255)
			{
				if (addrocketexplosion)
				{
					mo->RenderStyle = STYLE_Add;
					mo->Alpha = 1.;
				}
				else
				{
					mo->RenderStyle = STYLE_Translucent;
					mo->Alpha = 0.6666;
				}
			}
			else
			{
				mo->RenderStyle = ERenderStyle(deh.ExplosionStyle);
				mo->Alpha = deh.ExplosionAlpha;
			}
		}

		if (mo->flags4 & MF4_RANDOMIZE)
		{
			mo->tics -= (pr_explodemissile() & 3) * TICRATE / 35;
			if (mo->tics < 1)
				mo->tics = 1;
		}

		mo->flags &= ~MF_MISSILE;

	}
}

DEFINE_ACTION_FUNCTION(AActor, ExplodeMissile)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_POINTER_DEF(line, line_t);
	PARAM_OBJECT_DEF(target, AActor);
	P_ExplodeMissile(self, line, target);
	return 0;
}


void AActor::PlayBounceSound(bool onfloor)
{
	if (!onfloor && (BounceFlags & BOUNCE_NoWallSound))
	{
		return;
	}

	if (!(BounceFlags & BOUNCE_Quiet))
	{
		if (BounceFlags & BOUNCE_UseSeeSound)
		{
			S_Sound (this, CHAN_VOICE, SeeSound, 1, ATTN_IDLE);
		}
		else if (onfloor || WallBounceSound <= 0)
		{
			S_Sound (this, CHAN_VOICE, BounceSound, 1, ATTN_IDLE);
		}
		else
		{
			S_Sound (this, CHAN_VOICE, WallBounceSound, 1, ATTN_IDLE);
		}
	}
}

//----------------------------------------------------------------------------
//
// PROC P_FloorBounceMissile
//
// Returns true if the missile was destroyed
//----------------------------------------------------------------------------

bool AActor::FloorBounceMissile (secplane_t &plane)
{
	if (Z() <= floorz && P_HitFloor (this))
	{
		// Landed in some sort of liquid
		if (BounceFlags & BOUNCE_ExplodeOnWater)
		{
			if (flags & MF_MISSILE)
				P_ExplodeMissile(this, NULL, NULL);
			else
				CallDie(NULL, NULL);
			return true;
		}
		if (!(BounceFlags & BOUNCE_CanBounceWater))
		{
			Destroy ();
			return true;
		}
	}

	if (plane.fC() < 0)
	{ // on ceiling
		if (!(BounceFlags & BOUNCE_Ceilings))
			return true;
	}
	else
	{ // on floor
		if (!(BounceFlags & BOUNCE_Floors))
			return true;
	}

	// The amount of bounces is limited
	if (bouncecount>0 && --bouncecount==0)
	{
		if (flags & MF_MISSILE)
			P_ExplodeMissile(this, NULL, NULL);
		else
			CallDie(NULL, NULL);
		return true;
	}

	double dot = (Vel | plane.Normal()) * 2;

	if (BounceFlags & (BOUNCE_HereticType | BOUNCE_MBF))
	{
		Vel -= plane.Normal() * dot;
		AngleFromVel();
		if (!(BounceFlags & BOUNCE_MBF)) // Heretic projectiles die, MBF projectiles don't.
		{
			flags |= MF_INBOUNCE;
			SetState (FindState(NAME_Death));
			flags &= ~MF_INBOUNCE;
			return false;
		}
		else Vel.Z *= bouncefactor;
	}
	else // Don't run through this for MBF-style bounces
	{
		// The reflected velocity keeps only about 70% of its original speed
		Vel = (Vel - plane.Normal() * dot) * bouncefactor;
		AngleFromVel();
	}

	PlayBounceSound(true);

	// Set bounce state
	if (BounceFlags & BOUNCE_UseBounceState)
	{
		FName names[2];
		FState *bouncestate;

		names[0] = NAME_Bounce;
		names[1] = plane.fC() < 0 ? NAME_Ceiling : NAME_Floor;
		bouncestate = FindState(2, names);
		if (bouncestate != NULL)
		{
			SetState(bouncestate);
		}
	}

	if (BounceFlags & BOUNCE_MBF) // Bring it to rest below a certain speed
	{
		if (fabs(Vel.Z) < Mass * GetGravity() / 64)
			Vel.Z = 0;
	}
	else if (BounceFlags & (BOUNCE_AutoOff|BOUNCE_AutoOffFloorOnly))
	{
		if (plane.fC() > 0 || (BounceFlags & BOUNCE_AutoOff))
		{
			// AutoOff only works when bouncing off a floor, not a ceiling (or in compatibility mode.)
			if (!(flags & MF_NOGRAVITY) && (Vel.Z < 3))
				BounceFlags &= ~BOUNCE_TypeMask;
		}
	}
	return false;
}

//----------------------------------------------------------------------------
//
// FUNC P_FaceMobj
//
// Returns 1 if 'source' needs to turn clockwise, or 0 if 'source' needs
// to turn counter clockwise.  'delta' is set to the amount 'source'
// needs to turn.
//
//----------------------------------------------------------------------------

int P_FaceMobj (AActor *source, AActor *target, DAngle *delta)
{
	DAngle diff;

	diff = deltaangle(source->Angles.Yaw, source->AngleTo(target));
	if (diff > 0)
	{
		*delta = diff;
		return 1;
	}
	else
	{
		*delta = -diff;
		return 0;
	}
}

//----------------------------------------------------------------------------
//
// CanSeek
//
// Checks if a seeker missile can home in on its target
//
//----------------------------------------------------------------------------

bool AActor::CanSeek(AActor *target) const
{
	if (target->flags5 & MF5_CANTSEEK) return false;
	if ((flags2 & MF2_DONTSEEKINVISIBLE) && 
		((target->flags & MF_SHADOW) || 
		 (target->renderflags & RF_INVISIBLE) || 
		 !target->RenderStyle.IsVisible(target->Alpha)
		)
	   ) return false;
	return true;
}

DEFINE_ACTION_FUNCTION(AActor, CanSeek)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT_NOT_NULL(target, AActor);
	ACTION_RETURN_BOOL(self->CanSeek(target));
}

//----------------------------------------------------------------------------
//
// FUNC P_SeekerMissile
//
// The missile's tracer field must be the target.  Returns true if
// target was tracked, false if not.
//
//----------------------------------------------------------------------------

bool P_SeekerMissile (AActor *actor, double thresh, double turnMax, bool precise, bool usecurspeed)
{
	int dir;
	DAngle delta;
	AActor *target;
	double speed;

	speed = !usecurspeed ? actor->Speed : actor->VelToSpeed();
	target = actor->tracer;
	if (target == NULL || !actor->CanSeek(target))
	{
		return false;
	}
	if (!(target->flags & MF_SHOOTABLE))
	{ // Target died
		actor->tracer = NULL;
		return false;
	}
	if (speed == 0)
	{ // Technically, we're not seeking since our speed is 0, but the target *is* seekable.
		return true;
	}
	dir = P_FaceMobj (actor, target, &delta);
	if (delta > thresh)
	{
		delta /= 2;
		if (delta > turnMax)
		{
			delta = turnMax;
		}
	}
	if (dir)
	{ // Turn clockwise
		actor->Angles.Yaw += delta;
	}
	else
	{ // Turn counter clockwise
		actor->Angles.Yaw -= delta;
	}
	
	if (!precise)
	{
		actor->VelFromAngle(speed);

		if (!(actor->flags3 & (MF3_FLOORHUGGER|MF3_CEILINGHUGGER)))
		{
			if (actor->Top() < target->Z() ||
				target->Top() < actor->Z())
			{ // Need to seek vertically
				actor->Vel.Z = (target->Center() - actor->Center()) / actor->DistanceBySpeed(target, speed);
			}
		}
	}
	else
	{
		DAngle pitch = 0.;
		if (!(actor->flags3 & (MF3_FLOORHUGGER|MF3_CEILINGHUGGER)))
		{ // Need to seek vertically
			double dist = MAX(1., actor->Distance2D(target));
			// Aim at a player's eyes and at the middle of the actor for everything else.
			double aimheight = target->Height/2;
			if (target->IsKindOf(RUNTIME_CLASS(APlayerPawn)))
			{
				aimheight = static_cast<APlayerPawn *>(target)->ViewHeight;
			}
			pitch = DVector2(dist, target->Z() + aimheight - actor->Center()).Angle();
		}
		actor->Vel3DFromAngle(-pitch, speed);
	}

	return true;
}


//
// P_XYMovement
//
// Returns the actor's old floorz.
//
#define STOPSPEED			(0x1000/65536.)
#define CARRYSTOPSPEED		((0x1000*32/3)/65536.)

double P_XYMovement (AActor *mo, DVector2 scroll) 
{
	static int pushtime = 0;
	bool bForceSlide = !scroll.isZero();
	DAngle Angle;
	DVector2 ptry;
	player_t *player;
	DVector2 move;
	const secplane_t * walkplane;
	static const double windTab[3] = { 5 / 32., 10 / 32., 25 / 32. };
	int steps, step, totalsteps;
	DVector2 start;
	double Oldfloorz = mo->floorz;
	double oldz = mo->Z();

	double maxmove = (mo->waterlevel < 1) || (mo->flags & MF_MISSILE) || 
					  (mo->player && mo->player->crouchoffset<-10) ? MAXMOVE : MAXMOVE/4;

	if (mo->flags2 & MF2_WINDTHRUST && mo->waterlevel < 2 && !(mo->flags & MF_NOCLIP))
	{
		int special = mo->Sector->special;
		switch (special)
		{
			case 40: case 41: case 42: // Wind_East
				mo->Thrust(0., windTab[special-40]);
				break;
			case 43: case 44: case 45: // Wind_North
				mo->Thrust(90., windTab[special-43]);
				break;
			case 46: case 47: case 48: // Wind_South
				mo->Thrust(270., windTab[special-46]);
				break;
			case 49: case 50: case 51: // Wind_West
				mo->Thrust(180., windTab[special-49]);
				break;
		}
	}

	// [RH] No need to clamp these now. However, wall running needs it so
	// that large thrusts can't propel an actor through a wall, because wall
	// running depends on the player's original movement continuing even after
	// it gets blocked.
	if ((mo->player != NULL && (i_compatflags & COMPATF_WALLRUN)) || (mo->waterlevel >= 1) ||
		(mo->player != NULL && mo->player->crouchfactor < 0.75))
	{
		// preserve the direction instead of clamping x and y independently.
		double cx = mo->Vel.X == 0 ? 1. : clamp(mo->Vel.X, -maxmove, maxmove) / mo->Vel.X;
		double cy = mo->Vel.Y == 0 ? 1. : clamp(mo->Vel.Y, -maxmove, maxmove) / mo->Vel.Y;
		double fac = MIN(cx, cy);

		mo->Vel.X *= fac;
		mo->Vel.Y *= fac;
	}
	const double VELOCITY_THRESHOLD = 5000;	// don't let it move faster than this. Fixed point overflowed at 32768 but that's too much to make this safe.
	if (mo->Vel.LengthSquared() >= VELOCITY_THRESHOLD*VELOCITY_THRESHOLD)
	{
		mo->Vel.MakeResize(VELOCITY_THRESHOLD);
	}
	move = mo->Vel;
	// [RH] Carrying sectors didn't work with low speeds in BOOM. This is
	// because BOOM relied on the speed being fast enough to accumulate
	// despite friction. If the speed is too low, then its movement will get
	// cancelled, and it won't accumulate to the desired speed.
	mo->flags4 &= ~MF4_SCROLLMOVE;
	if (fabs(scroll.X) > CARRYSTOPSPEED)
	{
		scroll.X *= CARRYFACTOR;
		mo->Vel.X += scroll.X;
		mo->flags4 |= MF4_SCROLLMOVE;
	}
	if (fabs(scroll.Y) > CARRYSTOPSPEED)
	{
		scroll.Y *= CARRYFACTOR;
		mo->Vel.Y += scroll.Y;
		mo->flags4 |= MF4_SCROLLMOVE;
	}
	move += scroll;

	if (move.isZero())
	{
		if (mo->flags & MF_SKULLFLY)
		{
			// the skull slammed into something
			mo->flags &= ~MF_SKULLFLY;
			mo->Vel.Zero();
			if (!(mo->flags2 & MF2_DORMANT))
			{
				if (mo->SeeState != NULL) mo->SetState (mo->SeeState);
				else mo->SetIdle();
			}
			else
			{
				mo->SetIdle();
				mo->tics = -1;
			}
		}
		return Oldfloorz;
	}

	player = mo->player;

	// [RH] Adjust player movement on sloped floors
	DVector2 startmove = move;
	walkplane = P_CheckSlopeWalk (mo, move);

	// [RH] Take smaller steps when moving faster than the object's size permits.
	// Moving as fast as the object's "diameter" is bad because it could skip
	// some lines because the actor could land such that it is just touching the
	// line. For Doom to detect that the line is there, it needs to actually cut
	// through the actor.

	{
		double maxmove = mo->radius - 1;

		if (maxmove <= 0)
		{ // gibs can have radius 0, so don't divide by zero below!
			maxmove = MAXMOVE;
		}

		const double xspeed = fabs (move.X);
		const double yspeed = fabs (move.Y);

		steps = 1;

		if (xspeed > yspeed)
		{
			if (xspeed > maxmove)
			{
				steps = int(1 + xspeed / maxmove);
			}
		}
		else
		{
			if (yspeed > maxmove)
			{
				steps = int(1 + yspeed / maxmove);
			}
		}
	}

	// P_SlideMove needs to know the step size before P_CheckSlopeWalk
	// because it also calls P_CheckSlopeWalk on its clipped steps.
	DVector2 onestep = startmove / steps;

	start = mo->Pos();
	step = 1;
	totalsteps = steps;

	// [RH] Instead of doing ripping damage each step, do it each tic.
	// This makes it compatible with Heretic and Hexen, which only did
	// one step for their missiles with ripping damage (excluding those
	// that don't use P_XYMovement). It's also more intuitive since it
	// makes the damage done dependant on the amount of time the projectile
	// spends inside a target rather than on the projectile's size. The
	// last actor ripped through is recorded so that if the projectile
	// passes through more than one actor this tic, each one takes damage
	// and not just the first one.
	pushtime++;

	FCheckPosition tm(!!(mo->flags2 & MF2_RIP));

	DAngle oldangle = mo->Angles.Yaw;
	do
	{
		if (i_compatflags & COMPATF_WALLRUN) pushtime++;
		tm.PushTime = pushtime;

		ptry = start + move * step / steps;

		DVector2 startvel = mo->Vel;

		// killough 3/15/98: Allow objects to drop off
		// [RH] If walking on a slope, stay on the slope
		if (!P_TryMove (mo, ptry, true, walkplane, tm))
		{
			// blocked move
			AActor *BlockingMobj = mo->BlockingMobj;
			line_t *BlockingLine = mo->BlockingLine;

			if (!(mo->flags & MF_MISSILE) && (mo->BounceFlags & BOUNCE_MBF) 
				&& (BlockingMobj != NULL ? P_BounceActor(mo, BlockingMobj, false) : P_BounceWall(mo)))
			{
				// Do nothing, relevant actions already done in the condition.
				// This allows to avoid setting velocities to 0 in the final else of this series.
			}
			else if ((mo->flags2 & (MF2_SLIDE|MF2_BLASTED) || bForceSlide) && !(mo->flags&MF_MISSILE))
			{	// try to slide along it
				if (BlockingMobj == NULL)
				{ // slide against wall
					if (BlockingLine != NULL &&
						mo->player && mo->waterlevel && mo->waterlevel < 3 &&
						(mo->player->cmd.ucmd.forwardmove | mo->player->cmd.ucmd.sidemove) &&
						mo->BlockingLine->sidedef[1] != NULL)
					{
						mo->Vel.Z = WATER_JUMP_SPEED;
					}
					// If the blocked move executed any push specials that changed the
					// actor's velocity, do not attempt to slide.
					if (mo->Vel.XY() == startvel)
					{
						if (player && (i_compatflags & COMPATF_WALLRUN))
						{
						// [RH] Here is the key to wall running: The move is clipped using its full speed.
						// If the move is done a second time (because it was too fast for one move), it
						// is still clipped against the wall at its full speed, so you effectively
						// execute two moves in one tic.
							P_SlideMove (mo, mo->Vel, 1);
						}
						else
						{
							P_SlideMove (mo, onestep, totalsteps);
						}
						if (mo->Vel.XY().isZero())
						{
							steps = 0;
						}
						else
						{
							if (!player || !(i_compatflags & COMPATF_WALLRUN))
							{
								move = mo->Vel;
								onestep = move / steps;
								P_CheckSlopeWalk (mo, move);
							}
							start = mo->Pos().XY() - move * step / steps;
						}
					}
					else
					{
						steps = 0;
					}
				}
				else
				{ // slide against another actor
					DVector2 t;
					t.X = 0, t.Y = onestep.Y;
					walkplane = P_CheckSlopeWalk (mo, t);
					if (P_TryMove (mo, mo->Pos() + t, true, walkplane, tm))
					{
						mo->Vel.X = 0;
					}
					else
					{
						t.X = onestep.X, t.Y = 0;
						walkplane = P_CheckSlopeWalk (mo, t);
						if (P_TryMove (mo, mo->Pos() + t, true, walkplane, tm))
						{
							mo->Vel.Y = 0;
						}
						else
						{
							mo->Vel.X = mo->Vel.Y = 0;
						}
					}
					if (player && player->mo == mo)
					{
						if (mo->Vel.X == 0)
							player->Vel.X = 0;
						if (mo->Vel.Y == 0)
							player->Vel.Y = 0;
					}
					steps = 0;
				}
			}
			else if (mo->flags & MF_MISSILE)
			{
				steps = 0;
				if (BlockingMobj)
				{
					if (mo->BounceFlags & BOUNCE_Actors)
					{
						// Bounce test and code moved to P_BounceActor
						if (!P_BounceActor(mo, BlockingMobj, false))
						{	// Struck a player/creature
							P_ExplodeMissile (mo, NULL, BlockingMobj);
						}
						return Oldfloorz;
					}
				}
				else
				{
					// Struck a wall
					if (P_BounceWall (mo))
					{
						mo->PlayBounceSound(false);
						return Oldfloorz;
					}
				}
				if (BlockingMobj && (BlockingMobj->flags2 & MF2_REFLECTIVE))
				{
					bool seeker = (mo->flags2 & MF2_SEEKERMISSILE) ? true : false;
					// Don't change the angle if there's THRUREFLECT on the monster.
					if (!(BlockingMobj->flags7 & MF7_THRUREFLECT))
					{
						DAngle angle = BlockingMobj->AngleTo(mo);
						bool dontReflect = (mo->AdjustReflectionAngle(BlockingMobj, angle));
						// Change angle for deflection/reflection

						if (!dontReflect)
						{
							bool tg = (mo->target != NULL);
							bool blockingtg = (BlockingMobj->target != NULL);
							if ((BlockingMobj->flags7 & MF7_AIMREFLECT) && (tg | blockingtg))
							{
								AActor *origin = tg ? mo->target : BlockingMobj->target;

								//dest->x - source->x
								DVector3 vect = mo->Vec3To(origin);
								vect.Z += origin->Height / 2;
								mo->Vel = vect.Resized(mo->Speed);
							}
							else
							{
								if ((BlockingMobj->flags7 & MF7_MIRRORREFLECT) && (tg | blockingtg))
								{
									mo->Angles.Yaw += 180.;
									mo->Vel *= -.5;
								}
								else
								{
									mo->Angles.Yaw = angle;
									mo->VelFromAngle(mo->Speed / 2);
									mo->Vel.Z *= -.5;
								}
							}
						}
						else
						{
							goto explode;
						}						
					}
					if (mo->flags2 & MF2_SEEKERMISSILE)
					{
						mo->tracer = mo->target;
					}
					mo->target = BlockingMobj;
					return Oldfloorz;
				}
explode:
				// explode a missile
				bool onsky = false;
					if (tm.ceilingline &&
						tm.ceilingline->backsector &&
						tm.ceilingline->backsector->GetTexture(sector_t::ceiling) == skyflatnum &&
						mo->Z() >= tm.ceilingline->backsector->ceilingplane.ZatPoint(mo->PosRelative(tm.ceilingline)))
					{
						if (!(mo->flags3 & MF3_SKYEXPLODE))
						{
							// Hack to prevent missiles exploding against the sky.
							// Does not handle sky floors.
							mo->Destroy();
							return Oldfloorz;
						}
						else onsky = true;
					}
					// [RH] Don't explode on horizon lines.
					if (mo->BlockingLine != NULL && mo->BlockingLine->special == Line_Horizon)
					{
						if (!(mo->flags3 & MF3_SKYEXPLODE))
						{
							mo->Destroy();
							return Oldfloorz;
						}
						else onsky = true;
					}
				P_ExplodeMissile (mo, mo->BlockingLine, BlockingMobj, onsky);
				return Oldfloorz;
			}
			else
			{
				mo->Vel.X = mo->Vel.Y = 0;
				steps = 0;
			}
		}
		else
		{
			if (mo->Pos().XY() != ptry)
			{
				// If the new position does not match the desired position, the player
				// must have gone through a teleporter or portal.
				
				if (mo->Vel.X == 0 && mo->Vel.Y == 0)
				{
					// Stop moving right now if it was a regular teleporter.
					step = steps;
				}
				else
				{
					// It was a portal, line-to-line or fogless teleporter, so the move should continue.
					// For that we need to adjust the start point, and the movement vector.
					DAngle anglediff = deltaangle(oldangle, mo->Angles.Yaw);

					if (anglediff != 0)
					{
						move = move.Rotated(anglediff);
						oldangle = mo->Angles.Yaw;
					}
					start = mo->Pos() - move * step / steps;
				}
			}
		}
	} while (++step <= steps);

	// Friction

	if (player && player->mo == mo && player->cheats & CF_NOVELOCITY)
	{ // debug option for no sliding at all
		mo->Vel.X = mo->Vel.Y = 0;
		player->Vel.X = player->Vel.Y = 0;
		return Oldfloorz;
	}

	if (mo->flags & (MF_MISSILE | MF_SKULLFLY))
	{ // no friction for missiles
		return Oldfloorz;
	}

	if (mo->Z() > mo->floorz && !(mo->flags2 & MF2_ONMOBJ) &&
		!mo->IsNoClip2() &&
		(!(mo->flags2 & MF2_FLY) || !(mo->flags & MF_NOGRAVITY)) &&
		!mo->waterlevel)
	{ // [RH] Friction when falling is available for larger aircontrols
		if (player != NULL && level.airfriction != 1.)
		{
			mo->Vel.X *= level.airfriction;
			mo->Vel.Y *= level.airfriction;

			if (player->mo == mo)		//  Not voodoo dolls
			{
				player->Vel.X *= level.airfriction;
				player->Vel.Y *= level.airfriction;
			}
		}
		return Oldfloorz;
	}

	// killough 8/11/98: add bouncers
	// killough 9/15/98: add objects falling off ledges
	// killough 11/98: only include bouncers hanging off ledges
	if ((mo->flags & MF_CORPSE) || (mo->BounceFlags & BOUNCE_MBF && mo->Z() > mo->dropoffz) || (mo->flags6 & MF6_FALLING))
	{ // Don't stop sliding if halfway off a step with some velocity
		if (fabs(mo->Vel.X) > 0.25 || fabs(mo->Vel.Y) > 0.25)
		{
			if (mo->floorz > mo->Sector->floorplane.ZatPoint(mo))
			{
				if (mo->dropoffz != mo->floorz) // 3DMidtex or other special cases that must be excluded
				{
					unsigned i;
					for(i=0;i<mo->Sector->e->XFloor.ffloors.Size();i++)
					{
						// Sliding around on 3D floors looks extremely bad so
						// if the floor comes from one in the current sector stop sliding the corpse!
						F3DFloor * rover=mo->Sector->e->XFloor.ffloors[i];
						if (!(rover->flags&FF_EXISTS)) continue;
						if (rover->flags&FF_SOLID && rover->top.plane->ZatPoint(mo) == mo->floorz) break;
					}
					if (i==mo->Sector->e->XFloor.ffloors.Size()) 
						return Oldfloorz;
				}
			}
		}
	}

	// killough 11/98:
	// Stop voodoo dolls that have come to rest, despite any
	// moving corresponding player:
	if (fabs(mo->Vel.X) < STOPSPEED && fabs(mo->Vel.Y) < STOPSPEED
		&& (!player || (player->mo != mo)
			|| !(player->cmd.ucmd.forwardmove | player->cmd.ucmd.sidemove)))
	{
		// if in a walking frame, stop moving
		// killough 10/98:
		// Don't affect main player when voodoo dolls stop:
		if (player && player->mo == mo && !(player->cheats & CF_PREDICTING))
		{
			player->mo->PlayIdle ();
		}

		mo->Vel.X = mo->Vel.Y = 0;
		mo->flags4 &= ~MF4_SCROLLMOVE;

		// killough 10/98: kill any bobbing velocity too (except in voodoo dolls)
		if (player && player->mo == mo)
			player->Vel.X = player->Vel.Y = 0;
	}
	else
	{
		// phares 3/17/98
		// Friction will have been adjusted by friction thinkers for icy
		// or muddy floors. Otherwise it was never touched and
		// remained set at ORIG_FRICTION
		//
		// killough 8/28/98: removed inefficient thinker algorithm,
		// instead using touching_sectorlist in P_GetFriction() to
		// determine friction (and thus only when it is needed).
		//
		// killough 10/98: changed to work with new bobbing method.
		// Reducing player velocity is no longer needed to reduce
		// bobbing, so ice works much better now.

		double friction = P_GetFriction (mo, NULL);

		mo->Vel.X *= friction;
		mo->Vel.Y *= friction;

		// killough 10/98: Always decrease player bobbing by ORIG_FRICTION.
		// This prevents problems with bobbing on ice, where it was not being
		// reduced fast enough, leading to all sorts of kludges being developed.

		if (player && player->mo == mo)		//  Not voodoo dolls
		{
			player->Vel.X *= ORIG_FRICTION;
			player->Vel.Y *= ORIG_FRICTION;
		}

		// Don't let the velocity become less than the smallest representable fixed point value.
		if (fabs(mo->Vel.X) < MinVel) mo->Vel.X = 0;
		if (fabs(mo->Vel.Y) < MinVel) mo->Vel.Y = 0;
		if (player && player->mo == mo)		//  Not voodoo dolls
		{
			if (fabs(player->Vel.X) < MinVel) player->Vel.X = 0;
			if (fabs(player->Vel.Y) < MinVel) player->Vel.Y = 0;
		}
	}
	return Oldfloorz;
}

// Move this to p_inter ***
void P_MonsterFallingDamage (AActor *mo)
{
	int damage;
	double vel;

	if (!(level.flags2 & LEVEL2_MONSTERFALLINGDAMAGE))
		return;
	if (mo->floorsector->Flags & SECF_NOFALLINGDAMAGE)
		return;

	vel = fabs(mo->Vel.Z);
	if (vel > 35)
	{ // automatic death
		damage = TELEFRAG_DAMAGE;
	}
	else
	{
		damage = int((vel - 23)*6);
	}
	damage = TELEFRAG_DAMAGE;	// always kill 'em
	P_DamageMobj (mo, NULL, NULL, damage, NAME_Falling);
}

//
// P_ZMovement
//

void P_ZMovement (AActor *mo, double oldfloorz)
{
	double dist;
	double delta;
	double oldz = mo->Z();
	double grav = mo->GetGravity();

//
// check for smooth step up
//
	if (mo->player && mo->player->mo == mo && mo->Z() < mo->floorz)
	{
		mo->player->viewheight -= mo->floorz - mo->Z();
		mo->player->deltaviewheight = mo->player->GetDeltaViewHeight();
	}

	mo->AddZ(mo->Vel.Z);

//
// apply gravity
//
	if (mo->Z() > mo->floorz && !(mo->flags & MF_NOGRAVITY))
	{
		double startvelz = mo->Vel.Z;

		if (mo->waterlevel == 0 || (mo->player &&
			!(mo->player->cmd.ucmd.forwardmove | mo->player->cmd.ucmd.sidemove)))
		{
			// [RH] Double gravity only if running off a ledge. Coming down from
			// an upward thrust (e.g. a jump) should not double it.
			if (mo->Vel.Z == 0 && oldfloorz > mo->floorz && mo->Z() == oldfloorz)
			{
				mo->Vel.Z -= grav + grav;
			}
			else
			{
				mo->Vel.Z -= grav;
			}
		}
		if (mo->player == NULL)
		{
			if (mo->waterlevel >= 1)
			{
				double sinkspeed;

				if ((mo->flags & MF_SPECIAL) && !(mo->flags3 & MF3_ISMONSTER))
				{ // Pickup items don't sink if placed and drop slowly if dropped
					sinkspeed = (mo->flags & MF_DROPPED) ? -WATER_SINK_SPEED / 8 : 0;
				}
				else
				{
					sinkspeed = -WATER_SINK_SPEED;

					// If it's not a player, scale sinkspeed by its mass, with
					// 100 being equivalent to a player.
					if (mo->player == NULL)
					{
						sinkspeed = sinkspeed * clamp(mo->Mass, 1, 4000) / 100;
					}
				}
				if (mo->Vel.Z < sinkspeed)
				{ // Dropping too fast, so slow down toward sinkspeed.
					mo->Vel.Z -= MAX(sinkspeed*2, -8.);
					if (mo->Vel.Z > sinkspeed)
					{
						mo->Vel.Z = sinkspeed;
					}
				}
				else if (mo->Vel.Z > sinkspeed)
				{ // Dropping too slow/going up, so trend toward sinkspeed.
					mo->Vel.Z = startvelz + MAX(sinkspeed/3, -8.);
					if (mo->Vel.Z < sinkspeed)
					{
						mo->Vel.Z = sinkspeed;
					}
				}
			}
		}
		else
		{
			if (mo->waterlevel > 1)
			{
				double sinkspeed = -WATER_SINK_SPEED;

				if (mo->Vel.Z < sinkspeed)
				{
					mo->Vel.Z = (startvelz < sinkspeed) ? startvelz : sinkspeed;
				}
				else
				{
					mo->Vel.Z = startvelz + ((mo->Vel.Z - startvelz) *
						(mo->waterlevel == 1 ? WATER_SINK_SMALL_FACTOR : WATER_SINK_FACTOR));
				}
			}
		}
	}

	// Hexen compatibility handling for floatbobbing. Ugh...
	// Hexen yanked all items to the floor, except those being spawned at map start in the air.
	// Those were kept at their original height.
	// Do this only if the item was actually spawned by the map above ground to avoid problems.
	if (mo->specialf1 > 0 && (mo->flags2 & MF2_FLOATBOB) && (ib_compatflags & BCOMPATF_FLOATBOB))
	{
		mo->SetZ(mo->floorz + mo->specialf1);
	}


//
// adjust height
//
	if ((mo->flags & MF_FLOAT) && !(mo->flags2 & MF2_DORMANT) && mo->target)
	{	// float down towards target if too close
		if (!(mo->flags & (MF_SKULLFLY | MF_INFLOAT)))
		{
			dist = mo->Distance2D (mo->target);
			delta = (mo->target->Center()) - mo->Z();
			if (delta < 0 && dist < -(delta*3))
				mo->AddZ(-mo->FloatSpeed);
			else if (delta > 0 && dist < (delta*3))
				mo->AddZ(mo->FloatSpeed);
		}
	}
	if (mo->player && (mo->flags & MF_NOGRAVITY) && (mo->Z() > mo->floorz))
	{
		if (!mo->IsNoClip2())
		{
			mo->AddZ(DAngle(360 / 80.f * level.maptime).Sin() / 8);
		}
		mo->Vel.Z *= FRICTION_FLY;
	}
	if (mo->waterlevel && !(mo->flags & MF_NOGRAVITY))
	{
		double friction = -1;

		// Check 3D floors -- might be the source of the waterlevel
		for (auto rover : mo->Sector->e->XFloor.ffloors)
		{
			if (!(rover->flags & FF_EXISTS)) continue;
			if (!(rover->flags & FF_SWIMMABLE)) continue;

			if (mo->Z() >= rover->top.plane->ZatPoint(mo) ||
				mo->Center() < rover->bottom.plane->ZatPoint(mo))
				continue;

			friction = rover->model->GetFriction(rover->top.isceiling);
			break;
		}
		if (friction < 0)
			friction = mo->Sector->GetFriction();	// get real friction, even if from a terrain definition

		mo->Vel.Z *= friction;
	}

//
// clip movement
//
	if (mo->Z() <= mo->floorz)
	{	// Hit the floor
		if ((!mo->player || !(mo->player->cheats & CF_PREDICTING)) &&
			mo->Sector->SecActTarget != NULL &&
			mo->Sector->floorplane.ZatPoint(mo) == mo->floorz)
		{ // [RH] Let the sector do something to the actor
			mo->Sector->TriggerSectorActions (mo, SECSPAC_HitFloor);
		}
		P_CheckFor3DFloorHit(mo, mo->floorz);
		// [RH] Need to recheck this because the sector action might have
		// teleported the actor so it is no longer below the floor.
		if (mo->Z() <= mo->floorz)
		{
			if ((mo->flags & MF_MISSILE) && !(mo->flags & MF_NOCLIP))
			{
				mo->SetZ(mo->floorz);
				if (mo->BounceFlags & BOUNCE_Floors)
				{
					mo->FloorBounceMissile (mo->floorsector->floorplane);
					/* if (!(mo->flags6 & MF6_CANJUMP)) */ return;
				}
				else if (mo->flags3 & MF3_NOEXPLODEFLOOR)
				{
					P_HitFloor (mo);
					mo->Vel.Z = 0;
					return;
				}
				else if ((mo->flags3 & MF3_FLOORHUGGER) && !(mo->flags5 & MF5_NODROPOFF))
				{ // Floor huggers can go up steps
					return;
				}
				else
				{
					bool onsky = false;
					if (mo->floorpic == skyflatnum)
					{
						if (!(mo->flags3 & MF3_SKYEXPLODE))
						{
							// [RH] Just remove the missile without exploding it
							//		if this is a sky floor.
							mo->Destroy();
							return;
						}
						else onsky = true;
					}
					P_HitFloor (mo);
					P_ExplodeMissile (mo, NULL, NULL, onsky);
					return;
				}
			}
			else if (mo->BounceFlags & BOUNCE_MBF && mo->Vel.Z) // check for MBF-like bounce on non-missiles
			{
				mo->FloorBounceMissile(mo->floorsector->floorplane);
			}
			if (mo->flags3 & MF3_ISMONSTER)		// Blasted mobj falling
			{
				if (mo->Vel.Z < -23)
				{
					P_MonsterFallingDamage (mo);
				}
			}
			mo->SetZ(mo->floorz);
			if (mo->Vel.Z < 0)
			{
				const double minvel = -8;	// landing speed from a jump with normal gravity

				// Spawn splashes, etc.
				P_HitFloor (mo);
				if (mo->DamageType == NAME_Ice && mo->Vel.Z < minvel)
				{
					mo->tics = 1;
					mo->Vel.Zero();
					return;
				}
				// Let the actor do something special for hitting the floor
				if (mo->flags7 & MF7_SMASHABLE)
				{
					P_DamageMobj(mo, nullptr, nullptr, mo->health, NAME_Smash);
				}
				if (mo->player)
				{
					if (mo->player->jumpTics < 0 || mo->Vel.Z < minvel)
					{ // delay any jumping for a short while
						mo->player->jumpTics = 7;
					}
					if (mo->Vel.Z < minvel && !(mo->flags & MF_NOGRAVITY))
					{
						// Squat down.
						// Decrease viewheight for a moment after hitting the ground (hard),
						// and utter appropriate sound.
						PlayerLandedOnThing (mo, NULL);
					}
				}
				mo->Vel.Z = 0;
			}
			if (mo->flags & MF_SKULLFLY)
			{ // The skull slammed into something
				mo->Vel.Z = -mo->Vel.Z;
			}
			mo->Crash();
		}
	}

	if (mo->flags2 & MF2_FLOORCLIP)
	{
		mo->AdjustFloorClip ();
	}

	if (mo->Top() > mo->ceilingz)
	{ // hit the ceiling
		if ((!mo->player || !(mo->player->cheats & CF_PREDICTING)) &&
			mo->Sector->SecActTarget != NULL &&
			mo->Sector->ceilingplane.ZatPoint(mo) == mo->ceilingz)
		{ // [RH] Let the sector do something to the actor
			mo->Sector->TriggerSectorActions (mo, SECSPAC_HitCeiling);
		}
		P_CheckFor3DCeilingHit(mo, mo->ceilingz);
		// [RH] Need to recheck this because the sector action might have
		// teleported the actor so it is no longer above the ceiling.
		if (mo->Top() > mo->ceilingz)
		{
			mo->SetZ(mo->ceilingz - mo->Height);
			if (mo->BounceFlags & BOUNCE_Ceilings)
			{	// ceiling bounce
				mo->FloorBounceMissile (mo->ceilingsector->ceilingplane);
				/*if (!(mo->flags6 & MF6_CANJUMP))*/ return;
			}
			if (mo->flags & MF_SKULLFLY)
			{	// the skull slammed into something
				mo->Vel.Z = -mo->Vel.Z;
			}
			if (mo->Vel.Z > 0)
				mo->Vel.Z = 0;
			if ((mo->flags & MF_MISSILE) && !(mo->flags & MF_NOCLIP))
			{
				if (mo->flags3 & MF3_CEILINGHUGGER)
				{
					return;
				}
				bool onsky = false;
				if (mo->ceilingpic == skyflatnum)
				{
					if (!(mo->flags3 & MF3_SKYEXPLODE))
					{
						mo->Destroy();
						return;
					}
					else onsky = true;
				}
				P_ExplodeMissile (mo, NULL, NULL, onsky);
				return;
			}
		}
	}
	P_CheckFakeFloorTriggers (mo, oldz);
}

void P_CheckFakeFloorTriggers (AActor *mo, double oldz, bool oldz_has_viewheight)
{
	if (mo->player && (mo->player->cheats & CF_PREDICTING))
	{
		return;
	}
	sector_t *sec = mo->Sector;
	assert (sec != NULL);
	if (sec == NULL)
	{
		return;
	}
	if (sec->heightsec != NULL && sec->SecActTarget != NULL)
	{
		sector_t *hs = sec->heightsec;
		double waterz = hs->floorplane.ZatPoint(mo);
		double newz;
		double viewheight;

		if (mo->player != NULL)
		{
			viewheight = mo->player->viewheight;
		}
		else
		{
			viewheight = mo->Height;
		}

		if (oldz > waterz && mo->Z() <= waterz)
		{ // Feet hit fake floor
			sec->TriggerSectorActions (mo, SECSPAC_HitFakeFloor);
		}

		newz = mo->Z() + viewheight;
		if (!oldz_has_viewheight)
		{
			oldz += viewheight;
		}

		if (oldz <= waterz && newz > waterz)
		{ // View went above fake floor
			sec->TriggerSectorActions (mo, SECSPAC_EyesSurface);
		}
		else if (oldz > waterz && newz <= waterz)
		{ // View went below fake floor
			sec->TriggerSectorActions (mo, SECSPAC_EyesDive);
		}

		if (!(hs->MoreFlags & SECF_FAKEFLOORONLY))
		{
			waterz = hs->ceilingplane.ZatPoint(mo);
			if (oldz <= waterz && newz > waterz)
			{ // View went above fake ceiling
				sec->TriggerSectorActions (mo, SECSPAC_EyesAboveC);
			}
			else if (oldz > waterz && newz <= waterz)
			{ // View went below fake ceiling
				sec->TriggerSectorActions (mo, SECSPAC_EyesBelowC);
			}
		}
	}
}

//===========================================================================
//
// PlayerLandedOnThing
//
//===========================================================================

static void PlayerLandedOnThing (AActor *mo, AActor *onmobj)
{
	bool grunted;

	if (!mo->player)
		return;

	if (mo->player->mo == mo)
	{
		mo->player->deltaviewheight = mo->Vel.Z / 8.;
	}

	if (mo->player->cheats & CF_PREDICTING)
		return;

	P_FallingDamage (mo);

	// [RH] only make noise if alive
	if (!mo->player->morphTics && mo->health > 0)
	{
		grunted = false;
		// Why should this number vary by gravity?
		if (mo->health > 0 && mo->Vel.Z < -mo->player->mo->GruntSpeed)
		{
			S_Sound (mo, CHAN_VOICE, "*grunt", 1, ATTN_NORM);
			grunted = true;
		}
		if (onmobj != NULL || !Terrains[P_GetThingFloorType (mo)].IsLiquid)
		{
			if (!grunted || !S_AreSoundsEquivalent (mo, "*grunt", "*land"))
			{
				S_Sound (mo, CHAN_AUTO, "*land", 1, ATTN_NORM);
			}
		}
	}
//	mo->player->centering = true;
}



//
// P_NightmareRespawn
//
void P_NightmareRespawn (AActor *mobj)
{
	double z;
	AActor *mo;
	AActor *info = mobj->GetDefault();

	mobj->skillrespawncount++;

	// spawn the new monster (assume the spawn will be good)
	if (info->flags & MF_SPAWNCEILING)
		z = ONCEILINGZ;
	else if (info->flags2 & MF2_SPAWNFLOAT)
		z = FLOATRANDZ;
	else
		z = ONFLOORZ;

	// spawn it
	mo = AActor::StaticSpawn(mobj->GetClass(), DVector3(mobj->SpawnPoint.X, mobj->SpawnPoint.Y, z), NO_REPLACE, true);
	mo->health = mobj->SpawnHealth();

	if (z == ONFLOORZ)
	{
		mo->AddZ(mobj->SpawnPoint.Z);
		if (mo->Z() < mo->floorz)
		{ // Do not respawn monsters in the floor, even if that's where they
		  // started. The initial P_ZMovement() call would have put them on
		  // the floor right away, but we need them on the floor now so we
		  // can use P_CheckPosition() properly.
			mo->SetZ(mo->floorz);
		}
		if (mo->Top() > mo->ceilingz)
		{
			mo->SetZ(mo->ceilingz- mo->Height);
		}
	}
	else if (z == ONCEILINGZ)
	{
		mo->AddZ(-mobj->SpawnPoint.Z);
	}

	// If there are 3D floors, we need to find floor/ceiling again.
	P_FindFloorCeiling(mo, FFCF_SAMESECTOR | FFCF_ONLY3DFLOORS | FFCF_3DRESTRICT);

	if (z == ONFLOORZ)
	{
		if (mo->Z() < mo->floorz)
		{ // Do not respawn monsters in the floor, even if that's where they
		  // started. The initial P_ZMovement() call would have put them on
		  // the floor right away, but we need them on the floor now so we
		  // can use P_CheckPosition() properly.
			mo->SetZ(mo->floorz);
		}
		if (mo->Top() > mo->ceilingz)
		{ // Do the same for the ceiling.
			mo->SetZ(mo->ceilingz - mo->Height);
		}
	}

	// something is occupying its position?
	if (!P_CheckPosition(mo, mo->Pos(), true))
	{
		//[GrafZahl] MF_COUNTKILL still needs to be checked here.
		mo->ClearCounters();
		mo->Destroy ();
		return;		// no respawn
	}

	z = mo->Z();

	// inherit attributes from deceased one
	mo->SpawnPoint = mobj->SpawnPoint;
	mo->SpawnAngle = mobj->SpawnAngle;
	mo->SpawnFlags = mobj->SpawnFlags & ~MTF_DORMANT;	// It wasn't dormant when it died, so it's not dormant now, either.
	mo->Angles.Yaw = (double)mobj->SpawnAngle;

	mo->HandleSpawnFlags ();
	mo->reactiontime = 18;
	mo->CopyFriendliness (mobj, false);
	mo->Translation = mobj->Translation;

	mo->skillrespawncount = mobj->skillrespawncount;

	mo->Prev.Z = z;		// Do not interpolate Z position if we changed it since spawning.

	// spawn a teleport fog at old spot because of removal of the body?
	P_SpawnTeleportFog(mobj, mobj->Pos(), true, true);

	// spawn a teleport fog at the new spot
	P_SpawnTeleportFog(mobj, DVector3(mobj->SpawnPoint, z), false, true);

	// remove the old monster
	mobj->Destroy ();
}


AActor *AActor::TIDHash[128];

//
// P_ClearTidHashes
//
// Clears the tid hashtable.
//

void AActor::ClearTIDHashes ()
{
	memset(TIDHash, 0, sizeof(TIDHash));
}

//
// P_AddMobjToHash
//
// Inserts an mobj into the correct chain based on its tid.
// If its tid is 0, this function does nothing.
//
void AActor::AddToHash ()
{
	if (tid == 0)
	{
		iprev = NULL;
		inext = NULL;
		return;
	}
	else
	{
		int hash = TIDHASH (tid);

		inext = TIDHash[hash];
		iprev = &TIDHash[hash];
		TIDHash[hash] = this;
		if (inext)
		{
			inext->iprev = &inext;
		}
	}
}

//
// P_RemoveMobjFromHash
//
// Removes an mobj from its hash chain.
//
void AActor::RemoveFromHash ()
{
	if (tid != 0 && iprev)
	{
		*iprev = inext;
		if (inext)
		{
			inext->iprev = iprev;
		}
		iprev = NULL;
		inext = NULL;
	}
	tid = 0;
}

DEFINE_ACTION_FUNCTION(AActor, RemoveFromHash)
{
	PARAM_SELF_PROLOGUE(AActor);
	self->RemoveFromHash();
	return 0;
}

DEFINE_ACTION_FUNCTION(AActor, ChangeTid)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_INT(tid);
	self->RemoveFromHash();
	self->tid = tid;
	self->AddToHash();
	return 0;
}



//==========================================================================
//
// P_IsTIDUsed
//
// Returns true if there is at least one actor with the specified TID
// (dead or alive).
//
//==========================================================================

bool P_IsTIDUsed(int tid)
{
	AActor *probe = AActor::TIDHash[tid & 127];
	while (probe != NULL)
	{
		if (probe->tid == tid)
		{
			return true;
		}
		probe = probe->inext;
	}
	return false;
}

//==========================================================================
//
// P_FindUniqueTID
//
// Returns an unused TID. If start_tid is 0, then a random TID will be
// chosen. Otherwise, it will perform a linear search starting from
// start_tid. If limit is non-0, then it will not check more than <limit>
// number of TIDs. Returns 0 if no suitable TID was found.
//
//==========================================================================

int P_FindUniqueTID(int start_tid, int limit)
{
	int tid;

	if (start_tid != 0)
	{ // Do a linear search.
		if (start_tid > INT_MAX-limit+1)
		{ // If 'limit+start_tid-1' overflows, clamp 'limit' to INT_MAX
			limit = INT_MAX;
		}
		else
		{
			limit += start_tid-1;
		}
		for (tid = start_tid; tid <= limit; ++tid)
		{
			if (tid != 0 && !P_IsTIDUsed(tid))
			{
				return tid;
			}
		}
		// Nothing free found.
		return 0;
	}
	// Do a random search. To try and be a bit more performant, this
	// actually does several linear searches. In the case of *very*
	// dense TID usage, this could potentially perform worse than doing
	// a complete linear scan starting at 1. However, you would need
	// to use an absolutely ridiculous number of actors before this
	// becomes a real concern.
	if (limit == 0)
	{
		limit = INT_MAX;
	}
	for (int i = 0; i < limit; i += 5)
	{
		// Use a positive starting TID.
		tid = pr_uniquetid.GenRand32() & INT_MAX;
		tid = P_FindUniqueTID(tid == 0 ? 1 : tid, 5);
		if (tid != 0)
		{
			return tid;
		}
	}
	// Nothing free found.
	return 0;
}

DEFINE_ACTION_FUNCTION(AActor, FindUniqueTid)
{
	PARAM_PROLOGUE;
	PARAM_INT_DEF(start);
	PARAM_INT_DEF(limit);
	ACTION_RETURN_INT(P_FindUniqueTID(start, limit));
}




CCMD(utid)
{
	Printf("%d\n",
		P_FindUniqueTID(argv.argc() > 1 ? atoi(argv[1]) : 0,
		(argv.argc() > 2 && atoi(argv[2]) >= 0) ? atoi(argv[2]) : 0));
}

//==========================================================================
//
// AActor :: GetMissileDamage
//
// If the actor's damage amount is an expression, evaluate it and return
// the result. Otherwise, return ((random() & mask) + add) * damage.
//
//==========================================================================

int AActor::GetMissileDamage (int mask, int add)
{
	if (DamageVal >= 0)
	{
		if (mask == 0)
		{
			return add * DamageVal;
		}
		else
		{
			return ((pr_missiledamage() & mask) + add) * DamageVal;
		}
	}
	if (DamageFunc == nullptr)
	{
		// This should never happen
		assert(false && "No damage function found");
		return 0;
	}
	VMValue param = this;
	VMReturn result;

	int amount;

	result.IntAt(&amount);

	if (GlobalVMStack.Call(DamageFunc, &param, 1, &result, 1) < 1)
	{ // No results
		return 0;
	}
	return amount;
}

void AActor::Howl ()
{
	FSoundID howl = SoundVar(NAME_HowlSound);
	if (!S_IsActorPlayingSomething(this, CHAN_BODY, howl))
	{
		S_Sound (this, CHAN_BODY, howl, 1, ATTN_NORM);
	}
}

DEFINE_ACTION_FUNCTION(AActor, Howl)
{
	PARAM_SELF_PROLOGUE(AActor);
	self->Howl();
	return 0;
}

bool AActor::Slam (AActor *thing)
{
	flags &= ~MF_SKULLFLY;
	Vel.Zero();
	if (health > 0)
	{
		if (!(flags2 & MF2_DORMANT))
		{
			int dam = GetMissileDamage (7, 1);
			int newdam = P_DamageMobj (thing, this, this, dam, NAME_Melee);
			P_TraceBleed (newdam > 0 ? newdam : dam, thing, this);
			// The charging monster may have died by the target's actions here.
			if (health > 0)
			{
				if (SeeState != NULL) SetState (SeeState);
				else SetIdle();
			}
		}
		else
		{
			SetIdle();
			tics = -1;
		}
	}
	return false;			// stop moving
}

DEFINE_ACTION_FUNCTION(AActor, Slam)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT(thing, AActor);
	ACTION_RETURN_BOOL(self->Slam(thing));
}

bool AActor::CallSlam(AActor *thing)
{
	IFVIRTUAL(AActor, Slam)
	{
		VMValue params[2] = { (DObject*)this, thing };
		VMReturn ret;
		int retval;
		ret.IntAt(&retval);
		GlobalVMStack.Call(func, params, 2, &ret, 1, nullptr);
		return !!retval;

	}
	else return Slam(thing);
}



// This virtual method only exists on the script side.
int AActor::SpecialMissileHit (AActor *victim)
{
	IFVIRTUAL(AActor, SpecialMissileHit)
	{
		VMValue params[2] = { (DObject*)this, victim };
		VMReturn ret;
		int retval;
		ret.IntAt(&retval);
		GlobalVMStack.Call(func, params, 2, &ret, 1, nullptr);
		return retval;
	}
	else return -1;
}

bool AActor::AdjustReflectionAngle (AActor *thing, DAngle &angle)
{
	if (flags2 & MF2_DONTREFLECT) return true;
	if (thing->flags7 & MF7_THRUREFLECT) return false;
	// Change angle for reflection
	if (thing->flags4&MF4_SHIELDREFLECT)
	{
		// Shield reflection (from the Centaur)
		if (absangle(angle, thing->Angles.Yaw) > 45)
			return true;	// Let missile explode

		if (thing->flags7 & MF7_NOSHIELDREFLECT) return true;

		if (pr_reflect () < 128)
			angle += 45;
		else
			angle -= 45;

	}
	else if (thing->flags4&MF4_DEFLECT)
	{
		// deflect (like the Heresiarch)
		if(pr_reflect() < 128) 
			angle += 45;
		else 
			angle -= 45;
	}
	else
	{
		angle += ((pr_reflect() % 16) - 8);
	}
	//Always check for AIMREFLECT, no matter what else is checked above.
	if (thing->flags7 & MF7_AIMREFLECT)
	{
		if (this->target != NULL)
		{
			A_Face(this, this->target);
		}
		else if (thing->target != NULL)
		{
			A_Face(this, thing->target);
		}
	}
	
	return false;
}

int AActor::AbsorbDamage(int damage, FName dmgtype)
{
	for (AInventory *item = Inventory; item != nullptr; item = item->Inventory)
	{
		IFVIRTUALPTR(item, AInventory, AbsorbDamage)
		{
			VMValue params[4] = { item, damage, dmgtype.GetIndex(), &damage };
			GlobalVMStack.Call(func, params, 4, nullptr, 0, nullptr);
		}
	}
	return damage;
}

void AActor::AlterWeaponSprite(visstyle_t *vis)
{
	int changed = 0;
	TArray<AInventory *> items;
	// This needs to go backwards through the items but the list has no backlinks.
	for (AInventory *item = Inventory; item != nullptr; item = item->Inventory)
	{
		items.Push(item);
	}
	for(int i=items.Size()-1;i>=0;i--)
	{
		IFVIRTUALPTR(items[i], AInventory, AlterWeaponSprite)
		{
			VMValue params[3] = { items[i], vis, &changed };
			GlobalVMStack.Call(func, params, 3, nullptr, 0, nullptr);
		}
	}
}

void AActor::PlayActiveSound ()
{
	if (ActiveSound && !S_IsActorPlayingSomething (this, CHAN_VOICE, -1))
	{
		S_Sound (this, CHAN_VOICE, ActiveSound, 1,
			(flags3 & MF3_FULLVOLACTIVE) ? ATTN_NONE : ATTN_IDLE);
	}
}

DEFINE_ACTION_FUNCTION(AActor, PlayActiveSound)
{
	PARAM_SELF_PROLOGUE(AActor);
	self->PlayActiveSound();
	return 0;
}

bool AActor::IsOkayToAttack (AActor *link)
{
	if (!(player							// Original AActor::IsOkayToAttack was only for players
	//	|| (flags  & MF_FRIENDLY)			// Maybe let friendly monsters use the function as well?
		|| (flags5 & MF5_SUMMONEDMONSTER)	// AMinotaurFriend has its own version, generalized to other summoned monsters
		|| (flags2 & MF2_SEEKERMISSILE)))	// AHolySpirit and AMageStaffFX2 as well, generalized to other seeker missiles
	{	// Normal monsters and other actors always return false.
		return false;
	}
	// Standard things to eliminate: an actor shouldn't attack itself,
	// or a non-shootable, dormant, non-player-and-non-monster actor.
	if (link == this)									return false;
	if (!(link->player||(link->flags3 & MF3_ISMONSTER)))return false;
	if (!(link->flags & MF_SHOOTABLE))					return false;
	if (link->flags2 & MF2_DORMANT)						return false;

	// An actor shouldn't attack friendly actors. The reference depends
	// on the type of actor: for a player's actor, itself; for a projectile,
	// its target; and for a summoned minion, its tracer.
	AActor * Friend = NULL;
	if (player)											Friend = this;
	else if (flags5 & MF5_SUMMONEDMONSTER)				Friend = tracer;
	else if (flags2 & MF2_SEEKERMISSILE)				Friend = target;
	else if ((flags & MF_FRIENDLY) && FriendPlayer)		Friend = players[FriendPlayer-1].mo;

	// Friend checks
	if (link == Friend)									return false;
	if (Friend == NULL)									return false;
	if (Friend->IsFriend(link))							return false;
	if ((link->flags5 & MF5_SUMMONEDMONSTER)			// No attack against minions on the same side
		&& (link->tracer == Friend))					return false;
	if (multiplayer && !deathmatch						// No attack against fellow players in coop
		&& link->player && Friend->player)				return false;
	if (((flags & link->flags) & MF_FRIENDLY)			// No friendly infighting amongst minions
		&& IsFriend(link))								return false;

	// Now that all the actor checks are made, the line of sight can be checked
	if (P_CheckSight (this, link))
	{
		// AMageStaffFX2::IsOkayToAttack had an extra check here, generalized with a flag,
		// to only allow the check to succeed if the enemy was in a ~84� FOV of the player
		if (flags3 & MF3_SCREENSEEKER)
		{
			DAngle angle = absangle(Friend->AngleTo(link), Friend->Angles.Yaw);
			if (angle < 30 * (256./360.))
			{
				return true;
			}
		}
		// Other actors are not concerned by this check
		else return true;
	}
	// The sight check was failed, or the angle wasn't right for a screenseeker
	return false;
}

void AActor::SetShade (uint32_t rgb)
{
	PalEntry *entry = (PalEntry *)&rgb;
	fillcolor = rgb | (ColorMatcher.Pick (entry->r, entry->g, entry->b) << 24);
}

void AActor::SetShade (int r, int g, int b)
{
	fillcolor = MAKEARGB(ColorMatcher.Pick (r, g, b), r, g, b);
}

DEFINE_ACTION_FUNCTION(AActor, SetShade)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_INT(color);
	self->SetShade(color);
	return 0;
}

void AActor::SetPitch(DAngle p, bool interpolate, bool forceclamp)
{
	if (player != NULL || forceclamp)
	{ // clamp the pitch we set
		DAngle min, max;

		if (player != NULL)
		{
			min = player->MinPitch;
			max = player->MaxPitch;
		}
		else
		{
			min = -89.;
			max = 89.;
		}
		p = clamp(p, min, max);
	}
	if (p != Angles.Pitch)
	{
		Angles.Pitch = p;
		if (player != NULL && interpolate)
		{
			player->cheats |= CF_INTERPVIEW;
		}
	}
}

void AActor::SetAngle(DAngle ang, bool interpolate)
{
	if (ang != Angles.Yaw)
	{
		Angles.Yaw = ang;
		if (player != NULL && interpolate)
		{
			player->cheats |= CF_INTERPVIEW;
		}
	}
}

void AActor::SetRoll(DAngle r, bool interpolate)
{
	if (r != Angles.Roll)
	{
		Angles.Roll = r;
		if (player != NULL && interpolate)
		{
			player->cheats |= CF_INTERPVIEW;
		}
	}
}

PClassActor *AActor::GetBloodType(int type) const
{
	IFVIRTUAL(AActor, GetBloodType)
	{
		VMValue params[] = { (DObject*)this, type };
		PClassActor *res;
		VMReturn ret((void**)&res);
		GlobalVMStack.Call(func, params, countof(params), &ret, 1);
		return res;
	}
	return nullptr;
}


DVector3 AActor::GetPortalTransition(double byoffset, sector_t **pSec)
{
	bool moved = false;
	sector_t *sec = Sector;
	double testz = Z() + byoffset;
	DVector3 pos = Pos();

	while (!sec->PortalBlocksMovement(sector_t::ceiling))
	{
		if (testz >= sec->GetPortalPlaneZ(sector_t::ceiling))
		{
			pos = PosRelative(sec->GetOppositePortalGroup(sector_t::ceiling));
			sec = P_PointInSector(pos);
			moved = true;
		}
		else break;
	}
	if (!moved)
	{
		while (!sec->PortalBlocksMovement(sector_t::floor))
		{
			if (testz < sec->GetPortalPlaneZ(sector_t::floor))
			{
				pos = PosRelative(sec->GetOppositePortalGroup(sector_t::floor));
				sec = P_PointInSector(pos);
			}
			else break;
		}
	}
	if (pSec) *pSec = sec;
	return pos;
}



void AActor::CheckPortalTransition(bool islinked)
{
	bool moved = false;
	FLinkContext ctx;
	while (!Sector->PortalBlocksMovement(sector_t::ceiling))
	{
		if (Z() >= Sector->GetPortalPlaneZ(sector_t::ceiling))
		{
			DVector3 oldpos = Pos();
			if (islinked && !moved) UnlinkFromWorld(&ctx);
			SetXYZ(PosRelative(Sector->GetOppositePortalGroup(sector_t::ceiling)));
			Prev += Pos() - oldpos;
			Sector = P_PointInSector(Pos());
			PrevPortalGroup = Sector->PortalGroup;
			moved = true;
		}
		else break;
	}
	if (!moved)
	{
		while (!Sector->PortalBlocksMovement(sector_t::floor))
		{
			double portalz = Sector->GetPortalPlaneZ(sector_t::floor);
			if (Z() < portalz && floorz < portalz)
			{
				DVector3 oldpos = Pos();
				if (islinked && !moved) UnlinkFromWorld(&ctx);
				SetXYZ(PosRelative(Sector->GetOppositePortalGroup(sector_t::floor)));
				Prev += Pos() - oldpos;
				Sector = P_PointInSector(Pos());
				PrevPortalGroup = Sector->PortalGroup;
				moved = true;
			}
			else break;
		}
	}
	if (islinked && moved) LinkToWorld(&ctx);
}

//
// P_MobjThinker
//
void AActor::Tick ()
{
	// [RH] Data for Heretic/Hexen scrolling sectors
	static const int8_t HexenCompatSpeeds[] = {-25, 0, -10, -5, 0, 5, 10, 0, 25 };
	static const int8_t HexenScrollies[24][2] =
	{
		{  0,  1 }, {  0,  2 }, {  0,  4 },
		{ -1,  0 }, { -2,  0 }, { -4,  0 },
		{  0, -1 }, {  0, -2 }, {  0, -4 },
		{  1,  0 }, {  2,  0 }, {  4,  0 },
		{  1,  1 }, {  2,  2 }, {  4,  4 },
		{ -1,  1 }, { -2,  2 }, { -4,  4 },
		{ -1, -1 }, { -2, -2 }, { -4, -4 },
		{  1, -1 }, {  2, -2 }, {  4, -4 }
	};

	static const uint8_t HereticScrollDirs[4] = { 6, 9, 1, 4 };
	static const uint8_t HereticSpeedMuls[5] = { 5, 10, 25, 30, 35 };


	AActor *onmo;

	//assert (state != NULL);
	if (state == NULL)
	{
		Printf("Actor of type %s at (%f,%f) left without a state\n", GetClass()->TypeName.GetChars(), X(), Y());
		Destroy();
		return;
	}

	// This is necessary to properly interpolate movement outside this function
	// like from an ActorMover
	ClearInterpolation();

	if (flags5 & MF5_NOINTERACTION)
	{
		// only do the minimally necessary things here to save time:
		// Check the time freezer
		// apply velocity
		// ensure that the actor is not linked into the blockmap

		if (!(flags5 & MF5_NOTIMEFREEZE))
		{
			//Added by MC: Freeze mode.
			if (bglobal.freeze || level.flags2 & LEVEL2_FROZEN)
			{
				// Boss cubes shouldn't be accelerated by timefreeze
				if (flags6 & MF6_BOSSCUBE)
				{
					special2++;
				}
				return;
			}
		}

		if (!Vel.isZero() || !(flags & MF_NOBLOCKMAP))
		{
			FLinkContext ctx;
			UnlinkFromWorld(&ctx);
			flags |= MF_NOBLOCKMAP;
			SetXYZ(Vec3Offset(Vel));
			CheckPortalTransition(false);
			LinkToWorld(&ctx);
		}
		flags8 &= ~MF8_INSCROLLSEC;
	}
	else
	{
		AInventory * item = Inventory;

		// Handle powerup effects here so that the order is controlled
		// by the order in the inventory, not the order in the thinker table
		while (item != NULL && item->Owner == this)
		{
			IFVIRTUALPTR(item, AInventory, DoEffect)
			{
				VMValue params[1] = { item };
				GlobalVMStack.Call(func, params, 1, nullptr, 0, nullptr);
			}
			item = item->Inventory;
		}

		if (flags & MF_UNMORPHED)
		{
			return;
		}

		if (!(flags5 & MF5_NOTIMEFREEZE))
		{
			// Boss cubes shouldn't be accelerated by timefreeze
			if (flags6 & MF6_BOSSCUBE)
			{
				special2++;
			}
			//Added by MC: Freeze mode.
			if (bglobal.freeze && !(player && player->Bot == NULL))
			{
				return;
			}

			// Apply freeze mode.
			if ((level.flags2 & LEVEL2_FROZEN) && (player == NULL || player->timefreezer == 0))
			{
				return;
			}
		}


		if (effects & FX_ROCKET) 
		{
			if (++smokecounter == 4)
			{
				// add some smoke behind the rocket 
				smokecounter = 0;
				AActor *th = Spawn("RocketSmokeTrail", Vec3Offset(-Vel), ALLOW_REPLACE);
				if (th)
				{
					th->tics -= pr_rockettrail()&3;
					if (th->tics < 1) th->tics = 1;
					if (!(cl_rockettrails & 2)) th->renderflags |= RF_INVISIBLE;
				}
			}
		}
		else if (effects & FX_GRENADE) 
		{
			if (++smokecounter == 8)
			{
				smokecounter = 0;
				DAngle moveangle = Vel.Angle();
				double xo = -moveangle.Cos() * radius * 2 + pr_rockettrail() / 64.;
				double yo = -moveangle.Sin() * radius * 2 + pr_rockettrail() / 64.;
				double zo = -Height * Vel.Z / 8. + Height * (2 / 3.);
				AActor * th = Spawn("GrenadeSmokeTrail", Vec3Offset(xo, yo, zo), ALLOW_REPLACE);
				if (th)
				{
					th->tics -= pr_rockettrail()&3;
					if (th->tics < 1) th->tics = 1;
					if (!(cl_rockettrails & 2)) th->renderflags |= RF_INVISIBLE;
				}
			}
		}

		double oldz = Z();

		// [RH] Give the pain elemental vertical friction
		// This used to be in APainElemental::Tick but in order to use
		// A_PainAttack with other monsters it has to be here
		if (flags4 & MF4_VFRICTION)
		{
			if (health >0)
			{
				if (fabs (Vel.Z) < 0.25)
				{
					Vel.Z = 0;
					flags4 &= ~MF4_VFRICTION;
				}
				else
				{
					Vel.Z *= (0xe800 / 65536.);
				}
			}
		}

		// [RH] Pulse in and out of visibility
		if (effects & FX_VISIBILITYPULSE)
		{
			if (visdir > 0)
			{
				Alpha += 1/32.;
				if (Alpha >= 1.)
				{
					Alpha = 1.;
					visdir = -1;
				}
			}
			else
			{
				Alpha -= 1/32.;
				if (Alpha <= 0.25)
				{
					Alpha = 0.25;
					visdir = 1;
				}
			}
		}
		else if (flags & MF_STEALTH)
		{
			// [RH] Fade a stealth monster in and out of visibility
			RenderStyle.Flags &= ~STYLEF_Alpha1;
			if (visdir > 0)
			{
				Alpha += 2./TICRATE;
				if (Alpha > 1.)
				{
					Alpha = 1.;
					visdir = 0;
				}
			}
			else if (visdir < 0)
			{
				Alpha -= 1.5/TICRATE;
				if (Alpha < StealthAlpha)
				{
					Alpha = StealthAlpha;
					visdir = 0;
				}
			}
		}

		if (bglobal.botnum && !demoplayback &&
			((flags & (MF_SPECIAL|MF_MISSILE)) || (flags3 & MF3_ISMONSTER)))
		{
			bglobal.BotTick(this);
		}

		// [RH] Consider carrying sectors here
		DVector2 cumm(0, 0);

		if ((((flags8 & MF8_INSCROLLSEC) && level.Scrolls.Size() > 0) || player != NULL) && !(flags & MF_NOCLIP) && !(flags & MF_NOSECTOR))
		{
			double height, waterheight;	// killough 4/4/98: add waterheight
			const msecnode_t *node;
			int countx, county;

			// Clear the flag for the next frame.
			flags8 &= ~MF8_INSCROLLSEC;

			// killough 3/7/98: Carry things on floor
			// killough 3/20/98: use new sector list which reflects true members
			// killough 3/27/98: fix carrier bug
			// killough 4/4/98: Underwater, carry things even w/o gravity

			// Move objects only if on floor or underwater,
			// non-floating, and clipped.

			countx = county = 0;

			for (node = touching_sectorlist; node; node = node->m_tnext)
			{
				sector_t *sec = node->m_sector;
				DVector2 scrollv;

				if (level.Scrolls.Size() > unsigned(sec->Index()))
				{
					scrollv = level.Scrolls[sec->Index()];
				}
				else
				{
					scrollv.Zero();
				}

				if (player != NULL)
				{
					int scrolltype = sec->special;

					if (scrolltype >= Scroll_North_Slow &&
						scrolltype <= Scroll_SouthWest_Fast)
					{ // Hexen scroll special
						scrolltype -= Scroll_North_Slow;
						if (i_compatflags&COMPATF_RAVENSCROLL)
						{
							scrollv.X -= HexenCompatSpeeds[HexenScrollies[scrolltype][0]+4] * (1. / (32 * CARRYFACTOR));
							scrollv.Y += HexenCompatSpeeds[HexenScrollies[scrolltype][1]+4] * (1. / (32 * CARRYFACTOR));

						}
						else
						{
							// Use speeds that actually match the scrolling textures!
							scrollv.X -= HexenScrollies[scrolltype][0] * 0.5;
							scrollv.Y += HexenScrollies[scrolltype][1] * 0.5;
						}
					}
					else if (scrolltype >= Carry_East5 &&
							 scrolltype <= Carry_West35)
					{ // Heretic scroll special
						scrolltype -= Carry_East5;
						uint8_t dir = HereticScrollDirs[scrolltype / 5];
						double carryspeed = HereticSpeedMuls[scrolltype % 5] * (1. / (32 * CARRYFACTOR));
						if (scrolltype < 5 && !(i_compatflags&COMPATF_RAVENSCROLL)) 
						{
							// Use speeds that actually match the scrolling textures!
							carryspeed = (1 << ((scrolltype % 5) + 15)) / 65536.;
						}
						scrollv.X += carryspeed * ((dir & 3) - 1);
						scrollv.Y += carryspeed * (((dir & 12) >> 2) - 1);
					}
					else if (scrolltype == dScroll_EastLavaDamage)
					{ // Special Heretic scroll special
						if (i_compatflags&COMPATF_RAVENSCROLL)
						{
							scrollv.X += 28. / (32*CARRYFACTOR);
						}
						else
						{
							// Use a speed that actually matches the scrolling texture!
							scrollv.X += 12. / (32 * CARRYFACTOR);
						}
					}
					else if (scrolltype == Scroll_StrifeCurrent)
					{ // Strife scroll special
						int anglespeed = tagManager.GetFirstSectorTag(sec) - 100;
						double carryspeed = (anglespeed % 10) / (16 * CARRYFACTOR);
						DAngle angle = ((anglespeed / 10) * 45.);
						scrollv += angle.ToVector(carryspeed);
					}
				}

				if (scrollv.isZero())
				{
					continue;
				}
				sector_t *heightsec = sec->GetHeightSec();
				if (flags & MF_NOGRAVITY && heightsec == NULL)
				{
					continue;
				}
				DVector3 pos = PosRelative(sec);
				height = sec->floorplane.ZatPoint (pos);
				double height2 = sec->floorplane.ZatPoint(this);
				if (isAbove(height))
				{
					if (heightsec == NULL)
					{
						continue;
					}

					waterheight = heightsec->floorplane.ZatPoint (pos);
					if (waterheight > height && Z() >= waterheight)
					{
						continue;
					}
				}

				cumm += scrollv;
				if (scrollv.X) countx++;
				if (scrollv.Y) county++;
			}

			// Some levels designed with Boom in mind actually want things to accelerate
			// at neighboring scrolling sector boundaries. But it is only important for
			// non-player objects.
			if (player != NULL || !(i_compatflags & COMPATF_BOOMSCROLL))
			{
				if (countx > 1)
				{
					cumm.X /= countx;
				}
				if (county > 1)
				{
					cumm.Y /= county;
				}
			}
		}

		// [RH] If standing on a steep slope, fall down it
		if ((flags & MF_SOLID) && !(flags & (MF_NOCLIP|MF_NOGRAVITY)) &&
			!(flags & MF_NOBLOCKMAP) &&
			Vel.Z <= 0 &&
			floorz == Z())
		{
			secplane_t floorplane;

			// Check 3D floors as well
			floorplane = P_FindFloorPlane(floorsector, PosAtZ(floorz));

			if (floorplane.fC() < STEEPSLOPE &&
				floorplane.ZatPoint (PosRelative(floorsector)) <= floorz)
			{
				const msecnode_t *node;
				bool dopush = true;

				if (floorplane.fC() > STEEPSLOPE*2/3)
				{
					for (node = touching_sectorlist; node; node = node->m_tnext)
					{
						const sector_t *sec = node->m_sector;
						if (sec->floorplane.fC() >= STEEPSLOPE)
						{
							if (floorplane.ZatPoint(PosRelative(node->m_sector)) >= Z() - MaxStepHeight)
							{
								dopush = false;
								break;
							}
						}
					}
				}
				if (dopush)
				{
					Vel += floorplane.Normal().XY();
				}
			}
		}

		// [RH] Missiles moving perfectly vertical need some X/Y movement, or they
		// won't hurt anything. Don't do this if damage is 0! That way, you can
		// still have missiles that go straight up and down through actors without
		// damaging anything.
		// (for backwards compatibility this must check for lack of damage function, not for zero damage!)
		if ((flags & MF_MISSILE) && Vel.X == 0 && Vel.Y == 0 && !IsZeroDamage())
		{
			Vel.X = MinVel;
		}

		// Handle X and Y velocities
		BlockingMobj = NULL;
		double oldfloorz = P_XYMovement (this, cumm);
		if (ObjectFlags & OF_EuthanizeMe)
		{ // actor was destroyed
			return;
		}
		if (Vel.X == 0 && Vel.Y == 0) // Actors at rest
		{
			if (flags2 & MF2_BLASTED)
			{ // Reset to not blasted when velocities are gone
				flags2 &= ~MF2_BLASTED;
			}
			if ((flags6 & MF6_TOUCHY) && !IsSentient())
			{ // Arm a mine which has come to rest
				flags6 |= MF6_ARMED;
			}

		}
		if (Vel.Z != 0 || BlockingMobj || Z() != floorz)
		{	// Handle Z velocity and gravity
			if (((flags2 & MF2_PASSMOBJ) || (flags & MF_SPECIAL)) && !(i_compatflags & COMPATF_NO_PASSMOBJ))
			{
				if (!(onmo = P_CheckOnmobj (this)))
				{
					P_ZMovement (this, oldfloorz);
					flags2 &= ~MF2_ONMOBJ;
				}
				else
				{
					if (player)
					{
						if (Vel.Z < level.gravity * Sector->gravity * (-1./100)// -655.36f)
							&& !(flags&MF_NOGRAVITY))
						{
							PlayerLandedOnThing (this, onmo);
						}
					}
					if (onmo->Top() - Z() <= MaxStepHeight)
					{
						if (player && player->mo == this)
						{
							player->viewheight -= onmo->Top() - Z();
							double deltaview = player->GetDeltaViewHeight();
							if (deltaview > player->deltaviewheight)
							{
								player->deltaviewheight = deltaview;
							}
						} 
						SetZ(onmo->Top());
					}
					// Check for MF6_BUMPSPECIAL
					// By default, only players can activate things by bumping into them
					// We trigger specials as long as we are on top of it and not just when
					// we land on it. This could be considered as gravity making us continually
					// bump into it, but it also avoids having to worry about walking on to
					// something without dropping and not triggering anything.
					if ((onmo->flags6 & MF6_BUMPSPECIAL) && ((player != NULL)
						|| ((onmo->activationtype & THINGSPEC_MonsterTrigger) && (flags3 & MF3_ISMONSTER))
						|| ((onmo->activationtype & THINGSPEC_MissileTrigger) && (flags & MF_MISSILE))
						) && (level.maptime > onmo->lastbump)) // Leave the bumper enough time to go away
					{
						if (player == NULL || !(player->cheats & CF_PREDICTING))
						{
							if (P_ActivateThingSpecial(onmo, this))
								onmo->lastbump = level.maptime + TICRATE;
						}
					}
					if (Vel.Z != 0 && (BounceFlags & BOUNCE_Actors))
					{
						P_BounceActor(this, onmo, true);
					}
					else
					{
						flags2 |= MF2_ONMOBJ;
						Vel.Z = 0;
						Crash();
					}
				}
			}
			else
			{
				P_ZMovement (this, oldfloorz);
			}

			if (ObjectFlags & OF_EuthanizeMe)
				return;		// actor was destroyed
		}
		else if (Z() <= floorz)
		{
			Crash();
			if (ObjectFlags & OF_EuthanizeMe)
				return;		// actor was destroyed
		}

		CheckPortalTransition(true);

		UpdateWaterLevel ();

		// [RH] Don't advance if predicting a player
		if (player && (player->cheats & CF_PREDICTING))
		{
			return;
		}

		// Check for poison damage, but only once per PoisonPeriod tics (or once per second if none).
		if (PoisonDurationReceived && (level.time % (PoisonPeriodReceived ? PoisonPeriodReceived : TICRATE) == 0))
		{
			P_DamageMobj(this, NULL, Poisoner, PoisonDamageReceived, PoisonDamageTypeReceived ? PoisonDamageTypeReceived : (FName)NAME_Poison, 0);

			--PoisonDurationReceived;

			// Must clear damage when duration is done, otherwise it
			// could be added to with ADDITIVEPOISONDAMAGE.
			if (!PoisonDurationReceived) PoisonDamageReceived = 0;
		}
	}

	assert (state != NULL);
	if (state == NULL)
	{
		Destroy();
		return;
	}
	if (!CheckNoDelay())
		return; // freed itself
	// cycle through states, calling action functions at transitions

	UpdateRenderSectorList();

	if (tics != -1)
	{
		// [RH] Use tics <= 0 instead of == 0 so that spawnstates
		// of 0 tics work as expected.
		if (--tics <= 0)
		{
			if (!SetState(state->GetNextState()))
				return; 		// freed itself
		}
	}

	if (tics == -1 || state->GetCanRaise())
	{
		int respawn_monsters = G_SkillProperty(SKILLP_Respawn);
		// check for nightmare respawn
		if (!(flags5 & MF5_ALWAYSRESPAWN))
		{
			if (!respawn_monsters || !(flags3 & MF3_ISMONSTER) || (flags2 & MF2_DORMANT) || (flags5 & MF5_NEVERRESPAWN))
				return;

			int limit = G_SkillProperty (SKILLP_RespawnLimit);
			if (limit > 0 && skillrespawncount >= limit)
				return;
		}

		movecount++;

		if (movecount < respawn_monsters)
			return;

		if (level.time & 31)
			return;

		if (pr_nightmarerespawn() > 4)
			return;

		P_NightmareRespawn (this);
	}
}

DEFINE_ACTION_FUNCTION(AActor, Tick)
{
	PARAM_SELF_PROLOGUE(AActor);
	self->Tick();
	return 0;
}


//==========================================================================
//
// AActor :: CheckNoDelay
//
//==========================================================================

bool AActor::CheckNoDelay()
{
	if ((flags7 & MF7_HANDLENODELAY) && !(flags2 & MF2_DORMANT))
	{
		flags7 &= ~MF7_HANDLENODELAY;
		if (state->GetNoDelay())
		{
			// For immediately spawned objects with the NoDelay flag set for their
			// Spawn state, explicitly call the current state's function.
			FState *newstate;
			FStateParamInfo stp = { state, STATE_Actor, PSP_WEAPON };
			if (state->CallAction(this, this, &stp, &newstate))
			{
				if (ObjectFlags & OF_EuthanizeMe)
				{
					return false;		// freed itself
				}
				if (newstate != NULL)
				{
					return SetState(newstate);
				}
			}
		}
	}
	return true;
}

DEFINE_ACTION_FUNCTION(AActor, CheckNoDelay)
{
	PARAM_SELF_PROLOGUE(AActor);
	ACTION_RETURN_BOOL(self->CheckNoDelay());
}

//==========================================================================
//
// AActor :: CheckSectorTransition
//
// Fire off some sector triggers if the actor has changed sectors.
//
//==========================================================================

void AActor::CheckSectorTransition(sector_t *oldsec)
{
	if (oldsec != Sector)
	{
		if (oldsec->SecActTarget != NULL)
		{
			oldsec->TriggerSectorActions(this, SECSPAC_Exit);
		}
		if (Sector->SecActTarget != NULL)
		{
			int act = SECSPAC_Enter;
			if (Z() <= Sector->floorplane.ZatPoint(this))
			{
				act |= SECSPAC_HitFloor;
			}
			if (Top() >= Sector->ceilingplane.ZatPoint(this))
			{
				act |= SECSPAC_HitCeiling;
			}
			if (Sector->heightsec != NULL && Z() == Sector->heightsec->floorplane.ZatPoint(this))
			{
				act |= SECSPAC_HitFakeFloor;
			}
			Sector->TriggerSectorActions(this, act);
		}
		if (Z() == floorz)
		{
			P_CheckFor3DFloorHit(this, Z());
		}
		if (Top() == ceilingz)
		{
			P_CheckFor3DCeilingHit(this, Top());
		}
	}
}

//==========================================================================
//
// AActor::SplashCheck
//
// Returns true if actor should splash
//
//==========================================================================

void AActor::SplashCheck()
{
	double fh = -FLT_MAX;
	bool reset = false;

	waterlevel = 0;

	if (Sector == NULL)
	{
		return;
	}

	if (Sector->MoreFlags & SECF_UNDERWATER)	// intentionally not SECF_UNDERWATERMASK
	{
		waterlevel = 3;
	}
	else
	{
		const sector_t *hsec = Sector->GetHeightSec();
		if (hsec != NULL)
		{
			fh = hsec->floorplane.ZatPoint(this);
			//if (hsec->MoreFlags & SECF_UNDERWATERMASK)	// also check Boom-style non-swimmable sectors
			{
				if (Z() < fh)
				{
					waterlevel = 1;
					if (Center() < fh)
					{
						waterlevel = 2;
						if ((player && Z() + player->viewheight <= fh) ||
							(Top() <= fh))
						{
							waterlevel = 3;
						}
					}
				}
				else if (!(hsec->MoreFlags & SECF_FAKEFLOORONLY) && (Top() > hsec->ceilingplane.ZatPoint(this)))
				{
					waterlevel = 3;
				}
				else
				{
					waterlevel = 0;
				}
			}
		}
		else
		{
			// Check 3D floors as well!
			for (auto rover : Sector->e->XFloor.ffloors)
			{
				if (!(rover->flags & FF_EXISTS)) continue;
				if (rover->flags & FF_SOLID) continue;

				bool reset = !(rover->flags & FF_SWIMMABLE);
				if (reset && rover->alpha == 0) continue;
				double ff_bottom = rover->bottom.plane->ZatPoint(this);
				double ff_top = rover->top.plane->ZatPoint(this);

				if (ff_top <= Z() || ff_bottom > (Center())) continue;

				fh = ff_top;
				if (Z() < fh)
				{
					waterlevel = 1;
					if (Center() < fh)
					{
						waterlevel = 2;
						if ((player && Z() + player->viewheight <= fh) ||
							(Top() <= fh))
						{
							waterlevel = 3;
						}
					}
				}

				break;
			}
		}
	}

	// some additional checks to make deep sectors like Boom's splash without setting
	// the water flags. 
	if (boomwaterlevel == 0 && waterlevel != 0)
	{
		P_HitWater(this, Sector, PosAtZ(fh), true);
	}
	boomwaterlevel = waterlevel;
	return;
}

//==========================================================================
//
// AActor::UpdateWaterLevel
//
// Returns true if actor should splash
//
//==========================================================================

bool AActor::UpdateWaterLevel(bool dosplash)
{
	if (dosplash) SplashCheck();

	double fh = -FLT_MAX;
	bool reset = false;

	waterlevel = 0;

	if (Sector == NULL)
	{
		return false;
	}

	if (Sector->MoreFlags & SECF_UNDERWATER)	// intentionally not SECF_UNDERWATERMASK
	{
		waterlevel = 3;
	}
	else
	{
		const sector_t *hsec = Sector->GetHeightSec();
		if (hsec != NULL)
		{
			fh = hsec->floorplane.ZatPoint(this);
			if (hsec->MoreFlags & SECF_UNDERWATERMASK)	// also check Boom-style non-swimmable sectors
			{
				if (Z() < fh)
				{
					waterlevel = 1;
					if (Center() < fh)
					{
						waterlevel = 2;
						if ((player && Z() + player->viewheight <= fh) ||
							(Top() <= fh))
						{
							waterlevel = 3;
						}
					}
				}
				else if (!(hsec->MoreFlags & SECF_FAKEFLOORONLY) && (Top() > hsec->ceilingplane.ZatPoint(this)))
				{
					waterlevel = 3;
				}
				else
				{
					waterlevel = 0;
				}
			}
		}
		else
		{
			// Check 3D floors as well!
			for (auto rover : Sector->e->XFloor.ffloors)
			{
				if (!(rover->flags & FF_EXISTS)) continue;
				if (rover->flags & FF_SOLID) continue;
				if (!(rover->flags & FF_SWIMMABLE)) continue;

				double ff_bottom = rover->bottom.plane->ZatPoint(this);
				double ff_top = rover->top.plane->ZatPoint(this);

				if (ff_top <= Z() || ff_bottom > (Center())) continue;

				fh = ff_top;
				if (Z() < fh)
				{
					waterlevel = 1;
					if (Center() < fh)
					{
						waterlevel = 2;
						if ((player && Z() + player->viewheight <= fh) ||
							(Top() <= fh))
						{
							waterlevel = 3;
						}
					}
				}

				break;
			}
		}
	}
	return false;	// we did the splash ourselves
}

DEFINE_ACTION_FUNCTION(AActor, UpdateWaterLevel)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_BOOL_DEF(splash);
	ACTION_RETURN_BOOL(self->UpdateWaterLevel(splash));
}

//==========================================================================
//
// P_SpawnMobj
//
//==========================================================================

AActor *AActor::StaticSpawn (PClassActor *type, const DVector3 &pos, replace_t allowreplacement, bool SpawningMapThing)
{
	if (type == NULL)
	{
		I_Error ("Tried to spawn a class-less actor\n");
	}

	if (allowreplacement)
	{
		type = type->GetReplacement();
	}

	AActor *actor;
	
	actor = static_cast<AActor *>(const_cast<PClassActor *>(type)->CreateNew ());

	// Set default dialogue
	actor->ConversationRoot = GetConversation(actor->GetClass()->TypeName);
	if (actor->ConversationRoot != -1)
	{
		actor->Conversation = StrifeDialogues[actor->ConversationRoot];
	}
	else
	{
		actor->Conversation = NULL;
	}

	actor->SetXYZ(pos);
	actor->OldRenderPos = { FLT_MAX, FLT_MAX, FLT_MAX };
	actor->picnum.SetInvalid();
	actor->health = actor->SpawnHealth();

	// Actors with zero gravity need the NOGRAVITY flag set.
	if (actor->Gravity == 0) actor->flags |= MF_NOGRAVITY;

	FRandom &rng = bglobal.m_Thinking ? pr_botspawnmobj : pr_spawnmobj;

	if (actor->isFast() && actor->flags3 & MF3_ISMONSTER)
		actor->reactiontime = 0;

	if (actor->flags3 & MF3_ISMONSTER)
	{
		actor->LastLookPlayerNumber = rng() % MAXPLAYERS;
		actor->TIDtoHate = 0;
	}

	// Set the state, but do not use SetState, because action
	// routines can't be called yet.  If the spawnstate has an action
	// routine, it will not be called.
	FState *st = actor->SpawnState;
	actor->state = st;
	actor->tics = st->GetTics();
	
	actor->sprite = st->sprite;
	actor->frame = st->GetFrame();
	actor->renderflags = (actor->renderflags & ~RF_FULLBRIGHT) | ActorRenderFlags::FromInt (st->GetFullbright());
	actor->touching_sectorlist = nullptr;	// NULL head of sector list // phares 3/13/98
	actor->touching_rendersectors = nullptr;
	if (G_SkillProperty(SKILLP_FastMonsters))
	{
		double f = actor->FloatVar(NAME_FastSpeed);
		if (f >= 0) actor->Speed = f;
	}

	// set subsector and/or block links
	actor->LinkToWorld (nullptr, SpawningMapThing);
	actor->ClearInterpolation();

	actor->dropoffz = actor->floorz = actor->Sector->floorplane.ZatPoint(pos);
	actor->ceilingz = actor->Sector->ceilingplane.ZatPoint(pos);

	// The z-coordinate needs to be set once before calling P_FindFloorCeiling
	// For FLOATRANDZ just use the floor here.
	if (pos.Z == ONFLOORZ || pos.Z == FLOATRANDZ)
	{
		actor->SetZ(actor->floorz);
	}
	else if (pos.Z == ONCEILINGZ)
	{
		actor->SetZ(actor->ceilingz - actor->Height);
	}

	if (SpawningMapThing || !type->IsDescendantOf (RUNTIME_CLASS(APlayerPawn)))
	{
		// Check if there's something solid to stand on between the current position and the
		// current sector's floor. For map spawns this must be delayed until after setting the
		// z-coordinate.
		if (!SpawningMapThing) 
		{
			P_FindFloorCeiling(actor, FFCF_ONLYSPAWNPOS);
		}
		else
		{
			actor->floorsector = actor->Sector;
			actor->floorpic = actor->floorsector->GetTexture(sector_t::floor);
			actor->floorterrain = actor->floorsector->GetTerrain(sector_t::floor);
			actor->ceilingsector = actor->Sector;
			actor->ceilingpic = actor->ceilingsector->GetTexture(sector_t::ceiling);
		}
	}
	else if (!(actor->flags5 & MF5_NOINTERACTION))
	{
		P_FindFloorCeiling (actor);
	}
	else
	{
		actor->floorpic = actor->Sector->GetTexture(sector_t::floor);
		actor->floorterrain = actor->Sector->GetTerrain(sector_t::floor);
		actor->floorsector = actor->Sector;
		actor->ceilingpic = actor->Sector->GetTexture(sector_t::ceiling);
		actor->ceilingsector = actor->Sector;
	}

	actor->SpawnPoint.X = pos.X;
	actor->SpawnPoint.Y = pos.Y;
	// do not copy Z!

	if (pos.Z == ONFLOORZ)
	{
		actor->SetZ(actor->floorz);
	}
	else if (pos.Z == ONCEILINGZ)
	{
		actor->SetZ(actor->ceilingz - actor->Height);
	}
	else if (pos.Z == FLOATRANDZ)
	{
		double space = actor->ceilingz - actor->Height - actor->floorz;
		if (space > 48)
		{
			space -= 40;
			actor->SetZ( space * rng() / 256. + actor->floorz + 40);
		}
		else
		{
			actor->SetZ(actor->floorz);
		}
	}
	else
	{
		actor->SpawnPoint.Z = (actor->Z() - actor->Sector->floorplane.ZatPoint(actor));
	}

	if (actor->FloatBobPhase == (uint8_t)-1) actor->FloatBobPhase = rng();	// Don't make everything bob in sync (unless deliberately told to do)
	if (actor->flags2 & MF2_FLOORCLIP)
	{
		actor->AdjustFloorClip ();
	}
	else
	{
		actor->Floorclip = 0;
	}
	actor->UpdateWaterLevel (false);
	if (!SpawningMapThing)
	{
		actor->CallBeginPlay ();
		if (actor->ObjectFlags & OF_EuthanizeMe)
		{
			return NULL;
		}
	}
	if (level.flags & LEVEL_NOALLIES && !actor->player)
	{
		actor->flags &= ~MF_FRIENDLY;
	}
	// [RH] Count monsters whenever they are spawned.
	if (actor->CountsAsKill())
	{
		level.total_monsters++;
	}
	// [RH] Same, for items
	if (actor->flags & MF_COUNTITEM)
	{
		level.total_items++;
	}
	// And for secrets
	if (actor->flags5 & MF5_COUNTSECRET)
	{
		level.total_secrets++;
	}
	// force scroller check in the first tic.
	actor->flags8 |= MF8_INSCROLLSEC;
	return actor;
}

DEFINE_ACTION_FUNCTION(AActor, Spawn)
{
	PARAM_PROLOGUE;
	PARAM_CLASS_NOT_NULL(type, AActor);
	PARAM_FLOAT_DEF(x);
	PARAM_FLOAT_DEF(y);
	PARAM_FLOAT_DEF(z);
	PARAM_INT_DEF(flags);
	ACTION_RETURN_OBJECT(AActor::StaticSpawn(type, DVector3(x, y, z), replace_t(flags)));
}

PClassActor *ClassForSpawn(FName classname)
{
	PClass *cls = PClass::FindClass(classname);
	if (cls == NULL)
	{
		I_Error("Attempt to spawn actor of unknown type '%s'\n", classname.GetChars());
	}
	if (!cls->IsKindOf(RUNTIME_CLASS(PClassActor)))
	{
		I_Error("Attempt to spawn non-actor of type '%s'\n", classname.GetChars());
	}
	return static_cast<PClassActor*>(cls);
}

void AActor::LevelSpawned ()
{
	if (tics > 0 && !(flags4 & MF4_SYNCHRONIZED))
	{
		tics = 1 + (pr_spawnmapthing() % tics);
	}
	// [RH] Clear MF_DROPPED flag if the default version doesn't have it set.
	// (AInventory::BeginPlay() makes all inventory items spawn with it set.)
	if (!(GetDefault()->flags & MF_DROPPED))
	{
		flags &= ~MF_DROPPED;
	}
	HandleSpawnFlags ();
}

void AActor::HandleSpawnFlags ()
{
	if (SpawnFlags & MTF_AMBUSH)
	{
		flags |= MF_AMBUSH;
	}
	if (SpawnFlags & MTF_DORMANT)
	{
		CallDeactivate (NULL);
	}
	if (SpawnFlags & MTF_STANDSTILL)
	{
		flags4 |= MF4_STANDSTILL;
	}
	if (SpawnFlags & MTF_FRIENDLY)
	{
		flags |= MF_FRIENDLY;
		// Friendlies don't count as kills!
		if (flags & MF_COUNTKILL)
		{
			flags &= ~MF_COUNTKILL;
			level.total_monsters--;
		}
	}
	if (SpawnFlags & MTF_SHADOW)
	{
		flags |= MF_SHADOW;
		RenderStyle = STYLE_Translucent;
		Alpha = 0.25;
	}
	else if (SpawnFlags & MTF_ALTSHADOW)
	{
		RenderStyle = STYLE_None;
	}
	if (SpawnFlags & MTF_SECRET)
	{
		if (!(flags5 & MF5_COUNTSECRET))
		{
			//Printf("Secret %s in sector %i!\n", GetTag(), Sector->sectornum);
			flags5 |= MF5_COUNTSECRET;
			level.total_secrets++;
		}
	}
}

DEFINE_ACTION_FUNCTION(AActor, HandleSpawnFlags)
{
	PARAM_SELF_PROLOGUE(AActor);
	self->HandleSpawnFlags();
	return 0;
}

void AActor::BeginPlay ()
{
	// If the actor is spawned with the dormant flag set, clear it, and use
	// the normal deactivation logic to make it properly dormant.
	if (flags2 & MF2_DORMANT)
	{
		flags2 &= ~MF2_DORMANT;
		CallDeactivate (NULL);
	}
}

DEFINE_ACTION_FUNCTION(AActor, BeginPlay)
{
	PARAM_SELF_PROLOGUE(AActor);
	self->BeginPlay();
	return 0;
}

void AActor::CallBeginPlay()
{
	IFVIRTUAL(AActor, BeginPlay)
	{
		// Without the type cast this picks the 'void *' assignment...
		VMValue params[1] = { (DObject*)this };
		GlobalVMStack.Call(func, params, 1, nullptr, 0, nullptr);
	}
	else BeginPlay();
}


void AActor::PostBeginPlay ()
{
	if (Renderer != NULL)
	{
		Renderer->StateChanged(this);
	}
	PrevAngles = Angles;
	flags7 |= MF7_HANDLENODELAY;
}

void AActor::CallPostBeginPlay()
{
	Super::CallPostBeginPlay();
	E_WorldThingSpawned(this);
}

void AActor::MarkPrecacheSounds() const
{
	SeeSound.MarkUsed();
	AttackSound.MarkUsed();
	PainSound.MarkUsed();
	DeathSound.MarkUsed();
	ActiveSound.MarkUsed();
	UseSound.MarkUsed();
	BounceSound.MarkUsed();
	WallBounceSound.MarkUsed();
	CrushPainSound.MarkUsed();
}

DEFINE_ACTION_FUNCTION(AActor, MarkPrecacheSounds)
{
	PARAM_SELF_PROLOGUE(AActor);
	self->MarkPrecacheSounds();
	return 0;
}

bool AActor::isFast()
{
	if (flags5&MF5_ALWAYSFAST) return true;
	if (flags5&MF5_NEVERFAST) return false;
	return !!G_SkillProperty(SKILLP_FastMonsters);
}

bool AActor::isSlow()
{
	return !!G_SkillProperty(SKILLP_SlowMonsters);
}

//===========================================================================
//
// Activate
//
//===========================================================================

void AActor::Activate (AActor *activator)
{
	if ((flags3 & MF3_ISMONSTER) && (health > 0 || (flags & MF_ICECORPSE)))
	{
		if (flags2 & MF2_DORMANT)
		{
			flags2 &= ~MF2_DORMANT;
			FState *state = FindState(NAME_Active);
			if (state != NULL) 
			{
				SetState(state);
			}
			else
			{
				tics = 1;
			}
		}
	}
}

DEFINE_ACTION_FUNCTION(AActor, Activate)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT(activator, AActor);
	self->Activate(activator);
	return 0;
}

void AActor::CallActivate(AActor *activator)
{
	IFVIRTUAL(AActor, Activate)
	{
		// Without the type cast this picks the 'void *' assignment...
		VMValue params[2] = { (DObject*)this, (DObject*)activator };
		GlobalVMStack.Call(func, params, 2, nullptr, 0, nullptr);
	}
	else Activate(activator);
}


//===========================================================================
//
// Deactivate
//
//===========================================================================

void AActor::Deactivate (AActor *activator)
{
	if ((flags3 & MF3_ISMONSTER) && (health > 0 || (flags & MF_ICECORPSE)))
	{
		if (!(flags2 & MF2_DORMANT))
		{
			flags2 |= MF2_DORMANT;
			FState *state = FindState(NAME_Inactive);
			if (state != NULL) 
			{
				SetState(state);
			}
			else
			{
				tics = -1;
			}
		}
	}
}

DEFINE_ACTION_FUNCTION(AActor, Deactivate)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT(activator, AActor);
	self->Deactivate(activator);
	return 0;
}

void AActor::CallDeactivate(AActor *activator)
{
	IFVIRTUAL(AActor, Deactivate)
	{
		// Without the type cast this picks the 'void *' assignment...
		VMValue params[2] = { (DObject*)this, (DObject*)activator };
		GlobalVMStack.Call(func, params, 2, nullptr, 0, nullptr);
	}
	else Deactivate(activator);
}

//===========================================================================
//
// Destroy
//
//===========================================================================

void AActor::OnDestroy ()
{
	// [ZZ] call destroy event hook.
	//      note that this differs from ThingSpawned in that you can actually override OnDestroy to avoid calling the hook.
	//      but you can't really do that without utterly breaking the game, so it's ok.
	//      note: if OnDestroy is ever made optional, E_WorldThingDestroyed should still be called for ANY thing.
	E_WorldThingDestroyed(this);

	ClearRenderSectorList();
	ClearRenderLineList();

	// [RH] Destroy any inventory this actor is carrying
	DestroyAllInventory ();

	// [RH] Unlink from tid chain
	RemoveFromHash ();

	// unlink from sector and block lists
	UnlinkFromWorld (nullptr);
	flags |= MF_NOSECTOR|MF_NOBLOCKMAP;

	// Transform any playing sound into positioned, non-actor sounds.
	S_RelinkSound (this, NULL);

	Super::OnDestroy();
}

//===========================================================================
//
// AdjustFloorClip
//
//===========================================================================

void AActor::AdjustFloorClip ()
{
	if (flags3 & MF3_SPECIALFLOORCLIP)
	{
		return;
	}

	double oldclip = Floorclip;
	double shallowestclip = INT_MAX;
	const msecnode_t *m;

	// possibly standing on a 3D-floor
	if (Sector->e->XFloor.ffloors.Size() && Z() > Sector->floorplane.ZatPoint(this)) Floorclip = 0;

	// [RH] clip based on shallowest floor player is standing on
	// If the sector has a deep water effect, then let that effect
	// do the floorclipping instead of the terrain type.
	for (m = touching_sectorlist; m; m = m->m_tnext)
	{
		DVector3 pos = PosRelative(m->m_sector);
		sector_t *hsec = m->m_sector->GetHeightSec();
		if (hsec == NULL && m->m_sector->floorplane.ZatPoint (pos) == Z())
		{
			double clip = Terrains[m->m_sector->GetTerrain(sector_t::floor)].FootClip;
			if (clip < shallowestclip)
			{
				shallowestclip = clip;
			}
		}
	}
	if (shallowestclip == INT_MAX)
	{
		Floorclip = 0;
	}
	else
	{
		Floorclip = shallowestclip;
	}
	if (player && player->mo == this && oldclip != Floorclip)
	{
		player->viewheight -= (oldclip - Floorclip);
		player->deltaviewheight = player->GetDeltaViewHeight();
	}
}

DEFINE_ACTION_FUNCTION(AActor, AdjustFloorClip)
{
	PARAM_SELF_PROLOGUE(AActor);
	self->AdjustFloorClip();
	return 0;
}

//
// P_SpawnPlayer
// Called when a player is spawned on the level.
// Most of the player structure stays unchanged between levels.
//
EXTERN_CVAR (Bool, chasedemo)
EXTERN_CVAR(Bool, sv_singleplayerrespawn)
EXTERN_CVAR(Float, fov)

extern bool demonew;

APlayerPawn *P_SpawnPlayer (FPlayerStart *mthing, int playernum, int flags)
{
	player_t *p;
	APlayerPawn *mobj, *oldactor;
	uint8_t	  state;
	DVector3 spawn;
	DAngle SpawnAngle;

	if (mthing == NULL)
	{
		return NULL;
	}
	// not playing?
	if ((unsigned)playernum >= (unsigned)MAXPLAYERS || !playeringame[playernum])
		return NULL;

	// Old lerp data needs to go
	if (playernum == consoleplayer)
	{
		P_PredictionLerpReset();
	}

	p = &players[playernum];

	if (p->cls == NULL)
	{
		// [GRB] Pick a class from player class list
		if (PlayerClasses.Size () > 1)
		{
			int type;

			if (!deathmatch || !multiplayer)
			{
				type = SinglePlayerClass[playernum];
			}
			else
			{
				type = p->userinfo.GetPlayerClassNum();
				if (type < 0)
				{
					type = pr_multiclasschoice() % PlayerClasses.Size ();
				}
			}
			p->CurrentPlayerClass = type;
		}
		else
		{
			p->CurrentPlayerClass = 0;
		}
		p->cls = PlayerClasses[p->CurrentPlayerClass].Type;
	}

	if (( dmflags2 & DF2_SAME_SPAWN_SPOT ) &&
		( p->playerstate == PST_REBORN ) &&
		( deathmatch == false ) &&
		( gameaction != ga_worlddone ) &&
		( p->mo != NULL ) && 
		( !(p->mo->Sector->Flags & SECF_NORESPAWN) ) &&
		( NULL != p->attacker ) &&							// don't respawn on damaging floors
		( p->mo->Sector->damageamount < TELEFRAG_DAMAGE ))	// this really should be a bit smarter...
	{
		spawn = p->mo->Pos();
		SpawnAngle = p->mo->Angles.Yaw;
	}
	else
	{
		spawn.X = mthing->pos.X;
		spawn.Y = mthing->pos.Y;

		// Allow full angular precision
		SpawnAngle = (double)mthing->angle;
		if (i_compatflags2 & COMPATF2_BADANGLES)
		{
			SpawnAngle += 0.01;
		}

		if (GetDefaultByType(p->cls)->flags & MF_SPAWNCEILING)
			spawn.Z = ONCEILINGZ;
		else if (GetDefaultByType(p->cls)->flags2 & MF2_SPAWNFLOAT)
			spawn.Z = FLOATRANDZ;
		else
			spawn.Z = ONFLOORZ;
	}

	mobj = static_cast<APlayerPawn *>
		(Spawn (p->cls, spawn, NO_REPLACE));

	if (level.flags & LEVEL_USEPLAYERSTARTZ)
	{
		if (spawn.Z == ONFLOORZ)
			mobj->AddZ(mthing->pos.Z);
		else if (spawn.Z == ONCEILINGZ)
			mobj->AddZ(-mthing->pos.Z);
		P_FindFloorCeiling(mobj, FFCF_SAMESECTOR | FFCF_ONLY3DFLOORS | FFCF_3DRESTRICT);
	}

	mobj->FriendPlayer = playernum + 1;	// [RH] players are their own friends
	oldactor = p->mo;
	p->mo = mobj;
	mobj->player = p;
	state = p->playerstate;
	if (state == PST_REBORN || state == PST_ENTER)
	{
		G_PlayerReborn (playernum);
	}
	else if (oldactor != NULL && oldactor->player == p && !(flags & SPF_TEMPPLAYER))
	{
		// Move the voodoo doll's inventory to the new player.
		mobj->ObtainInventory (oldactor);
		FBehavior::StaticStopMyScripts (oldactor);	// cancel all ENTER/RESPAWN scripts for the voodoo doll
	}

	// [GRB] Reset skin
	p->userinfo.SkinNumChanged(R_FindSkin (Skins[p->userinfo.GetSkin()].Name, p->CurrentPlayerClass));

	if (!(mobj->flags2 & MF2_DONTTRANSLATE))
	{
		// [RH] Be sure the player has the right translation
		R_BuildPlayerTranslation (playernum);

		// [RH] set color translations for player sprites
		mobj->Translation = TRANSLATION(TRANSLATION_Players,playernum);
	}

	mobj->Angles.Yaw = SpawnAngle;
	mobj->Angles.Pitch = mobj->Angles.Roll = 0.;
	mobj->health = p->health;

	// [RH] Set player sprite based on skin
	if (!(mobj->flags4 & MF4_NOSKIN))
	{
		mobj->sprite = Skins[p->userinfo.GetSkin()].sprite;
	}

	p->DesiredFOV = p->FOV = fov;
	p->camera = p->mo;
	p->playerstate = PST_LIVE;
	p->refire = 0;
	p->damagecount = 0;
	p->bonuscount = 0;
	p->morphTics = 0;
	p->MorphedPlayerClass = 0;
	p->MorphStyle = 0;
	p->MorphExitFlash = NULL;
	p->extralight = 0;
	p->fixedcolormap = NOFIXEDCOLORMAP;
	p->fixedlightlevel = -1;
	p->viewheight = mobj->ViewHeight;
	p->inconsistant = 0;
	p->attacker = NULL;
	p->spreecount = 0;
	p->multicount = 0;
	p->lastkilltime = 0;
	p->BlendR = p->BlendG = p->BlendB = p->BlendA = 0.f;
	p->mo->ResetAirSupply(false);
	p->Uncrouch();
	p->MinPitch = p->MaxPitch = 0.;	// will be filled in by PostBeginPlay()/netcode
	p->MUSINFOactor = NULL;
	p->MUSINFOtics = -1;

	p->Vel.Zero();	// killough 10/98: initialize bobbing to 0.

	for (int ii = 0; ii < MAXPLAYERS; ++ii)
	{
		if (playeringame[ii] && players[ii].camera == oldactor)
		{
			players[ii].camera = mobj;
		}
	}

	// [RH] Allow chasecam for demo watching
	if ((demoplayback || demonew) && chasedemo)
		p->cheats = CF_CHASECAM;

	// setup gun psprite
	if (!(flags & SPF_TEMPPLAYER))
	{ // This can also start a script so don't do it for the dummy player.
		P_SetupPsprites (p, !!(flags & SPF_WEAPONFULLYUP));
	}

	if (deathmatch)
	{ // Give all cards in death match mode.
		p->mo->GiveDeathmatchInventory ();
	}
	else if ((multiplayer || (level.flags2 & LEVEL2_ALLOWRESPAWN) || sv_singleplayerrespawn) && state == PST_REBORN && oldactor != NULL)
	{ // Special inventory handling for respawning in coop
		p->mo->FilterCoopRespawnInventory (oldactor);
	}
	if (oldactor != NULL)
	{ // Remove any inventory left from the old actor. Coop handles
	  // it above, but the other modes don't.
		oldactor->DestroyAllInventory();
	}
	// [BC] Handle temporary invulnerability when respawned
	if (state == PST_REBORN || state == PST_ENTER)
	{
		IFVIRTUALPTR(p->mo, APlayerPawn, OnRespawn)
		{
			VMValue param = p->mo;
			GlobalVMStack.Call(func, &param, 1, nullptr, 0);
		}
	}

	if (StatusBar != NULL && (playernum == consoleplayer || StatusBar->GetPlayer() == playernum))
	{
		StatusBar->AttachToPlayer (p);
	}

	if (multiplayer)
	{
		P_SpawnTeleportFog(mobj, mobj->Vec3Angle(20., mobj->Angles.Yaw, 0.), false, true);
	}

	// "Fix" for one of the starts on exec.wad MAP01: If you start inside the ceiling,
	// drop down below it, even if that means sinking into the floor.
	if (mobj->Top() > mobj->ceilingz)
	{
		mobj->SetZ(mobj->ceilingz - mobj->Height, false);
	}

	// [BC] Do script stuff
	if (!(flags & SPF_TEMPPLAYER))
	{
		if (state == PST_ENTER || (state == PST_LIVE && !savegamerestore))
		{
			FBehavior::StaticStartTypedScripts (SCRIPT_Enter, p->mo, true);
		}
		else if (state == PST_REBORN)
		{
			assert (oldactor != NULL);

			// before relocating all pointers to the player all sound targets
			// pointing to the old actor have to be NULLed. Otherwise all
			// monsters who last targeted this player will wake up immediately
			// after the player has respawned.
			AActor *th;
			TThinkerIterator<AActor> it;
			while ((th = it.Next()))
			{
				if (th->LastHeard == oldactor) th->LastHeard = NULL;
			}
			for(auto &sec : level.sectors)
			{
				if (sec.SoundTarget == oldactor) sec.SoundTarget = nullptr;
			}

			DObject::StaticPointerSubstitution (oldactor, p->mo);
			// PointerSubstitution() will also affect the bodyque, so undo that now.
			for (int ii=0; ii < BODYQUESIZE; ++ii)
				if (bodyque[ii] == p->mo)
					bodyque[ii] = oldactor;
			E_PlayerRespawned(int(p - players));
			FBehavior::StaticStartTypedScripts (SCRIPT_Respawn, p->mo, true);
		}
	}
	return mobj;
}


//
// P_SpawnMapThing
// The fields of the mapthing should
// already be in host byte order.
//
// [RH] position is used to weed out unwanted start spots
AActor *P_SpawnMapThing (FMapThing *mthing, int position)
{
	PClassActor *i;
	int mask;
	AActor *mobj;

	if (mthing->EdNum == 0 || mthing->EdNum == -1)
		return NULL;

	// find which type to spawn
	FDoomEdEntry *mentry = mthing->info;

	if (mentry == NULL)
	{
		// [RH] Don't die if the map tries to spawn an unknown thing
		Printf("Unknown type %i at (%.1f, %.1f)\n",
			mthing->EdNum, mthing->pos.X, mthing->pos.Y);
		mentry = DoomEdMap.CheckKey(0);
		if (mentry == NULL)	// we need a valid entry for the rest of this function so if we can't find a default, let's exit right away.
		{
		return NULL;
	}
	}
	if (mentry->Type == NULL && mentry->Special <= 0)
	{
		// has been explicitly set to not spawning anything.
		return NULL;
	}

	// copy args to mapthing so that we have them in one place for the rest of this function	
	if (mentry->ArgsDefined > 0)
	{
		if (mentry->Type!= NULL) mthing->special = mentry->Special;
		memcpy(mthing->args, mentry->Args, sizeof(mthing->args[0]) * mentry->ArgsDefined);
	}

	int pnum = -1;
	if (mentry->Type == NULL)
	{

		switch (mentry->Special)
		{
		case SMT_DeathmatchStart:
		{
			// count deathmatch start positions
			FPlayerStart start(mthing, 0);
			deathmatchstarts.Push(start);
			return NULL;
		}

		case SMT_PolyAnchor:
		case SMT_PolySpawn:
		case SMT_PolySpawnCrush:
		case SMT_PolySpawnHurt:
	{
		polyspawns_t *polyspawn = new polyspawns_t;
		polyspawn->next = polyspawns;
		polyspawn->pos = mthing->pos;
		polyspawn->angle = mthing->angle;
		polyspawn->type = mentry->Special;
		polyspawns = polyspawn;
			if (mentry->Special != SMT_PolyAnchor)
			po_NumPolyobjs++;
		return NULL;
	}

		case SMT_Player1Start:
		case SMT_Player2Start:
		case SMT_Player3Start:
		case SMT_Player4Start:
		case SMT_Player5Start:
		case SMT_Player6Start:
		case SMT_Player7Start:
		case SMT_Player8Start:
			pnum = mentry->Special - SMT_Player1Start;
			break;

		// Sound sequence override will be handled later
		default:
			break;

	}
		}

	if (pnum == -1 || (level.flags & LEVEL_FILTERSTARTS))
	{
		// check for appropriate game type
		if (deathmatch) 
		{
			mask = MTF_DEATHMATCH;
		}
		else if (multiplayer)
		{
			mask = MTF_COOPERATIVE;
		}
		else
		{
			mask = MTF_SINGLE;
		}
		if (!(mthing->flags & mask))
		{
			return NULL;
		}

		mask = G_SkillProperty(SKILLP_SpawnFilter);
		if (!(mthing->SkillFilter & mask))
		{
			return NULL;
		}

		// Check class spawn masks. Now with player classes available
		// this is enabled for all games.
		if (!multiplayer)
		{ // Single player
			int spawnmask = players[consoleplayer].GetSpawnClass();
			if (spawnmask != 0 && (mthing->ClassFilter & spawnmask) == 0)
			{ // Not for current class
				return NULL;
			}
		}
		else if (!deathmatch)
		{ // Cooperative
			mask = 0;
			for (int i = 0; i < MAXPLAYERS; i++)
			{
				if (playeringame[i])
				{
					int spawnmask = players[i].GetSpawnClass();
					if (spawnmask != 0)
						mask |= spawnmask;
					else 
						mask = -1;
				}
			}
			if (mask != -1 && (mthing->ClassFilter & mask) == 0)
			{
				return NULL;
			}
		}
	}

	if (pnum != -1)
	{
		// [RH] Only spawn spots that match position.
		if (mthing->args[0] != position)
			return NULL;

		// save spots for respawning in network games
		FPlayerStart start(mthing, pnum+1);
		playerstarts[pnum] = start;
		if (level.flags2 & LEVEL2_RANDOMPLAYERSTARTS)
		{ // When using random player starts, all starts count
			AllPlayerStarts.Push(start);
		}
		else
		{ // When not using random player starts, later single player
		  // starts should override earlier ones, since the earlier
		  // ones are for voodoo dolls and not likely to be ideal for
		  // spawning regular players.
			unsigned i;
			for (i = 0; i < AllPlayerStarts.Size(); ++i)
			{
				if (AllPlayerStarts[i].type == pnum+1)
				{
					AllPlayerStarts[i] = start;
					break;
				}
			}
			if (i == AllPlayerStarts.Size())
			{
				AllPlayerStarts.Push(start);
			}
		}
		if (!deathmatch && !(level.flags2 & LEVEL2_RANDOMPLAYERSTARTS))
		{
			return P_SpawnPlayer(&start, pnum, (level.flags2 & LEVEL2_PRERAISEWEAPON) ? SPF_WEAPONFULLYUP : 0);
		}
		return NULL;
	}

	// [RH] sound sequence overriders
	if (mentry->Type == NULL && mentry->Special == SMT_SSeqOverride)
	{
		int type = mthing->args[0];
		if (type == 255) type = -1;
		if (type > 63)
		{
			Printf ("Sound sequence %d out of range\n", type);
		}
		else
		{
			P_PointInSector (mthing->pos)->seqType = type;
		}
		return NULL;
	}

	// [RH] If the thing's corresponding sprite has no frames, also map
	//		it to the unknown thing.
		// Handle decorate replacements explicitly here
		// to check for missing frames in the replacement object.
	i = mentry->Type->GetReplacement();

		const AActor *defaults = GetDefaultByType (i);
		if (defaults->SpawnState == NULL ||
			sprites[defaults->SpawnState->sprite].numframes == 0)
		{
			// We don't load mods for shareware games so we'll just ignore
			// missing actors. Heretic needs this since the shareware includes
			// the retail weapons in Deathmatch.
			if (gameinfo.flags & GI_SHAREWARE)
				return NULL;

			Printf ("%s at (%.1f, %.1f) has no frames\n",
					i->TypeName.GetChars(), mthing->pos.X, mthing->pos.Y);
			i = PClass::FindActor("Unknown");
			assert(i->IsKindOf(RUNTIME_CLASS(PClassActor)));
		}

	const AActor *info = GetDefaultByType (i);

	// don't spawn keycards and players in deathmatch
	if (deathmatch && info->flags & MF_NOTDMATCH)
		return NULL;

	// [RH] don't spawn extra weapons in coop if so desired
	if (multiplayer && !deathmatch && (dmflags & DF_NO_COOP_WEAPON_SPAWN))
	{
		if (GetDefaultByType(i)->flags7 & MF7_WEAPONSPAWN)
		{
			if ((mthing->flags & (MTF_DEATHMATCH|MTF_SINGLE)) == MTF_DEATHMATCH)
				return NULL;
		}
	}

	// don't spawn any monsters if -nomonsters
	if (((level.flags2 & LEVEL2_NOMONSTERS) || (dmflags & DF_NO_MONSTERS)) && info->flags3 & MF3_ISMONSTER )
	{
		return NULL;
	}
	
	// [RH] Other things that shouldn't be spawned depending on dmflags
	if (deathmatch || alwaysapplydmflags)
	{
		if (i->IsDescendantOf(RUNTIME_CLASS(AInventory)))
		{
			auto it = static_cast<AInventory*>(GetDefaultByType(i));

			if (dmflags & DF_NO_HEALTH)
			{
				if (it->ItemFlags & IF_ISHEALTH) return nullptr;
			}
			if (dmflags & DF_NO_ITEMS)
			{
				//			if (i->IsDescendantOf (RUNTIME_CLASS(AArtifact)))
				//				return;
			}
			if (dmflags & DF_NO_ARMOR)
			{
				if (it->ItemFlags & IF_ISARMOR) return nullptr;
			}
		}
	}

	// spawn it
	double sz;

	if (info->flags & MF_SPAWNCEILING)
		sz = ONCEILINGZ;
	else if (info->flags2 & MF2_SPAWNFLOAT)
		sz = FLOATRANDZ;
	else
		sz = ONFLOORZ;

	mobj = AActor::StaticSpawn (i, DVector3(mthing->pos, sz), NO_REPLACE, true);

	if (sz == ONFLOORZ)
	{
		mobj->AddZ(mthing->pos.Z);
		if ((mobj->flags2 & MF2_FLOATBOB) && (ib_compatflags & BCOMPATF_FLOATBOB))
		{
			mobj->specialf1 = mthing->pos.Z;
		}
	}
	else if (sz == ONCEILINGZ)
		mobj->AddZ(-mthing->pos.Z);

	mobj->SpawnPoint = mthing->pos;
	mobj->SpawnAngle = mthing->angle;
	mobj->SpawnFlags = mthing->flags;
	if (mthing->FloatbobPhase >= 0 && mthing->FloatbobPhase < 64) mobj->FloatBobPhase = mthing->FloatbobPhase;
	if (mthing->Gravity < 0) mobj->Gravity = -mthing->Gravity;
	else if (mthing->Gravity > 0) mobj->Gravity *= mthing->Gravity;
	else 
	{
		mobj->flags |= MF_NOGRAVITY;
		mobj->Gravity = 0;
	}

	// For Hexen floatbob 'compatibility' we do not really want to alter the floorz.
	if (mobj->specialf1 == 0 || !(mobj->flags2 & MF2_FLOATBOB) || !(ib_compatflags & BCOMPATF_FLOATBOB))
	{
		P_FindFloorCeiling(mobj, FFCF_SAMESECTOR | FFCF_ONLY3DFLOORS | FFCF_3DRESTRICT);
	}

	// if the actor got args defined either in DECORATE or MAPINFO we must ignore the map's properties.
	if (!(mobj->flags2 & MF2_ARGSDEFINED))
	{
		// [RH] Set the thing's special
		mobj->special = mthing->special;
		for(int j=0;j<5;j++) mobj->args[j]=mthing->args[j];
	}

	// [RH] Add ThingID to mobj and link it in with the others
	mobj->tid = mthing->thingid;
	mobj->AddToHash ();

	mobj->PrevAngles.Yaw = mobj->Angles.Yaw = (double)mthing->angle;

	// Check if this actor's mapthing has a conversation defined
	if (mthing->Conversation > 0)
	{
		// Make sure that this does not partially overwrite the default dialogue settings.
		int root = GetConversation(mthing->Conversation);
		if (root != -1)
		{
			mobj->ConversationRoot = root;
			mobj->Conversation = StrifeDialogues[mobj->ConversationRoot];
		}
	}

	// Set various UDMF options
	if (mthing->Alpha >= 0)
		mobj->Alpha = mthing->Alpha;
	if (mthing->RenderStyle != STYLE_Count)
		mobj->RenderStyle = (ERenderStyle)mthing->RenderStyle;
	if (mthing->Scale.X != 0)
		mobj->Scale.X = mthing->Scale.X * mobj->Scale.X;
	if (mthing->Scale.Y != 0)
		mobj->Scale.Y = mthing->Scale.Y * mobj->Scale.Y;
	if (mthing->pitch)
		mobj->Angles.Pitch = (double)mthing->pitch;
	if (mthing->roll)
		mobj->Angles.Roll = (double)mthing->roll;
	if (mthing->score)
		mobj->Score = mthing->score;
	if (mthing->fillcolor)
		mobj->fillcolor = mthing->fillcolor;

	mobj->CallBeginPlay ();
	if (!(mobj->ObjectFlags & OF_EuthanizeMe))
	{
		mobj->LevelSpawned ();
	}

	if (mthing->Health > 0)
		mobj->health = int(mobj->health * mthing->Health);
	else
		mobj->health = -int(mthing->Health);
	if (mthing->Health == 0)
		mobj->CallDie(NULL, NULL);
	else if (mthing->Health != 1)
		mobj->StartHealth = mobj->health;

	return mobj;
}



//
// GAME SPAWN FUNCTIONS
//


//
// P_SpawnPuff
//

AActor *P_SpawnPuff (AActor *source, PClassActor *pufftype, const DVector3 &pos1, DAngle hitdir, DAngle particledir, int updown, int flags, AActor *vict)
{
	AActor *puff;
	DVector3 pos = pos1;

	if (pufftype == nullptr) return nullptr;

	if (!(flags & PF_NORANDOMZ)) pos.Z += pr_spawnpuff.Random2() / 64.;
	puff = Spawn(pufftype, pos, ALLOW_REPLACE);
	if (puff == NULL) return NULL;

	if ((puff->flags4 & MF4_RANDOMIZE) && puff->tics > 0)
	{
		puff->tics -= pr_spawnpuff() & 3;
		if (puff->tics < 1)
			puff->tics = 1;
	}

	//Moved puff creation and target/master/tracer setting to here. 
	if (puff && vict)
	{
		if (puff->flags7 & MF7_HITTARGET)	puff->target = vict;
		if (puff->flags7 & MF7_HITMASTER)	puff->master = vict;
		if (puff->flags7 & MF7_HITTRACER)	puff->tracer = vict;
	}
	// [BB] If the puff came from a player, set the target of the puff to this player.
	if ( puff && (puff->flags5 & MF5_PUFFGETSOWNER))
		puff->target = source;
	
	// Angle is the opposite of the hit direction (i.e. the puff faces the source.)
	puff->Angles.Yaw = hitdir + 180;

	// If a puff has a crash state and an actor was not hit,
	// it will enter the crash state. This is used by the StrifeSpark
	// and BlasterPuff.
	FState *crashstate;
	if (!(flags & PF_HITTHING) && (crashstate = puff->FindState(NAME_Crash)) != NULL)
	{
		puff->SetState (crashstate);
	}
	else if ((flags & PF_HITTHINGBLEED) && (crashstate = puff->FindState(NAME_Death, NAME_Extreme, true)) != NULL)
	{
		puff->SetState (crashstate);
	}
	else if ((flags & PF_MELEERANGE) && puff->MeleeState != NULL)
	{
		// handle the hard coded state jump of Doom's bullet puff
		// in a more flexible manner.
		puff->SetState (puff->MeleeState);
	}

	if (!(flags & PF_TEMPORARY))
	{
		if (cl_pufftype && updown != 3 && (puff->flags4 & MF4_ALLOWPARTICLES))
		{
			P_DrawSplash2 (32, pos, particledir, updown, 1);
			if (cl_pufftype == 1) puff->renderflags |= RF_INVISIBLE;
		}

		if ((flags & PF_HITTHING) && puff->SeeSound)
		{ // Hit thing sound
			S_Sound (puff, CHAN_BODY, puff->SeeSound, 1, ATTN_NORM);
		}
		else if (puff->AttackSound)
		{
			S_Sound (puff, CHAN_BODY, puff->AttackSound, 1, ATTN_NORM);
		}
	}

	return puff;
}

DEFINE_ACTION_FUNCTION(AActor, SpawnPuff)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_CLASS(pufftype, AActor);
	PARAM_FLOAT(x);
	PARAM_FLOAT(y);
	PARAM_FLOAT(z);
	PARAM_ANGLE(hitdir);
	PARAM_ANGLE(particledir);
	PARAM_INT(updown);
	PARAM_INT_DEF(flags);
	PARAM_OBJECT_DEF(victim, AActor);
	ACTION_RETURN_OBJECT(P_SpawnPuff(self, pufftype, DVector3(x, y, z), hitdir, particledir, updown, flags, victim));
}

//---------------------------------------------------------------------------
//
// P_SpawnBlood
// 
//---------------------------------------------------------------------------

void P_SpawnBlood (const DVector3 &pos1, DAngle dir, int damage, AActor *originator)
{
	AActor *th;
	PClassActor *bloodcls = originator->GetBloodType();
	DVector3 pos = pos1;
	pos.Z += pr_spawnblood.Random2() / 64.;

	int bloodtype = cl_bloodtype;
	
	if (bloodcls != NULL && !(GetDefaultByType(bloodcls)->flags4 & MF4_ALLOWPARTICLES))
		bloodtype = 0;

	if (bloodcls != NULL)
	{
		th = Spawn(bloodcls, pos, NO_REPLACE); // GetBloodType already performed the replacement
		th->Vel.Z = 2;
		th->Angles.Yaw = dir;
		// [NG] Applying PUFFGETSOWNER to the blood will make it target the owner
		if (th->flags5 & MF5_PUFFGETSOWNER) th->target = originator;
		if (gameinfo.gametype & GAME_DoomChex)
		{
			th->tics -= pr_spawnblood() & 3;

			if (th->tics < 1)
				th->tics = 1;
		}
		// colorize the blood
		if (!(th->flags2 & MF2_DONTTRANSLATE))
		{
			th->Translation = originator->BloodTranslation;
		}
		
		// Moved out of the blood actor so that replacing blood is easier
		if (gameinfo.gametype & GAME_DoomStrifeChex)
		{
			if (gameinfo.gametype == GAME_Strife)
			{
				if (damage > 13)
				{
					FState *state = th->FindState(NAME_Spray);
					if (state != NULL)
					{
						th->SetState (state);
						goto statedone;
					}
				}
				else damage += 2;
			}
			int advance = 0;
			if (damage <= 12 && damage >= 9)
			{
				advance = 1;
			}
			else if (damage < 9)
			{
				advance = 2;
			}

			PClassActor *cls = th->GetClass();

			while (cls != RUNTIME_CLASS(AActor))
			{
				int checked_advance = advance;
				if (cls->OwnsState(th->SpawnState))
				{
					for (; checked_advance > 0; --checked_advance)
					{
						// [RH] Do not set to a state we do not own.
						if (cls->OwnsState(th->SpawnState + checked_advance))
						{
							th->SetState(th->SpawnState + checked_advance);
							goto statedone;
						}
					}
				}
				// We can safely assume the ParentClass is of type PClassActor
				// since we stop when we see the Actor base class.
				cls = static_cast<PClassActor *>(cls->ParentClass);
			}
		}

	statedone:
		if (!(bloodtype <= 1)) th->renderflags |= RF_INVISIBLE;
	}

	if (bloodtype >= 1)
		P_DrawSplash2 (40, pos, dir, 2, originator->BloodColor);
}

DEFINE_ACTION_FUNCTION(AActor, SpawnBlood)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_FLOAT(x);
	PARAM_FLOAT(y);
	PARAM_FLOAT(z);
	PARAM_ANGLE(dir);
	PARAM_INT(damage);
	P_SpawnBlood(DVector3(x, y, z), dir, damage, self);
	return 0;
}


//---------------------------------------------------------------------------
//
// PROC P_BloodSplatter
//
//---------------------------------------------------------------------------

void P_BloodSplatter (const DVector3 &pos, AActor *originator, DAngle hitangle)
{
	PClassActor *bloodcls = originator->GetBloodType(1); 

	int bloodtype = cl_bloodtype;
	
	if (bloodcls != NULL && !(GetDefaultByType(bloodcls)->flags4 & MF4_ALLOWPARTICLES))
		bloodtype = 0;

	if (bloodcls != NULL)
	{
		AActor *mo;

		mo = Spawn(bloodcls, pos, NO_REPLACE); // GetBloodType already performed the replacement
		mo->target = originator;
		mo->Vel.X = pr_splatter.Random2 () / 64.;
		mo->Vel.Y = pr_splatter.Random2() / 64.;
		mo->Vel.Z = 3;

		// colorize the blood!
		if (!(mo->flags2 & MF2_DONTTRANSLATE)) 
		{
			mo->Translation = originator->BloodTranslation;
		}

		if (!(bloodtype <= 1)) mo->renderflags |= RF_INVISIBLE;
	}
	if (bloodtype >= 1)
	{
		P_DrawSplash2 (40, pos, hitangle-180., 2, originator->BloodColor);
	}
}

//===========================================================================
//
//  P_BloodSplatter2
//
//===========================================================================

void P_BloodSplatter2 (const DVector3 &pos, AActor *originator, DAngle hitangle)
{
	PClassActor *bloodcls = originator->GetBloodType(2);

	int bloodtype = cl_bloodtype;
	
	if (bloodcls != NULL && !(GetDefaultByType(bloodcls)->flags4 & MF4_ALLOWPARTICLES))
		bloodtype = 0;

	DVector2 add;
	add.X = (pr_splat() - 128) / 32.;
	add.Y = (pr_splat() - 128) / 32.;

	if (bloodcls != NULL)
	{
		AActor *mo;


		mo = Spawn (bloodcls, pos + add, NO_REPLACE); // GetBloodType already performed the replacement
		mo->target = originator;

		// colorize the blood!
		if (!(mo->flags2 & MF2_DONTTRANSLATE))
		{
			mo->Translation = originator->BloodTranslation;
		}

		if (!(bloodtype <= 1)) mo->renderflags |= RF_INVISIBLE;
	}
	if (bloodtype >= 1)
	{
		P_DrawSplash2(40, pos + add, hitangle - 180., 2, originator->BloodColor);
	}
}

DEFINE_ACTION_FUNCTION(AActor, BloodSplatter)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_FLOAT(x);
	PARAM_FLOAT(y);
	PARAM_FLOAT(z);
	PARAM_ANGLE(dir);
	PARAM_BOOL_DEF(axe);
	if (axe) P_BloodSplatter2(DVector3(x, y, z), self, dir);
	else P_BloodSplatter(DVector3(x, y, z), self, dir);
	return 0;
}


//---------------------------------------------------------------------------
//
// PROC P_RipperBlood
//
//---------------------------------------------------------------------------

void P_RipperBlood (AActor *mo, AActor *bleeder)
{
	PClassActor *bloodcls = bleeder->GetBloodType();

	double xo = pr_ripperblood.Random2() / 16.;
	double yo = pr_ripperblood.Random2() / 16.;
	double zo = pr_ripperblood.Random2() / 16.;
	DVector3 pos = mo->Vec3Offset(xo, yo, zo);

	int bloodtype = cl_bloodtype;
	
	if (bloodcls != NULL && !(GetDefaultByType(bloodcls)->flags4 & MF4_ALLOWPARTICLES))
		bloodtype = 0;

	if (bloodcls != NULL)
	{
		AActor *th;
		th = Spawn (bloodcls, pos, NO_REPLACE); // GetBloodType already performed the replacement
		// [NG] Applying PUFFGETSOWNER to the blood will make it target the owner
		if (th->flags5 & MF5_PUFFGETSOWNER) th->target = bleeder;
		if (gameinfo.gametype == GAME_Heretic)
			th->flags |= MF_NOGRAVITY;
		th->Vel.X = mo->Vel.X / 2;
		th->Vel.Y = mo->Vel.Y / 2;
		th->tics += pr_ripperblood () & 3;

		// colorize the blood!
		if (!(th->flags2 & MF2_DONTTRANSLATE))
		{
			th->Translation = bleeder->BloodTranslation;
		}

		if (!(bloodtype <= 1)) th->renderflags |= RF_INVISIBLE;
	}
	if (bloodtype >= 1)
	{
		P_DrawSplash2(28, pos, bleeder->AngleTo(mo) + 180., 0, bleeder->BloodColor);
	}
}

//---------------------------------------------------------------------------
//
// FUNC P_GetThingFloorType
//
//---------------------------------------------------------------------------

int P_GetThingFloorType (AActor *thing)
{
	if (thing->floorterrain >= 0)
	{		
		return thing->floorterrain;
	}
	else
	{
		return thing->Sector->GetTerrain(sector_t::floor);
	}
}

DEFINE_ACTION_FUNCTION(AActor, GetFloorTerrain)
{
	PARAM_SELF_PROLOGUE(AActor);
	ACTION_RETURN_POINTER(&Terrains[P_GetThingFloorType(self)]);
}


//---------------------------------------------------------------------------
//
// FUNC P_HitWater
//
// Returns true if hit liquid and splashed, false if not.
//---------------------------------------------------------------------------

bool P_HitWater (AActor * thing, sector_t * sec, const DVector3 &pos, bool checkabove, bool alert, bool force)
{
	if (thing->flags3 & MF3_DONTSPLASH)
		return false;

	if (thing->player && (thing->player->cheats & CF_PREDICTING))
		return false;

	AActor *mo = NULL;
	FSplashDef *splash;
	int terrainnum;
	sector_t *hsec = NULL;
	
	// don't splash above the object
	if (checkabove)
	{
		double compare_z = thing->Center();
		// Missiles are typically small and fast, so they might
		// end up submerged by the move that calls P_HitWater.
		if (thing->flags & MF_MISSILE)
			compare_z -= thing->Vel.Z;
		if (pos.Z > compare_z) 
			return false;
	}

#if 0 // needs some rethinking before activation

	// This avoids spawning splashes on invisible self referencing sectors.
	// For network consistency do this only in single player though because
	// it is not guaranteed that all players have GL nodes loaded.
	if (!multiplayer && thing->subsector->sector != thing->subsector->render_sector)
	{
		double zs = thing->subsector->sector->floorplane.ZatPoint(pos);
		double zr = thing->subsector->render_sector->floorplane.ZatPoint(pos);

		if (zs > zr && thing->Z() >= zs) return false;
	}
#endif

	// 'force' means, we want this sector's terrain, no matter what.
	if (!force)
	{
		for (unsigned int i = 0; i<sec->e->XFloor.ffloors.Size(); i++)
		{
			F3DFloor * rover = sec->e->XFloor.ffloors[i];
			if (!(rover->flags & FF_EXISTS)) continue;
			double planez = rover->top.plane->ZatPoint(pos);
				if (pos.Z > planez - 0.5 && pos.Z < planez + 0.5)	// allow minor imprecisions
			{
				if ((rover->flags & (FF_SOLID | FF_SWIMMABLE)) || rover->alpha > 0)
				{
					terrainnum = rover->model->GetTerrain(rover->top.isceiling);
					goto foundone;
				}
			}
			planez = rover->bottom.plane->ZatPoint(pos);
			if (planez < pos.Z && !(planez < thing->floorz)) return false;
		}
	}
	hsec = sec->GetHeightSec();
	if (force || hsec == NULL || !(hsec->MoreFlags & SECF_CLIPFAKEPLANES))
	{
		terrainnum = sec->GetTerrain(sector_t::floor);
	}
	else
	{
		terrainnum = hsec->GetTerrain(sector_t::floor);
	}
foundone:

	int splashnum = Terrains[terrainnum].Splash;
	bool smallsplash = false;
	const secplane_t *plane;

	if (splashnum == -1)
		return Terrains[terrainnum].IsLiquid;

	// don't splash when touching an underwater floor
	if (thing->waterlevel >= 1 && pos.Z <= thing->floorz) return Terrains[terrainnum].IsLiquid;

	plane = hsec != NULL? &sec->heightsec->floorplane : &sec->floorplane;

	// Don't splash for living things with small vertical velocities.
	// There are levels where the constant splashing from the monsters gets extremely annoying
	if (((thing->flags3&MF3_ISMONSTER || thing->player) && thing->Vel.Z >= -6) && !force)
		return Terrains[terrainnum].IsLiquid;

	splash = &Splashes[splashnum];

	// Small splash for small masses
	if (thing->Mass < 10)
		smallsplash = true;

	if (smallsplash && splash->SmallSplash)
	{
		mo = Spawn (splash->SmallSplash, pos, ALLOW_REPLACE);
		if (mo) mo->Floorclip += splash->SmallSplashClip;
	}
	else
	{
		if (splash->SplashChunk)
		{
			mo = Spawn (splash->SplashChunk, pos, ALLOW_REPLACE);
			mo->target = thing;
			if (splash->ChunkXVelShift != 255)
			{
				mo->Vel.X = (pr_chunk.Random2() << splash->ChunkXVelShift) / 65536.;
			}
			if (splash->ChunkYVelShift != 255)
			{
				mo->Vel.Y = (pr_chunk.Random2() << splash->ChunkYVelShift) / 65536.;
			}
			mo->Vel.Z = splash->ChunkBaseZVel + (pr_chunk() << splash->ChunkZVelShift) / 65536.;
		}
		if (splash->SplashBase)
		{
			mo = Spawn (splash->SplashBase, pos, ALLOW_REPLACE);
		}
		if (thing->player && !splash->NoAlert && alert)
		{
			P_NoiseAlert (thing, thing, true);
		}
	}
	if (mo)
	{
		S_Sound (mo, CHAN_ITEM, smallsplash ?
			splash->SmallSplashSound : splash->NormalSplashSound,
			1, ATTN_IDLE);
	}
	else
	{
		S_Sound (pos, CHAN_ITEM, smallsplash ?
			splash->SmallSplashSound : splash->NormalSplashSound,
			1, ATTN_IDLE);
	}

	// Don't let deep water eat missiles
	return plane == &sec->floorplane ? Terrains[terrainnum].IsLiquid : false;
}

DEFINE_ACTION_FUNCTION(AActor, HitWater)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_POINTER_NOT_NULL(sec, sector_t);
	PARAM_FLOAT(x);
	PARAM_FLOAT(y);
	PARAM_FLOAT(z);
	PARAM_BOOL_DEF(checkabove);
	PARAM_BOOL_DEF(alert);
	PARAM_BOOL_DEF(force);
	ACTION_RETURN_BOOL(P_HitWater(self, sec, DVector3(x, y, z), checkabove, alert, force));
}


//---------------------------------------------------------------------------
//
// FUNC P_HitFloor
//
// Returns true if hit liquid and splashed, false if not.
//---------------------------------------------------------------------------

bool P_HitFloor (AActor *thing)
{
	const msecnode_t *m;

	// killough 11/98: touchy objects explode on impact
	// Allow very short drops to be safe, so that a touchy can be summoned without exploding.
	if (thing->flags6 & MF6_TOUCHY && ((thing->flags6 & MF6_ARMED) || thing->IsSentient()) && thing->Vel.Z < -5)
	{
		thing->flags6 &= ~MF6_ARMED; // Disarm
		P_DamageMobj (thing, NULL, NULL, thing->health, NAME_Crush, DMG_FORCED);  // kill object
		return false;
	}

	if (thing->flags3 & MF3_DONTSPLASH)
		return false;

	// don't splash if landing on the edge above water/lava/etc....
	DVector3 pos;
	for (m = thing->touching_sectorlist; m; m = m->m_tnext)
	{
		pos = thing->PosRelative(m->m_sector);
		if (thing->Z() == m->m_sector->floorplane.ZatPoint(pos))
		{
			break;
		}

		// Check 3D floors
		for(unsigned int i=0;i<m->m_sector->e->XFloor.ffloors.Size();i++)
		{		
			F3DFloor * rover = m->m_sector->e->XFloor.ffloors[i];
			if (!(rover->flags & FF_EXISTS)) continue;
			if (rover->flags & (FF_SOLID|FF_SWIMMABLE))
			{
				if (rover->top.plane->ZatPoint(pos) == thing->Z())
				{
					return P_HitWater (thing, m->m_sector, pos);
				}
			}
		}
	}
	if (m == NULL || m->m_sector->GetHeightSec() != NULL)
	{ 
		return false;
	}

	return P_HitWater (thing, m->m_sector, pos);
}

DEFINE_ACTION_FUNCTION(AActor, HitFloor)
{
	PARAM_SELF_PROLOGUE(AActor);
	ACTION_RETURN_BOOL(P_HitFloor(self));
}

//---------------------------------------------------------------------------
//
// P_CheckSplash
//
// Checks for splashes caused by explosions
//
//---------------------------------------------------------------------------

void P_CheckSplash(AActor *self, double distance)
{
	sector_t *floorsec;
	self->Sector->LowestFloorAt(self, &floorsec);
	if (self->Z() <= self->floorz + distance && self->floorsector == floorsec && self->Sector->GetHeightSec() == NULL && floorsec->heightsec == NULL)
	{
		// Explosion splashes never alert monsters. This is because A_Explode has
		// a separate parameter for that so this would get in the way of proper 
		// behavior.
		DVector3 pos = self->PosRelative(floorsec);
		pos.Z = self->floorz;
		P_HitWater (self, floorsec, pos, false, false);
	}
}

//---------------------------------------------------------------------------
//
// FUNC P_CheckMissileSpawn
//
// Returns true if the missile is at a valid spawn point, otherwise
// explodes it and returns false.
//
//---------------------------------------------------------------------------

bool P_CheckMissileSpawn (AActor* th, double maxdist)
{
	// [RH] Don't decrement tics if they are already less than 1
	if ((th->flags4 & MF4_RANDOMIZE) && th->tics > 0)
	{
		th->tics -= pr_checkmissilespawn() & 3;
		if (th->tics < 1)
			th->tics = 1;
	}

	DVector3 newpos = th->Pos();

	if (maxdist > 0)
	{
		// move a little forward so an angle can be computed if it immediately explodes
		DVector3 advance = th->Vel;
		double maxsquared = maxdist*maxdist;

		// Keep halving the advance vector until we get something less than maxdist
		// units away, since we still want to spawn the missile inside the shooter.
		do
		{
			advance *= 0.5f;
		}
		while (advance.XY().LengthSquared() >= maxsquared);
		newpos += advance;
	}

	FCheckPosition tm(!!(th->flags2 & MF2_RIP));

	// killough 8/12/98: for non-missile objects (e.g. grenades)
	// 
	// [GZ] MBF excludes non-missile objects from the P_TryMove test
	// and subsequent potential P_ExplodeMissile call. That is because
	// in MBF, a projectile is not an actor with the MF_MISSILE flag
	// but an actor with either or both the MF_MISSILE and MF_BOUNCES
	// flags, and a grenade is identified by not having MF_MISSILE.
	// Killough wanted grenades not to explode directly when spawned,
	// therefore they can be fired safely even when humping a wall as
	// they will then just drop on the floor at their shooter's feet.
	//
	// However, ZDoom does allow non-missiles to be shot as well, so
	// Killough's check for non-missiles is inadequate here. So let's
	// replace it by a check for non-missile and MBF bounce type.
	// This should allow MBF behavior where relevant without altering
	// established ZDoom behavior for crazy stuff like a cacodemon cannon.
	bool MBFGrenade = (!(th->flags & MF_MISSILE) || (th->BounceFlags & BOUNCE_MBF));

	// killough 3/15/98: no dropoff (really = don't care for missiles)
	auto oldf2 = th->flags2;
	th->flags2 &= ~MF2_MCROSS;	// The following check is not supposed to activate missile triggers.
	if (!(P_TryMove (th, newpos, false, NULL, tm, true)))
	{
		// [RH] Don't explode ripping missiles that spawn inside something
		if (th->BlockingMobj == NULL || !(th->flags2 & MF2_RIP) || (th->BlockingMobj->flags5 & MF5_DONTRIP))
		{
			// If this is a monster spawned by A_SpawnProjectile subtract it from the counter.
			th->ClearCounters();
			// [RH] Don't explode missiles that spawn on top of horizon lines
			if (th->BlockingLine != NULL && th->BlockingLine->special == Line_Horizon)
			{
				th->Destroy ();
			}
			else if (MBFGrenade && th->BlockingLine != NULL)
			{
				P_BounceWall(th);
			}
			else
			{
				P_ExplodeMissile (th, NULL, th->BlockingMobj);
			}
			return false;
		}
	}
	th->flags2 = oldf2;
	th->ClearInterpolation();
	return true;
}

DEFINE_ACTION_FUNCTION(AActor, CheckMissileSpawn)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_FLOAT(add);
	ACTION_RETURN_BOOL(P_CheckMissileSpawn(self, add));
}


//---------------------------------------------------------------------------
//
// FUNC P_PlaySpawnSound
//
// Plays a missiles spawn sound. Location depends on the
// MF_SPAWNSOUNDSOURCE flag.
//
//---------------------------------------------------------------------------

void P_PlaySpawnSound(AActor *missile, AActor *spawner)
{
	if (missile->SeeSound != 0)
	{
		if (!(missile->flags & MF_SPAWNSOUNDSOURCE))
		{
			S_Sound (missile, CHAN_VOICE, missile->SeeSound, 1, ATTN_NORM);
		}
		else if (spawner != NULL)
		{
			S_Sound (spawner, CHAN_WEAPON, missile->SeeSound, 1, ATTN_NORM);
		}
		else
		{
			// If there is no spawner use the spawn position.
			// But not in a silenced sector.
			if (!(missile->Sector->Flags & SECF_SILENT))
				S_Sound (missile->Pos(), CHAN_WEAPON, missile->SeeSound, 1, ATTN_NORM);
		}
	}
}

DEFINE_ACTION_FUNCTION(AActor, PlaySpawnSound)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT_NOT_NULL(missile, AActor);
	P_PlaySpawnSound(missile, self);
	return 0;
}


static double GetDefaultSpeed(PClassActor *type)
{
	if (type == NULL)
		return 0;

	auto def = GetDefaultByType(type);
	if (G_SkillProperty(SKILLP_FastMonsters))
	{
		double f = def->FloatVar(NAME_FastSpeed);
		if (f >= 0) return f;
	}
	return def->Speed;
}

DEFINE_ACTION_FUNCTION(AActor, GetDefaultSpeed)
{
	PARAM_PROLOGUE;
	PARAM_CLASS(type, AActor);
	ACTION_RETURN_FLOAT(GetDefaultSpeed(type));
}

//---------------------------------------------------------------------------
//
// FUNC P_SpawnMissile
//
// Returns NULL if the missile exploded immediately, otherwise returns
// a mobj_t pointer to the missile.
//
//---------------------------------------------------------------------------

AActor *P_SpawnMissileXYZ (DVector3 pos, AActor *source, AActor *dest, PClassActor *type, bool checkspawn, AActor *owner)
{
	if (source == nullptr || type == nullptr)
	{
		return nullptr;
	}

	if (dest == NULL)
	{
		Printf ("P_SpawnMissilyXYZ: Tried to shoot %s from %s with no dest\n",
			type->TypeName.GetChars(), source->GetClass()->TypeName.GetChars());
		return NULL;
	}

	if (pos.Z != ONFLOORZ && pos.Z != ONCEILINGZ)
	{
		pos.Z -= source->Floorclip;
	}

	AActor *th = Spawn (type, pos, ALLOW_REPLACE);
	
	P_PlaySpawnSound(th, source);

	// record missile's originator
	if (owner == NULL) owner = source;
	th->target = owner;

	double speed = th->Speed;

	// [RH]
	// Hexen calculates the missile velocity based on the source's location.
	// Would it be more useful to base it on the actual position of the
	// missile?
	// Answer: No, because this way, you can set up sets of parallel missiles.

	DVector3 velocity = source->Vec3To(dest);
	// Floor and ceiling huggers should never have a vertical component to their velocity
	if (th->flags3 & (MF3_FLOORHUGGER|MF3_CEILINGHUGGER))
	{
		velocity.Z = 0;
	}
	// [RH] Adjust the trajectory if the missile will go over the target's head.
	else if (pos.Z - source->Z() >= dest->Height)
	{
		velocity.Z += (dest->Height - pos.Z + source->Z());
	}
	th->Vel = velocity.Resized(speed);

	// invisible target: rotate velocity vector in 2D
	// [RC] Now monsters can aim at invisible player as if they were fully visible.
	if (dest->flags & MF_SHADOW && !(source->flags6 & MF6_SEEINVISIBLE))
	{
		DAngle an = pr_spawnmissile.Random2() * (22.5 / 256);
		double c = an.Cos();
		double s = an.Sin();
		
		double newx = th->Vel.X * c - th->Vel.Y * s;
		double newy = th->Vel.X * s + th->Vel.Y * c;

		th->Vel.X = newx;
		th->Vel.Y = newy;
	}

	th->AngleFromVel();

	if (th->flags4 & MF4_SPECTRAL)
	{
		th->SetFriendPlayer(owner->player);
	}

	return (!checkspawn || P_CheckMissileSpawn (th, source->radius)) ? th : NULL;
}

DEFINE_ACTION_FUNCTION(AActor, SpawnMissileXYZ)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_FLOAT(x);
	PARAM_FLOAT(y);
	PARAM_FLOAT(z);
	PARAM_OBJECT_NOT_NULL(dest, AActor);
	PARAM_CLASS(type, AActor);
	PARAM_BOOL_DEF(check);
	PARAM_OBJECT_DEF(owner, AActor);
	ACTION_RETURN_OBJECT(P_SpawnMissileXYZ(DVector3(x,y,z), self, dest, type, check, owner));
}

AActor *P_SpawnMissile(AActor *source, AActor *dest, PClassActor *type, AActor *owner)
{
	if (source == nullptr)
	{
		return nullptr;
	}
	return P_SpawnMissileXYZ(source->PosPlusZ(32 + source->GetBobOffset()), source, dest, type, true, owner);
}

DEFINE_ACTION_FUNCTION(AActor, SpawnMissile)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT_NOT_NULL(dest, AActor);
	PARAM_CLASS(type, AActor);
	PARAM_OBJECT_DEF(owner, AActor);
	ACTION_RETURN_OBJECT(P_SpawnMissile(self, dest, type, owner));
}

AActor *P_SpawnMissileZ(AActor *source, double z, AActor *dest, PClassActor *type)
{
	if (source == nullptr)
	{
		return nullptr;
	}
	return P_SpawnMissileXYZ(source->PosAtZ(z), source, dest, type);
}

DEFINE_ACTION_FUNCTION(AActor, SpawnMissileZ)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_FLOAT(z);
	PARAM_OBJECT_NOT_NULL(dest, AActor);
	PARAM_CLASS(type, AActor);
	ACTION_RETURN_OBJECT(P_SpawnMissileZ(self, z, dest, type));
}



AActor *P_OldSpawnMissile(AActor *source, AActor *owner, AActor *dest, PClassActor *type)
{
	if (source == nullptr || type == nullptr)
	{
		return nullptr;
	}
	AActor *th = Spawn (type, source->PosPlusZ(32.), ALLOW_REPLACE);

	P_PlaySpawnSound(th, source);
	th->target = owner;		// record missile's originator

	th->Angles.Yaw = source->AngleTo(dest);
	th->VelFromAngle();


	double dist = source->DistanceBySpeed(dest, MAX(1., th->Speed));
	th->Vel.Z = (dest->Z() - source->Z()) / dist;

	if (th->flags4 & MF4_SPECTRAL)
	{
		th->SetFriendPlayer(owner->player);
	}

	P_CheckMissileSpawn(th, source->radius);
	return th;
}

DEFINE_ACTION_FUNCTION(AActor, OldSpawnMissile)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT_NOT_NULL(dest, AActor);
	PARAM_CLASS(type, AActor);
	PARAM_OBJECT_DEF(owner, AActor);
	ACTION_RETURN_OBJECT(P_OldSpawnMissile(self, owner, dest, type));
}


//---------------------------------------------------------------------------
//
// FUNC P_SpawnMissileAngle
//
// Returns NULL if the missile exploded immediately, otherwise returns
// a mobj_t pointer to the missile.
//
//---------------------------------------------------------------------------

AActor *P_SpawnMissileAngle (AActor *source, PClassActor *type, DAngle angle, double vz)
{
	if (source == nullptr || type == nullptr)
	{
		return NULL;
	}
	return P_SpawnMissileAngleZSpeed (source, source->Z() + 32 + source->GetBobOffset(), type, angle, vz, GetDefaultSpeed (type));
}

AActor *P_SpawnMissileAngleZ (AActor *source, double z, PClassActor *type, DAngle angle, double vz)
{
	if (type == nullptr)
	{
		return nullptr;
	}
	return P_SpawnMissileAngleZSpeed (source, z, type, angle, vz, GetDefaultSpeed (type));
}

AActor *P_SpawnMissileZAimed (AActor *source, double z, AActor *dest, PClassActor *type)
{
	if (source == nullptr || type == nullptr)
	{
		return nullptr;
	}
	DAngle an;
	double dist;
	double speed;
	double vz;

	an = source->Angles.Yaw;

	if (dest->flags & MF_SHADOW)
	{
		an += pr_spawnmissile.Random2() * (16. / 360.);
	}
	dist = source->Distance2D (dest);
	speed = GetDefaultSpeed (type);
	dist /= speed;
	vz = dist != 0 ? (dest->Z() - source->Z())/dist : speed;
	return P_SpawnMissileAngleZSpeed (source, z, type, an, vz, speed);
}

DEFINE_ACTION_FUNCTION(AActor, SpawnMissileZAimed)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_FLOAT(z);
	PARAM_OBJECT_NOT_NULL(dest, AActor);
	PARAM_CLASS(type, AActor);
	ACTION_RETURN_OBJECT(P_SpawnMissileZAimed(self, z, dest, type));
}

//---------------------------------------------------------------------------
//
// FUNC P_SpawnMissileAngleZSpeed
//
// Returns NULL if the missile exploded immediately, otherwise returns
// a mobj_t pointer to the missile.
//
//---------------------------------------------------------------------------

AActor *P_SpawnMissileAngleZSpeed (AActor *source, double z,
	PClassActor *type, DAngle angle, double vz, double speed, AActor *owner, bool checkspawn)
{
	if (source == nullptr || type == nullptr)
	{
		return nullptr;
	}
	AActor *mo;

	if (z != ONFLOORZ && z != ONCEILINGZ) 
	{
		z -= source->Floorclip;
	}

	mo = Spawn (type, source->PosAtZ(z), ALLOW_REPLACE);

	P_PlaySpawnSound(mo, source);
	if (owner == NULL) owner = source;
	mo->target = owner;
	mo->Angles.Yaw = angle;
	mo->VelFromAngle(speed);
	mo->Vel.Z = vz;

	if (mo->flags4 & MF4_SPECTRAL)
	{
		mo->SetFriendPlayer(owner->player);
	}

	return (!checkspawn || P_CheckMissileSpawn(mo, source->radius)) ? mo : NULL;
}

DEFINE_ACTION_FUNCTION(AActor, SpawnMissileAngleZSpeed)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_FLOAT(z);
	PARAM_CLASS(type, AActor);
	PARAM_ANGLE(angle);
	PARAM_FLOAT(vz);
	PARAM_FLOAT(speed);
	PARAM_OBJECT_DEF(owner, AActor);
	PARAM_BOOL_DEF(checkspawn);
	ACTION_RETURN_OBJECT(P_SpawnMissileAngleZSpeed(self, z, type, angle, vz, speed, owner, checkspawn));
}


AActor *P_SpawnSubMissile(AActor *source, PClassActor *type, AActor *target)
{
	AActor *other = Spawn(type, source->Pos(), ALLOW_REPLACE);

	if (source == nullptr || type == nullptr)
	{
		return nullptr;
	}

	other->target = target;
	other->Angles.Yaw = source->Angles.Yaw;
	other->VelFromAngle();

	if (other->flags4 & MF4_SPECTRAL)
	{
		if (source->flags & MF_MISSILE && source->flags4 & MF4_SPECTRAL)
		{
			other->FriendPlayer = source->FriendPlayer;
		}
		else
		{
			other->SetFriendPlayer(target->player);
		}
	}

	if (P_CheckMissileSpawn(other, source->radius))
	{
		DAngle pitch = P_AimLineAttack(source, source->Angles.Yaw, 1024.);
		other->Vel.Z = -other->Speed * pitch.Sin();
		return other;
	}
	return NULL;
}

DEFINE_ACTION_FUNCTION(AActor, SpawnSubMissile)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_CLASS(cls, AActor);
	PARAM_OBJECT_NOT_NULL(target, AActor);
	ACTION_RETURN_OBJECT(P_SpawnSubMissile(self, cls, target));
}
/*
================
=
= P_SpawnPlayerMissile
=
= Tries to aim at a nearby monster
================
*/

AActor *P_SpawnPlayerMissile (AActor *source, PClassActor *type)
{
	if (source == NULL)
	{
		return NULL;
	}
	return P_SpawnPlayerMissile (source, 0, 0, 0, type, source->Angles.Yaw);
}

AActor *P_SpawnPlayerMissile (AActor *source, PClassActor *type, DAngle angle)
{
	return P_SpawnPlayerMissile (source, 0, 0, 0, type, angle);
}

AActor *P_SpawnPlayerMissile (AActor *source, double x, double y, double z,
							  PClassActor *type, DAngle angle, FTranslatedLineTarget *pLineTarget, AActor **pMissileActor,
							  bool nofreeaim, bool noautoaim, int aimflags)
{
	if (source == nullptr || type == nullptr)
	{
		return nullptr;
	}

	static const double angdiff[3] = { -5.625, 5.625, 0 };
	DAngle an = angle;
	DAngle pitch;
	FTranslatedLineTarget scratch;
	AActor *defaultobject = GetDefaultByType(type);
	DAngle vrange = nofreeaim ? 35. : 0.;

	if (!pLineTarget) pLineTarget = &scratch;
	if (source->player && source->player->ReadyWeapon && ((source->player->ReadyWeapon->WeaponFlags & WIF_NOAUTOAIM) || noautoaim))
	{
		// Keep exactly the same angle and pitch as the player's own aim
		an = angle;
		pitch = source->Angles.Pitch;
		pLineTarget->linetarget = NULL;
	}
	else // see which target is to be aimed at
	{
		// [XA] If MaxTargetRange is defined in the spawned projectile, use this as the
		//      maximum range for the P_AimLineAttack call later; this allows MaxTargetRange
		//      to function as a "maximum tracer-acquisition range" for seeker missiles.
		double linetargetrange = defaultobject->maxtargetrange > 0 ? defaultobject->maxtargetrange*64 : 16*64.;

		int i = 2;
		do
		{
			an = angle + angdiff[i];
			pitch = P_AimLineAttack (source, an, linetargetrange, pLineTarget, vrange, aimflags);
	
			if (source->player != NULL &&
				!nofreeaim &&
				level.IsFreelookAllowed() &&
				source->player->userinfo.GetAimDist() <= 0.5)
			{
				break;
			}
		} while (pLineTarget->linetarget == NULL && --i >= 0);

		if (pLineTarget->linetarget == NULL)
		{
			an = angle;
			if (nofreeaim || !level.IsFreelookAllowed())
			{
				pitch = 0.;
			}
		}
	}

	if (z != ONFLOORZ && z != ONCEILINGZ)
	{
		// Doom spawns missiles 4 units lower than hitscan attacks for players.
		z += source->Center() - source->Floorclip;
		if (source->player != NULL)	// Considering this is for player missiles, it better not be NULL.
		{
			z += ((source->player->mo->AttackZOffset - 4) * source->player->crouchfactor);
		}
		else
		{
			z += 4;
		}
		// Do not fire beneath the floor.
		if (z < source->floorz)
		{
			z = source->floorz;
		}
	}
	DVector3 pos = source->Vec2OffsetZ(x, y, z);
	AActor *MissileActor = Spawn (type, pos, ALLOW_REPLACE);
	if (pMissileActor) *pMissileActor = MissileActor;
	P_PlaySpawnSound(MissileActor, source);
	MissileActor->target = source;
	MissileActor->Angles.Yaw = an;
	if (MissileActor->flags3 & (MF3_FLOORHUGGER | MF3_CEILINGHUGGER))
	{
		MissileActor->VelFromAngle();
	}
	else
	{
		MissileActor->Vel3DFromAngle(pitch, MissileActor->Speed);
	}

	if (MissileActor->flags4 & MF4_SPECTRAL)
	{
		MissileActor->SetFriendPlayer(source->player);
	}
	if (P_CheckMissileSpawn (MissileActor, source->radius))
	{
		return MissileActor;
	}
	return NULL;
}

DEFINE_ACTION_FUNCTION(AActor, SpawnPlayerMissile)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_CLASS(type, AActor);
	PARAM_ANGLE_DEF(angle);
	PARAM_FLOAT_DEF(x);
	PARAM_FLOAT_DEF(y);
	PARAM_FLOAT_DEF(z);
	PARAM_POINTER_DEF(lt, FTranslatedLineTarget);
	PARAM_BOOL_DEF(nofreeaim);
	PARAM_BOOL_DEF(noautoaim);
	PARAM_INT_DEF(aimflags);
	AActor *missileactor;
	if (numparam == 2) angle = self->Angles.Yaw;
	AActor *misl = P_SpawnPlayerMissile(self, x, y, z, type, angle, lt, &missileactor, nofreeaim, noautoaim, aimflags);
	if (numret > 0) ret[0].SetPointer(misl, ATAG_OBJECT);
	if (numret > 1) ret[1].SetPointer(missileactor, ATAG_OBJECT), numret = 2;
	return numret;
}


int AActor::GetTeam()
{
	if (player)
	{
		return player->userinfo.GetTeam();
	}

	int myTeam = DesignatedTeam;

	// Check for monsters that belong to a player on the team but aren't part of the team themselves.
	if (myTeam == TEAM_NONE && FriendPlayer != 0)
	{
		myTeam = players[FriendPlayer - 1].userinfo.GetTeam();
	}
	return myTeam;

}

bool AActor::IsTeammate (AActor *other)
{
	if (!other)
	{
		return false;
	}
	else if (!deathmatch && player && other->player)
	{
		return true;
	}
	else if (teamplay)
	{
		int myTeam = GetTeam();
		int otherTeam = other->GetTeam();

		return (myTeam != TEAM_NONE && myTeam == otherTeam);
	}
	return false;
}

DEFINE_ACTION_FUNCTION(AActor, isTeammate)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT_NOT_NULL(other, AActor);
	ACTION_RETURN_BOOL(self->IsTeammate(other));
}

//==========================================================================
//
// AActor :: GetSpecies
//
// Species is defined as the lowest base class that is a monster
// with no non-monster class in between. If the actor specifies an explicit
// species (i.e. not 'None'), that is used. This is virtualized, so special
// monsters can change this behavior if they like.
//
//==========================================================================

FName AActor::GetSpecies()
{
	if (Species != NAME_None)
	{
		return Species;
	}

	PClassActor *thistype = GetClass();

	if (GetDefaultByType(thistype)->flags3 & MF3_ISMONSTER)
	{
		while (thistype->ParentClass)
		{
			if (GetDefaultByType(thistype->ParentClass)->flags3 & MF3_ISMONSTER)
				thistype = static_cast<PClassActor *>(thistype->ParentClass);
			else 
				break;
		}
	}
	return Species = thistype->TypeName; // [GZ] Speeds up future calls.
}

DEFINE_ACTION_FUNCTION(AActor, GetSpecies)
{
	PARAM_SELF_PROLOGUE(AActor);
	ACTION_RETURN_INT(self->GetSpecies());
}

//==========================================================================
//
// AActor :: IsFriend
//
// Checks if two monsters have to be considered friendly.
//
//==========================================================================

bool AActor::IsFriend (AActor *other)
{
	if (flags & other->flags & MF_FRIENDLY)
	{
		if (deathmatch && teamplay)
			return IsTeammate(other) ||
				(FriendPlayer != 0 && other->FriendPlayer != 0 &&
					players[FriendPlayer-1].mo->IsTeammate(players[other->FriendPlayer-1].mo));

		return !deathmatch ||
			FriendPlayer == other->FriendPlayer ||
			FriendPlayer == 0 ||
			other->FriendPlayer == 0 ||
			players[FriendPlayer-1].mo->IsTeammate(players[other->FriendPlayer-1].mo);
	}
	return false;
}

DEFINE_ACTION_FUNCTION(AActor, isFriend)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT_NOT_NULL(other, AActor);
	ACTION_RETURN_BOOL(self->IsFriend(other));
}

//==========================================================================
//
// AActor :: IsHostile
//
// Checks if two monsters have to be considered hostile under any circumstances
//
//==========================================================================

bool AActor::IsHostile (AActor *other)
{
	// Both monsters are non-friendlies so hostilities depend on infighting settings
	if (!((flags | other->flags) & MF_FRIENDLY)) return false;

	// Both monsters are friendly and belong to the same player if applicable.
	if (flags & other->flags & MF_FRIENDLY)
	{
		if (deathmatch && teamplay)
			return !IsTeammate(other) &&
				!(FriendPlayer != 0 && other->FriendPlayer != 0 &&
					players[FriendPlayer-1].mo->IsTeammate(players[other->FriendPlayer-1].mo));

		return deathmatch &&
			FriendPlayer != other->FriendPlayer &&
			FriendPlayer !=0 &&
			other->FriendPlayer != 0 &&
			!players[FriendPlayer-1].mo->IsTeammate(players[other->FriendPlayer-1].mo);
	}
	return true;
}

DEFINE_ACTION_FUNCTION(AActor, isHostile)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT_NOT_NULL(other, AActor);
	ACTION_RETURN_BOOL(self->IsHostile(other));
}


//==========================================================================
//
// AActor :: DoSpecialDamage
//
// override this for special damage effects.
//
//==========================================================================

int AActor::DoSpecialDamage (AActor *target, int damage, FName damagetype)
{
	if (target->player && target->player->mo == target && damage < 1000 &&
		(target->player->cheats & CF_GODMODE || target->player->cheats & CF_GODMODE2))
	{
		return -1;
	}
	else
	{
		if (target->player)
		{
			// Only do this for old style poison damage.
			if (PoisonDamage > 0 && PoisonDuration == INT_MIN)
			{
				P_PoisonPlayer (target->player, this, this->target, PoisonDamage);
				damage >>= 1;
			}
		}
	
		return damage;
	}
}

DEFINE_ACTION_FUNCTION(AActor, DoSpecialDamage)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT_NOT_NULL(target, AActor);
	PARAM_INT(damage);
	PARAM_NAME(damagetype);
	ACTION_RETURN_INT(self->DoSpecialDamage(target, damage, damagetype));
}

int AActor::CallDoSpecialDamage(AActor *target, int damage, FName damagetype)
{
	IFVIRTUAL(AActor, DoSpecialDamage)
	{
		// Without the type cast this picks the 'void *' assignment...
		VMValue params[4] = { (DObject*)this, (DObject*)target, damage, damagetype.GetIndex() };
		VMReturn ret;
		int retval;
		ret.IntAt(&retval);
		GlobalVMStack.Call(func, params, 4, &ret, 1, nullptr);
		return retval;
	}
	else return DoSpecialDamage(target, damage, damagetype);

}

//==========================================================================
//
// AActor :: TakeSpecialDamage
//
//==========================================================================

int AActor::TakeSpecialDamage (AActor *inflictor, AActor *source, int damage, FName damagetype)
{
	FState *death;

	// If the actor does not have a corresponding death state, then it does not take damage.
	// Note that DeathState matches every kind of damagetype, so an actor has that, it can
	// be hurt with any type of damage. Exception: Massacre damage always succeeds, because
	// it needs to work.

	// Always kill if there is a regular death state or no death states at all.
	if (FindState (NAME_Death) != NULL || !HasSpecialDeathStates() || damagetype == NAME_Massacre)
	{
		return damage;
	}
	
	if (inflictor && inflictor->DeathType != NAME_None)
		damagetype = inflictor->DeathType;

	if (damagetype == NAME_Ice)
	{
		death = FindState (NAME_Death, NAME_Ice, true);
		if (death == NULL && !deh.NoAutofreeze && !(flags4 & MF4_NOICEDEATH) &&
			(player || (flags3 & MF3_ISMONSTER)))
		{
			death = FindState(NAME_GenericFreezeDeath);
		}
	}
	else
	{
		death = FindState (NAME_Death, damagetype);
	}
	return (death == NULL) ? -1 : damage;
}

DEFINE_ACTION_FUNCTION(AActor, TakeSpecialDamage)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT(inflictor, AActor);
	PARAM_OBJECT(source, AActor);
	PARAM_INT(damage);
	PARAM_NAME(damagetype);
	ACTION_RETURN_INT(self->TakeSpecialDamage(inflictor, source, damage, damagetype));
}

int AActor::CallTakeSpecialDamage(AActor *inflictor, AActor *source, int damage, FName damagetype)
{
	IFVIRTUAL(AActor, TakeSpecialDamage)
	{
		VMValue params[5] = { (DObject*)this, inflictor, source, damage, damagetype.GetIndex() };
		VMReturn ret;
		int retval;
		ret.IntAt(&retval);
		GlobalVMStack.Call(func, params, 5, &ret, 1, nullptr);
		return retval;
	}
	else return TakeSpecialDamage(inflictor, source, damage, damagetype);

}

void AActor::Crash()
{
	// [RC] Weird that this forces the Crash state regardless of flag.
	if(!(flags6 & MF6_DONTCORPSE))
	{
	if (((flags & MF_CORPSE) || (flags6 & MF6_KILLED)) &&
		!(flags3 & MF3_CRASHED) &&
		!(flags & MF_ICECORPSE))
	{
		FState *crashstate = NULL;
		
		int gibh = GetGibHealth();
		if (DamageType != NAME_None)
		{
			if (health < gibh)
			{ // Extreme death
				FName labels[] = { NAME_Crash, NAME_Extreme, DamageType };
				crashstate = FindState (3, labels, true);
			}
			if (crashstate == NULL)
			{ // Normal death
				crashstate = FindState(NAME_Crash, DamageType, true);
			}
		}
		if (crashstate == NULL)
		{
			if (health < gibh)
			{ // Extreme death
				crashstate = FindState(NAME_Crash, NAME_Extreme);
			}
			else
			{ // Normal death
				crashstate = FindState(NAME_Crash);
			}
		}
		if (crashstate != NULL) SetState(crashstate);
		// Set MF3_CRASHED regardless of the presence of a crash state
		// so this code doesn't have to be executed repeatedly.
		flags3 |= MF3_CRASHED;
	}
	}
}

void AActor::SetIdle(bool nofunction)
{
	FState *idle = FindState (NAME_Idle);
	if (idle == NULL) idle = SpawnState;
	SetState(idle, nofunction);
}

DEFINE_ACTION_FUNCTION(AActor, SetIdle)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_BOOL_DEF(nofunction);
	self->SetIdle(nofunction);
	return 0;
}

int AActor::SpawnHealth() const
{
	int defhealth = StartHealth ? StartHealth : GetDefault()->health;
	if (!(flags3 & MF3_ISMONSTER) || defhealth == 0)
	{
		return defhealth;
	}
	else if (flags & MF_FRIENDLY)
	{
		int adj = int(defhealth * G_SkillProperty(SKILLP_FriendlyHealth));
		return (adj <= 0) ? 1 : adj;
	}
	else
	{
		int adj = int(defhealth * G_SkillProperty(SKILLP_MonsterHealth));
		return (adj <= 0) ? 1 : adj;
	}
}

DEFINE_ACTION_FUNCTION(AActor, SpawnHealth)
{
	PARAM_SELF_PROLOGUE(AActor);
	ACTION_RETURN_INT(self->SpawnHealth());
}

FState *AActor::GetRaiseState()
{
	if (!(flags & MF_CORPSE))
	{
		return NULL;	// not a monster
	}

	if (tics != -1 && // not lying still yet
		!state->GetCanRaise()) // or not ready to be raised yet
	{
		return NULL;
	}

	if (IsKindOf(RUNTIME_CLASS(APlayerPawn)))
	{
		return NULL;	// do not resurrect players
	}

	return FindState(NAME_Raise);
}

void AActor::Revive()
{
	AActor *info = GetDefault();
	flags = info->flags;
	flags2 = info->flags2;
	flags3 = info->flags3;
	flags4 = info->flags4;
	flags5 = info->flags5;
	flags6 = info->flags6;
	flags7 = info->flags7;
	if (SpawnFlags & MTF_FRIENDLY) flags |= MF_FRIENDLY;
	DamageType = info->DamageType;
	health = SpawnHealth();
	target = NULL;
	lastenemy = NULL;

	// [RH] If it's a monster, it gets to count as another kill
	if (CountsAsKill())
	{
		level.total_monsters++;
	}

	// [ZZ] resurrect hook
	E_WorldThingRevived(this);
}

int AActor::GetGibHealth() const
{
	IFVIRTUAL(AActor, GetGibHealth)
	{
		VMValue params[] = { (DObject*)this };
		int h;
		VMReturn ret(&h);
		GlobalVMStack.Call(func, params, 1, &ret, 1);
		return h;
	}
	return -SpawnHealth();
}

double AActor::GetCameraHeight() const
{
	return CameraHeight == INT_MIN ? Height / 2 : CameraHeight;
}

DEFINE_ACTION_FUNCTION(AActor, GetCameraHeight)
{
	PARAM_SELF_PROLOGUE(AActor);
	ACTION_RETURN_FLOAT(self->GetCameraHeight());
}


FDropItem *AActor::GetDropItems() const
{
	return GetClass()->DropItems;
}

DEFINE_ACTION_FUNCTION(AActor, GetDropItems)
{
	PARAM_SELF_PROLOGUE(AActor);
	ACTION_RETURN_OBJECT(self->GetDropItems());
}

double AActor::GetGravity() const
{
	if (flags & MF_NOGRAVITY) return 0;
	return level.gravity * Sector->gravity * Gravity * 0.00125;
}

DEFINE_ACTION_FUNCTION(AActor, GetGravity)
{
	PARAM_SELF_PROLOGUE(AActor);
	ACTION_RETURN_FLOAT(self->GetGravity());
}


// killough 11/98:
// Whether an object is "sentient" or not. Used for environmental influences.
// (left precisely the same as MBF even though it doesn't make much sense.)
bool AActor::IsSentient() const
{
	return health > 0 && SeeState != NULL;
}


FSharedStringArena AActor::mStringPropertyData;

const char *AActor::GetTag(const char *def) const
{
	if (Tag != NULL)
	{
		const char *tag = Tag->GetChars();
		if (tag[0] == '$')
		{
			return GStrings(tag + 1);
		}
		else
		{
			return tag;
		}
	}
	else if (def)
	{
		return def;
	}
	else
	{
		return GetClass()->TypeName.GetChars();
	}
}

DEFINE_ACTION_FUNCTION(AActor, GetTag)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_STRING(def);
	ACTION_RETURN_STRING(self->GetTag(def.Len() == 0? nullptr : def.GetChars()));
}

void AActor::SetTag(const char *def)
{
	if (def == NULL || *def == 0) Tag = nullptr;
	else Tag = mStringPropertyData.Alloc(def);
}

DEFINE_ACTION_FUNCTION(AActor, SetTag)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_STRING(def);
	if (def.IsEmpty()) self->Tag = nullptr;
	else self->Tag = self->mStringPropertyData.Alloc(def);
	return 0;
}


void AActor::ClearCounters()
{
	if (CountsAsKill() && health > 0)
	{
		level.total_monsters--;
		flags &= ~MF_COUNTKILL;
	}
	// Same, for items
	if (flags & MF_COUNTITEM)
	{
		level.total_items--;
		flags &= ~MF_COUNTITEM;
	}
	// And finally for secrets
	if (flags5 & MF5_COUNTSECRET)
	{
		level.total_secrets--;
		flags5 &= ~MF5_COUNTSECRET;
	}
}

DEFINE_ACTION_FUNCTION(AActor, ClearCounters)
{
	PARAM_SELF_PROLOGUE(AActor);
	self->ClearCounters();
	return 0;
}

int AActor::GetModifiedDamage(FName damagetype, int damage, bool passive)
{
	auto inv = Inventory;
	while (inv != nullptr)
	{
		IFVIRTUALPTR(inv, AInventory, ModifyDamage)
		{
			VMValue params[5] = { (DObject*)inv, damage, int(damagetype), &damage, passive };
			GlobalVMStack.Call(func, params, 5, nullptr, 0, nullptr);
		}
		inv = inv->Inventory;
	}
	return damage;
}

DEFINE_ACTION_FUNCTION(AActor, GetModifiedDamage)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_NAME(type);
	PARAM_INT(damage);
	PARAM_BOOL(passive);
	ACTION_RETURN_INT(self->GetModifiedDamage(type, damage, passive));
}

int AActor::ApplyDamageFactor(FName damagetype, int damage) const
{
	damage = int(damage * DamageFactor);
	if (damage > 0)
	{
		damage = DamageTypeDefinition::ApplyMobjDamageFactor(damage, damagetype, GetClass()->DamageFactors);
	}
	return damage;
}

DEFINE_ACTION_FUNCTION(AActor, ApplyDamageFactor)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_NAME(type);
	PARAM_INT(damage);
	ACTION_RETURN_INT(self->ApplyDamageFactor(type, damage));
}


void AActor::SetTranslation(FName trname)
{
	// There is no constant for the empty name...
	if (trname.GetChars()[0] == 0)
	{
		// an empty string resets to the default
		Translation = GetDefault()->Translation;
		return;
	}

	int tnum = R_FindCustomTranslation(trname);
	if (tnum >= 0)
	{
		Translation = tnum;
	}
	// silently ignore if the name does not exist, this would create some insane message spam otherwise.
}

//---------------------------------------------------------------------------
//
// PROP A_RestoreSpecialPosition
//
//---------------------------------------------------------------------------
static FRandom pr_restore("RestorePos");

void AActor::RestoreSpecialPosition()
{
	// Move item back to its original location
	DVector2 sp = SpawnPoint;

	FLinkContext ctx;
	UnlinkFromWorld(&ctx);
	SetXY(sp);
	LinkToWorld(&ctx, true);
	SetZ(Sector->floorplane.ZatPoint(sp));
	P_FindFloorCeiling(this, FFCF_ONLYSPAWNPOS | FFCF_NOPORTALS);	// no portal checks here so that things get spawned in this sector.

	if (flags & MF_SPAWNCEILING)
	{
		SetZ(ceilingz - Height - SpawnPoint.Z);
	}
	else if (flags2 & MF2_SPAWNFLOAT)
	{
		double space = ceilingz - Height - floorz;
		if (space > 48)
		{
			space -= 40;
			SetZ((space * pr_restore()) / 256. + floorz + 40);
		}
		else
		{
			SetZ(floorz);
		}
	}
	else
	{
		SetZ(SpawnPoint.Z + floorz);
	}
	// Redo floor/ceiling check, in case of 3D floors and portals
	P_FindFloorCeiling(this, FFCF_SAMESECTOR | FFCF_ONLY3DFLOORS | FFCF_3DRESTRICT);
	if (Z() < floorz)
	{ // Do not reappear under the floor, even if that's where we were for the
	  // initial spawn.
		SetZ(floorz);
	}
	if ((flags & MF_SOLID) && (Top() > ceilingz))
	{ // Do the same for the ceiling.
		SetZ(ceilingz - Height);
	}
	// Do not interpolate from the position the actor was at when it was
	// picked up, in case that is different from where it is now.
	ClearInterpolation();
}

DEFINE_ACTION_FUNCTION(AActor, A_RestoreSpecialPosition)
{
	PARAM_SELF_PROLOGUE(AActor);
	self->RestoreSpecialPosition();
	return 0;
}

double AActor::GetBobOffset(double ticfrac) const
{
	if (!(flags2 & MF2_FLOATBOB))
	{
		return 0;
	}
	return BobSin(FloatBobPhase + level.maptime + ticfrac);
}

DEFINE_ACTION_FUNCTION(AActor, GetBobOffset)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_FLOAT_DEF(frac);
	ACTION_RETURN_FLOAT(self->GetBobOffset(frac));
}




class DActorIterator : public DObject, public NActorIterator
{
	DECLARE_ABSTRACT_CLASS(DActorIterator, DObject)

public:
	DActorIterator(PClassActor *cls= nullptr, int tid = 0)
		: NActorIterator(cls, tid)
	{
	}
};

IMPLEMENT_CLASS(DActorIterator, true, false);
DEFINE_ACTION_FUNCTION(DActorIterator, Create)
{
	PARAM_PROLOGUE;
	PARAM_INT(tid);
	PARAM_CLASS_DEF(type, AActor);
	ACTION_RETURN_OBJECT(new DActorIterator(type, tid));
}

DEFINE_ACTION_FUNCTION(DActorIterator, Next)
{
	PARAM_SELF_PROLOGUE(DActorIterator);
	ACTION_RETURN_OBJECT(self->Next());
}

DEFINE_ACTION_FUNCTION(DActorIterator, Reinit)
{
	PARAM_SELF_PROLOGUE(DActorIterator);
	self->Reinit();
	return 0;
}



DEFINE_ACTION_FUNCTION(AActor, deltaangle)	// should this be global?
{
	PARAM_PROLOGUE;
	PARAM_FLOAT(a1);
	PARAM_FLOAT(a2);
	ACTION_RETURN_FLOAT(deltaangle(DAngle(a1), DAngle(a2)).Degrees);
}

DEFINE_ACTION_FUNCTION(AActor, absangle)	// should this be global?
{
	PARAM_PROLOGUE;
	PARAM_FLOAT(a1);
	PARAM_FLOAT(a2);
	ACTION_RETURN_FLOAT(absangle(DAngle(a1), DAngle(a2)).Degrees);
}

DEFINE_ACTION_FUNCTION(AActor, Distance2D)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT_NOT_NULL(other, AActor);
	ACTION_RETURN_FLOAT(self->Distance2D(other));
}

DEFINE_ACTION_FUNCTION(AActor, Distance3D)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT_NOT_NULL(other, AActor);
	ACTION_RETURN_FLOAT(self->Distance3D(other));
}

DEFINE_ACTION_FUNCTION(AActor, AddZ)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_FLOAT(addz);
	PARAM_BOOL_DEF(moving);
	self->AddZ(addz, moving);
	return 0;
}

DEFINE_ACTION_FUNCTION(AActor, SetZ)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_FLOAT(z);
	self->SetZ(z);
	return 0;
}

DEFINE_ACTION_FUNCTION(AActor, SetDamage)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_INT(dmg);
	self->SetDamage(dmg);
	return 0;
}

DEFINE_ACTION_FUNCTION(AActor, GetDefaultByType)
{
	PARAM_PROLOGUE;
	PARAM_CLASS(cls, AActor);
	ACTION_RETURN_OBJECT(cls == nullptr? nullptr : GetDefaultByType(cls));
}

// This combines all 3 variations of the internal function
DEFINE_ACTION_FUNCTION(AActor, VelFromAngle)
{
	PARAM_SELF_PROLOGUE(AActor);
	if (numparam == 1)
	{
		self->VelFromAngle();
	}
	else
	{
		PARAM_FLOAT(speed);
		if (numparam == 2)
		{
			self->VelFromAngle(speed);
		}
		else
		{
			PARAM_ANGLE(angle);
			self->VelFromAngle(speed, angle);
		}
	}
	return 0;
}

// This combines all 3 variations of the internal function
DEFINE_ACTION_FUNCTION(AActor, Vel3DFromAngle)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_FLOAT(speed);
	PARAM_ANGLE(angle);
	PARAM_ANGLE(pitch);
	self->Vel3DFromAngle(angle, pitch, speed);
	return 0;
}

// This combines all 3 variations of the internal function
DEFINE_ACTION_FUNCTION(AActor, Thrust)
{
	PARAM_SELF_PROLOGUE(AActor);
	if (numparam == 1)
	{
		self->Thrust();
	}
	else
	{
		PARAM_FLOAT(speed);
		if (numparam == 2)
		{
			self->Thrust(speed);
		}
		else
		{
			PARAM_ANGLE(angle);
			self->Thrust(angle, speed);
		}
	}
	return 0;
}

DEFINE_ACTION_FUNCTION(AActor, AngleTo)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT_NOT_NULL(targ, AActor);
	PARAM_BOOL_DEF(absolute);
	ACTION_RETURN_FLOAT(self->AngleTo(targ, absolute).Degrees);
}

DEFINE_ACTION_FUNCTION(AActor, AngleToVector)
{
	PARAM_PROLOGUE;
	PARAM_ANGLE(angle);
	PARAM_FLOAT_DEF(length);
	ACTION_RETURN_VEC2(angle.ToVector(length));
}

DEFINE_ACTION_FUNCTION(AActor, RotateVector)
{
	PARAM_PROLOGUE;
	PARAM_FLOAT(x);
	PARAM_FLOAT(y);
	PARAM_ANGLE(angle);
	ACTION_RETURN_VEC2(DVector2(x, y).Rotated(angle));
}

DEFINE_ACTION_FUNCTION(AActor, Normalize180)
{
	PARAM_PROLOGUE;
	PARAM_ANGLE(angle);
	ACTION_RETURN_FLOAT(angle.Normalized180().Degrees);
}

DEFINE_ACTION_FUNCTION(AActor, DistanceBySpeed)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT_NOT_NULL(targ, AActor);
	PARAM_FLOAT(speed);
	ACTION_RETURN_FLOAT(self->DistanceBySpeed(targ, speed));
}

DEFINE_ACTION_FUNCTION(AActor, SetXYZ)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_FLOAT(x);
	PARAM_FLOAT(y);
	PARAM_FLOAT(z);
	self->SetXYZ(x, y, z);
	return 0; 
}

DEFINE_ACTION_FUNCTION(AActor, Vec2Angle)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_FLOAT(length);
	PARAM_ANGLE(angle);
	PARAM_BOOL_DEF(absolute);
	ACTION_RETURN_VEC2(self->Vec2Angle(length, angle, absolute));
}


DEFINE_ACTION_FUNCTION(AActor, Vec3To)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT_NOT_NULL(t, AActor)
	ACTION_RETURN_VEC3(self->Vec3To(t));
}

DEFINE_ACTION_FUNCTION(AActor, Vec2To)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_OBJECT_NOT_NULL(t, AActor)
		ACTION_RETURN_VEC2(self->Vec2To(t));
}

DEFINE_ACTION_FUNCTION(AActor, Vec3Angle)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_FLOAT(length)
		PARAM_ANGLE(angle);
	PARAM_FLOAT(z);
	PARAM_BOOL_DEF(absolute);
	ACTION_RETURN_VEC3(self->Vec3Angle(length, angle, z, absolute));
}

DEFINE_ACTION_FUNCTION(AActor, Vec2OffsetZ)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_FLOAT(x);
	PARAM_FLOAT(y);
	PARAM_FLOAT(z);
	PARAM_BOOL_DEF(absolute);
	ACTION_RETURN_VEC3(self->Vec2OffsetZ(x, y, z, absolute));
}

DEFINE_ACTION_FUNCTION(AActor, Vec2Offset)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_FLOAT(x);
	PARAM_FLOAT(y);
	PARAM_BOOL_DEF(absolute);
	ACTION_RETURN_VEC2(self->Vec2Offset(x, y, absolute));
}

DEFINE_ACTION_FUNCTION(AActor, Vec3Offset)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_FLOAT(x);
	PARAM_FLOAT(y);
	PARAM_FLOAT(z);
	PARAM_BOOL_DEF(absolute);
	ACTION_RETURN_VEC3(self->Vec3Offset(x, y, z, absolute));
}

DEFINE_ACTION_FUNCTION(AActor, PosRelative)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_POINTER(sec, sector_t);
	ACTION_RETURN_VEC3(self->PosRelative(sec));
}

DEFINE_ACTION_FUNCTION(AActor, RestoreDamage)
{
	PARAM_SELF_PROLOGUE(AActor);
	self->RestoreDamage();
	return 0;
}

DEFINE_ACTION_FUNCTION(AActor, PlayerNumber)
{
	PARAM_SELF_PROLOGUE(AActor);
	ACTION_RETURN_INT(self->player ? int(self->player - players) : 0);
}

DEFINE_ACTION_FUNCTION(AActor, SetFriendPlayer)
{
	PARAM_SELF_PROLOGUE(AActor);
	PARAM_POINTER(player, player_t);
	self->SetFriendPlayer(player);
	return 0;
}

DEFINE_ACTION_FUNCTION(AActor, ClearBounce)
{
	PARAM_SELF_PROLOGUE(AActor);
	self->BounceFlags = 0;
	return 0;
}

DEFINE_ACTION_FUNCTION(AActor, AccuracyFactor)
{
	PARAM_SELF_PROLOGUE(AActor);
	ACTION_RETURN_FLOAT(self->AccuracyFactor());
}

DEFINE_ACTION_FUNCTION(AActor, CountsAsKill)
{
	PARAM_SELF_PROLOGUE(AActor);
	ACTION_RETURN_FLOAT(self->CountsAsKill());
}

DEFINE_ACTION_FUNCTION(AActor, IsZeroDamage)
{
	PARAM_SELF_PROLOGUE(AActor);
	ACTION_RETURN_BOOL(self->IsZeroDamage());
}

DEFINE_ACTION_FUNCTION(AActor, ClearInterpolation)
{
	PARAM_SELF_PROLOGUE(AActor);
	self->ClearInterpolation();
	return 0;
}

DEFINE_ACTION_FUNCTION(AActor, ApplyDamageFactors)
{
	PARAM_PROLOGUE;
	PARAM_CLASS(itemcls, AInventory);
	PARAM_NAME(damagetype);
	PARAM_INT(damage);
	PARAM_INT(defdamage);

	DmgFactors *df = itemcls->DamageFactors;
	if (df != nullptr && df->CountUsed() != 0)
	{
		ACTION_RETURN_INT(df->Apply(damagetype, damage));
	}
	else
	{
		ACTION_RETURN_INT(defdamage);
	}
}


//----------------------------------------------------------------------------
//
// DropItem handling
//
//----------------------------------------------------------------------------

DEFINE_FIELD(FDropItem, Next)
DEFINE_FIELD(FDropItem, Name)
DEFINE_FIELD(FDropItem, Probability)
DEFINE_FIELD(FDropItem, Amount)

void PrintMiscActorInfo(AActor *query)
{
	if (query)
	{
		int flagi;
		int querystyle = STYLE_Count;
		for (int style = STYLE_None; style < STYLE_Count; ++style)
		{ // Check for a legacy render style that matches.
			if (LegacyRenderStyles[style] == query->RenderStyle)
			{
				querystyle = style;
				break;
			}
		}
		static const char * renderstyles[]= {"None", "Normal", "Fuzzy", "SoulTrans",
			"OptFuzzy", "Stencil", "Translucent", "Add", "Shaded", "TranslucentStencil",
			"Shadow", "Subtract", "AddStencil", "AddShaded"};

		FLineSpecial *spec = P_GetLineSpecialInfo(query->special);

		Printf("%s @ %p has the following flags:\n   flags: %x", query->GetTag(), query, query->flags.GetValue());
		for (flagi = 0; flagi <= 31; flagi++)
			if (query->flags & ActorFlags::FromInt(1<<flagi)) Printf(" %s", FLAG_NAME(1<<flagi, flags));
		Printf("\n   flags2: %x", query->flags2.GetValue());
		for (flagi = 0; flagi <= 31; flagi++)
			if (query->flags2 & ActorFlags2::FromInt(1<<flagi)) Printf(" %s", FLAG_NAME(1<<flagi, flags2));
		Printf("\n   flags3: %x", query->flags3.GetValue());
		for (flagi = 0; flagi <= 31; flagi++)
			if (query->flags3 & ActorFlags3::FromInt(1<<flagi)) Printf(" %s", FLAG_NAME(1<<flagi, flags3));
		Printf("\n   flags4: %x", query->flags4.GetValue());
		for (flagi = 0; flagi <= 31; flagi++)
			if (query->flags4 & ActorFlags4::FromInt(1<<flagi)) Printf(" %s", FLAG_NAME(1<<flagi, flags4));
		Printf("\n   flags5: %x", query->flags5.GetValue());
		for (flagi = 0; flagi <= 31; flagi++)
			if (query->flags5 & ActorFlags5::FromInt(1<<flagi)) Printf(" %s", FLAG_NAME(1<<flagi, flags5));
		Printf("\n   flags6: %x", query->flags6.GetValue());
		for (flagi = 0; flagi <= 31; flagi++)
			if (query->flags6 & ActorFlags6::FromInt(1<<flagi)) Printf(" %s", FLAG_NAME(1<<flagi, flags6));
		Printf("\n   flags7: %x", query->flags7.GetValue());
		for (flagi = 0; flagi <= 31; flagi++)
			if (query->flags7 & ActorFlags7::FromInt(1<<flagi)) Printf(" %s", FLAG_NAME(1<<flagi, flags7));
		Printf("\n   flags8: %x", query->flags8.GetValue());
		for (flagi = 0; flagi <= 31; flagi++)
			if (query->flags8 & ActorFlags8::FromInt(1<<flagi)) Printf(" %s", FLAG_NAME(1<<flagi, flags8));
		Printf("\nBounce flags: %x\nBounce factors: f:%f, w:%f", 
			query->BounceFlags.GetValue(), query->bouncefactor,
			query->wallbouncefactor);
		/*for (flagi = 0; flagi < 31; flagi++)
			if (query->BounceFlags & 1<<flagi) Printf(" %s", flagnamesb[flagi]);*/
		Printf("\nRender style = %i:%s, alpha %f\nRender flags: %x", 
			querystyle, (querystyle < STYLE_Count ? renderstyles[querystyle] : "Unknown"),
			query->Alpha, query->renderflags.GetValue());
		/*for (flagi = 0; flagi < 31; flagi++)
			if (query->renderflags & 1<<flagi) Printf(" %s", flagnamesr[flagi]);*/
		Printf("\nSpecial+args: %s(%i, %i, %i, %i, %i)\nspecial1: %i, special2: %i.",
			(spec ? spec->name : "None"),
			query->args[0], query->args[1], query->args[2], query->args[3], 
			query->args[4],	query->special1, query->special2);
		Printf("\nTID: %d", query->tid);
		Printf("\nCoord= x: %f, y: %f, z:%f, floor:%f, ceiling:%f, height= %f",
			query->X(), query->Y(), query->Z(),
			query->floorz, query->ceilingz, query->Height);
		Printf("\nSpeed= %f, velocity= x:%f, y:%f, z:%f, combined:%f.\n",
			query->Speed, query->Vel.X, query->Vel.Y, query->Vel.Z, query->Vel.Length());
		Printf("Scale: x:%f, y:%f\n", query->Scale.X, query->Scale.Y);
	}
}
