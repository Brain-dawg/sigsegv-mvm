#ifdef SE_IS_TF2
#include "re/path.h"
#include "stub/tfbot.h"
#include "util/clientmsg.h"


#define MAX_SEGMENTS 256

#define SEG_GROUND SegmentType::TYPE0

#define RESULT_SUCCESS (ResultType)0
#define RESULT_FAIL1   (ResultType)1
#define RESULT_FAIL2   (ResultType)2

#define NUM_TRAVERSE_TYPES 9


// template<class PathCost> bool Path::Compute(INextBot *nextbot, const Vector& goal, PathCost& cost_func, float maxPathLength, bool b1)
// {
// 	VPROF_BUDGET("Path::Compute(goal)", "NextBotSpiky");
	
// 	this->Invalidate();
	
// 	const Vector& nb_pos = nextbot->GetPosition();
	
// 	CNavArea *startArea = nextbot->GetEntity()->GetLastKnownArea();
// 	if (startArea == nullptr) {
// 		this->OnPathChanged(nextbot, RESULT_FAIL2);
// 		return false;
// 	}
	
// 	CNavArea *goalArea = TheNavMesh->GetNearestNavArea(goal, true, 200.0f, true, true);
// 	if (startArea == goalArea) {
// 		this->BuildTrivialPath(nextbot, goal);
// 		return true;
// 	}
	
// 	// TODO: better name for goal2
// 	Vector goal2 = goal;
// 	if (goalArea != nullptr) {
// 		goal2.z = goalArea->GetZ(goal2.x, goal2.y);
// 	} else {
// 		TheNavMesh->GetGroundHeight(goal2, &goal2.z);
// 	}
	
// 	CNavArea *closestArea = nullptr;
// 	bool success = NavAreaBuildPath(startArea, goalArea, &goal, cost_func, &closestArea, maxPathLength, nextbot->GetEntity()->GetTeamNumber());
	
// 	if (closestArea == nullptr) {
// 		return false;
// 	}
	
// 	int num_segments = 0;
// 	for (CNavArea *area = closestArea; area != nullptr; area = area->GetParent()) {
// 		++num_segments;
		
// 		if (area == startArea)                break;
// 		if (num_segments >= MAX_SEGMENTS - 1) break;
// 	}
	
// 	if (num_segments == 1) {
// 		this->BuildTrivialPath(nextbot, goal);
// 		return success;
// 	}
	
// 	this->m_iSegCount = num_segments;
	
// 	CNavArea *area = closestArea;
// 	for (int i = num_segments; i != 0 && area != nullptr; --i, area = area->GetParent()) {
// 		Segment *seg = &this->m_Segments[i - 1];
		
// 		seg->area = area;
// 		seg->how  = area->GetParentHow();
// 		seg->type = SEG_GROUND;
// 	}
	
// 	// TODO: name for b1
// 	if (success || b1) {
// 		Segment *seg_last = &this->m_Segments[this->m_iSegCount];
		
// 		seg_last->area   = closestArea;
// 		seg_last->pos    = goal2;
// 		seg_last->ladder = nullptr;
// 		seg_last->how    = NUM_TRAVERSE_TYPES;
// 		seg_last->type   = SEG_GROUND;
		
// 		++this->m_iSegCount;
// 	}
	
// 	if (!this->ComputePathDetails(nextbot, nb_pos)) {
// 		this->Invalidate();
// 		this->OnPathChanged(nextbot, RESULT_FAIL2);
// 		return false;
// 	}
	
// 	this->Optimize(nextbot);
// 	this->PostProcess();
	
// 	this->OnPathChanged(nextbot, (success ? RESULT_SUCCESS : RESULT_FAIL1));
	
// 	return success;
// }

// template<class PathCost> bool Path::Compute(INextBot *nextbot, CBaseCombatCharacter *subject, PathCost& cost_func, float maxPathLength, bool b1)
// {
// 	VPROF_BUDGET("Path::Compute(subject)", "NextBotSpiky");
	
// 	this->Invalidate();
	
// 	this->m_hSubject = subject;
	
// 	const Vector& nb_pos = nextbot->GetPosition();
	
// 	CNavArea *startArea = nextbot->GetEntity()->GetLastKnownArea();
// 	if (startArea == nullptr) {
// 		this->OnPathChanged(nextbot, RESULT_FAIL2);
// 		return false;
// 	}
	
// 	CNavArea *goalArea = subject->GetLastKnownArea();
// 	if (goalArea == nullptr) {
// 		this->OnPathChanged(nextbot, RESULT_FAIL2);
// 		return false;
// 	}
	
// 	Vector goal = subject->GetAbsOrigin();
	
// 	if (startArea == goalArea) {
// 		this->BuildTrivialPath(nextbot, goal);
// 		return true;
// 	}
	
// 	CNavArea *closestArea = nullptr;
// 	bool success = NavAreaBuildPath(startArea, goalArea, &goal, cost_func, &closestArea, maxPathLength, nextbot->GetEntity()->GetTeamNumber());
	
