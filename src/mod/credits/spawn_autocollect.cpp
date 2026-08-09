#ifdef SE_IS_TF2
#include "mod.h"
#include "stub/nav.h"
#include "stub/tfentities.h"
#include "stub/gamerules.h"
#include "stub/entities.h"

namespace Mod::Credits::Spawn_AutoCollect
{
	void AutoCollect(CCurrencyPack *pack)
	{
		if (!pack->IsDistributed())
			TFGameRules()->DistributeCurrencyAmount(pack->GetAmount(), NULL, true, false, false);

		pack->SetTouched(true);
		UTIL_Remove(pack);
	}

	DETOUR_DECL_MEMBER(void, CCurrencyPack_ComeToRest)
	{
		auto item = reinterpret_cast<CItem *>(this);
		item->ComeToRest(); // BaseClass::ComeToRest()

		auto pack = reinterpret_cast<CCurrencyPack *>(this);

		if (pack->IsMarkedForDeletion() || pack->IsClaimed())
			return;

		if (TheNavMesh->GetNavArea(pack->GetAbsOrigin()) == NULL ||
			PointInRespawnRoom(NULL, pack->GetAbsOrigin(), false))
		{
			AutoCollect(pack);
			return;
		}

		for (auto elem : ITriggerHurtAutoList::AutoList())
		{
			auto trigger = rtti_scast<CTriggerHurt *>(elem);
			if (trigger == nullptr)
				continue;

			if (!trigger->m_bDisabled && trigger->PointIsWithin(pack->GetAbsOrigin()))
			{
				AutoCollect(pack);
				return;
			}
		}
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("Credits:Spawn_AutoCollect")
		{
			MOD_ADD_DETOUR_MEMBER(CCurrencyPack_ComeToRest, "CCurrencyPack::ComeToRest");
		}
	};
	CMod s_Mod;

	ConVar cvar_enable("sig_credits_spawn_autocollect_fix", "0", FCVAR_NOTIFY,
		"Mod: Fix credit autocollection by triggers using AABB collision instead of BSP.",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
#endif
