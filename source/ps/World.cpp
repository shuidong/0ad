#include "precompiled.h"

#include "CStr.h"
#include "CLogger.h"
#include "ps/Errors.h"

#include "World.h"
#include "MapReader.h"
#include "Game.h"
#include "Terrain.h"
#include "LightEnv.h"
#include "BaseEntityCollection.h"
#include "EntityManager.h"
#include "timer.h"
#include "Loader.h"

#define LOG_CATEGORY "world"

CLightEnv g_LightEnv;

void CWorld::Initialize(CGameAttributes *pAttribs)
{
	TIMER(CWorld__Initialize);

	// TODO: Find a better way of handling these global things
	ONCE(g_EntityTemplateCollection.loadTemplates());

	// Load the map, if one was specified
	if (pAttribs->m_MapFile.Length())
	{
		CStr mapfilename("maps/scenarios/");

		mapfilename += (CStr)pAttribs->m_MapFile;

		try {
			CMapReader reader;
			reader.LoadMap(mapfilename, &m_Terrain, &m_UnitManager, &g_LightEnv);
		} catch (...) {
			LOG(ERROR, LOG_CATEGORY, "Failed to load map %s", mapfilename.c_str());
			throw PSERROR_Game_World_MapLoadFailed();
		}
	}
}

struct ThunkParams
{
	CWorld* const this_;
	CGameAttributes* const pAttribs;
	ThunkParams(CWorld* this__, CGameAttributes* pAttribs_)
		: this_(this__), pAttribs(pAttribs_) {}
};

static int LoadThunk(void* param, double time_left)
{
	const ThunkParams* p = (const ThunkParams*)param;
	CWorld* const this_             = p->this_;
	CGameAttributes* const pAttribs = p->pAttribs;

	this_->Initialize(pAttribs);
	delete p;
	return 0;
}

void CWorld::RegisterInit(CGameAttributes *pAttribs)
{
	void* param = new ThunkParams(this, pAttribs);
	THROW_ERR(LDR_Register(LoadThunk, param, L"CWorld", 1000));
}


CWorld::~CWorld()
{
	// The Entity Manager should perhaps be converted into a CWorld member..
	// But for now, we'll just create and delete the global singleton instance
	// following the creation and deletion of CWorld.
	// The reason for not keeping the instance around is that we require a
	// clean slate for each game start.
	delete &m_EntityManager;
}