// 	if (closestArea == nullptr) {
// 		return false;
// 	}
	
// 	int num_segments = 0;
// 	for (CNavArea *area = closestArea; area != nullptr; area = area->GetParent()) {
// 		++num_segments;
		
// 		if (area == startArea)                break;
// 		if (num_segments >= MAX_SEGMENTS - 1) break;
// 	}
	
// 	if (num_segments == 1) {
// 		this->BuildTrivialPath(nextbot, goal);
// 		return success;
// 	}
	
// 	this->m_iSegCount = num_segments;
	
// 	CNavArea *area = closestArea;
// 	for (int i = num_segments; i != 0 && area != nullptr; --i, area = area->GetParent()) {
// 		Segment *seg = &this->m_Segments[i - 1];
		
// 		seg->area = area;
// 		seg->how  = area->GetParentHow();
// 		seg->type = SEG_GROUND;
// 	}
	
// 	// TODO: name for b1
// 	if (success || b1) {
// 		Segment *seg_last = &this->m_Segments[this->m_iSegCount];
		
// 		seg_last->area   = closestArea;
// 		seg_last->pos    = goal;
// 		seg_last->ladder = nullptr;
// 		seg_last->how    = NUM_TRAVERSE_TYPES;
// 		seg_last->type   = SEG_GROUND;
		
// 		++this->m_iSegCount;
// 	}
	
// 	if (!this->ComputePathDetails(nextbot, nb_pos)) {
// 		this->Invalidate();
// 		this->OnPathChanged(nextbot, RESULT_FAIL2);
// 		return false;
// 	}
	
// 	this->Optimize(nextbot);
// 	this->PostProcess();
	
// 	this->OnPathChanged(nextbot, (success ? RESULT_SUCCESS : RESULT_FAIL1));
	
// 	return success;
// }

MemberFuncThunk<Path *, bool, INextBot *, CBaseCombatCharacter *, CTFBotPathCost &, float, bool> ft_Path_CTFBotPathCost_Compute("Path::Compute<CTFBotPathCost> [ent]");
template<class PathCost> bool Path::Compute(INextBot *nextbot, CBaseCombatCharacter *subject, PathCost& cost_func, float maxPathLength, bool b1)
{
	return ft_Path_CTFBotPathCost_Compute(this, nextbot, subject, reinterpret_cast<CTFBotPathCost &>(cost_func), maxPathLength, b1);
}

MemberFuncThunk<Path *, bool, INextBot *, const Vector &, CTFBotPathCost &, float, bool> ft_Path_CTFBotPathCost_Compute_Vector("Path::Compute<CTFBotPathCost> [vec]");
template<class PathCost> bool Path::Compute(INextBot *nextbot, const Vector &subject, PathCost& cost_func, float maxPathLength, bool b1)
{
	return ft_Path_CTFBotPathCost_Compute_Vector(this, nextbot, subject, reinterpret_cast<CTFBotPathCost &>(cost_func), maxPathLength, b1);
}

template bool Path::Compute(INextBot *nextbot, CBaseCombatCharacter *subject, CTFBotPathCost& cost_func, float maxPathLength, bool b1);
template bool Path::Compute(INextBot *nextbot, CBaseCombatCharacter *subject, CZombiePathCost& cost_func, float maxPathLength, bool b1);
template bool Path::Compute(INextBot *nextbot, const Vector&subject, CTFBotPathCost& cost_func, float maxPathLength, bool b1);
template bool Path::Compute(INextBot *nextbot, const Vector&subject, CZombiePathCost& cost_func, float maxPathLength, bool b1);

const Vector& Path::GetStartPosition() const
{
	if (!this->IsValid()) {
		return vec3_origin;
	}
	
	// TODO: name for vector_8
	return this->m_Segments[0].pos;
}

const Vector& Path::GetEndPosition() const
{
	if (!this->IsValid()) {
		return vec3_origin;
	}
	
	// TODO: name for vector_8
	return this->m_Segments[this->m_iSegCount - 1].pos;
}

float Path::GetLength() const
{
	if (!this->IsValid()) {
		return 0.0f;
	}
	
	return this->m_Segments[this->m_iSegCount - 1].distanceFromStart;
}

CTFBotPathCost::CTFBotPathCost(CTFBot *actor, RouteType rtype) :
	m_Actor(actor), m_iRouteType(rtype)
{
	this->m_flStepHeight      = actor->GetLocomotionInterface()->GetStepHeight();
	this->m_flMaxJumpHeight   = actor->GetLocomotionInterface()->GetMaxJumpHeight();
	this->m_flDeathDropHeight = actor->GetLocomotionInterface()->GetDeathDropHeight();
	
	if (actor->IsPlayerClass(TF_CLASS_SPY)) {
		TheNavMesh->CollectBuiltObjects(&this->m_EnemyObjects, GetEnemyTeam(actor));
	} else {
		this->m_EnemyObjects.RemoveAll();
	}
}
#endif
