#ifndef _INCLUDE_SIGSEGV_MEM_DETOUR_H_
#define _INCLUDE_SIGSEGV_MEM_DETOUR_H_


#include "abi.h"
#include "library.h"
#include "util/scope.h"
#include "mem/virtual_hook.h"


class IDetour
{
public:

	enum DetourPriority : int
	{
		LOWEST = -2,
		LOW = -1,
		NORMAL = 0,
		HIGH = 1,
		HIGHEST = 2
	};

	virtual ~IDetour() {}
	
	virtual const char *GetName() const = 0;
	virtual void *GetFuncPtr() const = 0;
	
	bool Load();
	void Unload();
	
	bool IsLoaded() const { return this->m_bLoaded; }
	bool IsActive() const { return this->m_bActive; }
	
	void Toggle(bool enable);
	void Enable();
	void Disable();

	DetourPriority GetPriority() const { return this->m_Priority; }
	void SetPriority(DetourPriority priority) { this->m_Priority = priority; }
	
protected:
	IDetour() {}
	
	virtual bool DoLoad() = 0;
	virtual void DoUnload() = 0;
	
	virtual void DoEnable();
	virtual void DoDisable();
	
private:
	bool m_bLoaded = false;
	bool m_bActive = false;
	DetourPriority m_Priority = NORMAL;
};


class IDetour_SymNormal : public IDetour
{
public:
	virtual const char *GetName() const override { return this->m_strName.c_str(); }
	virtual void *GetFuncPtr() const override { return this->m_pFunc; }
	
protected:
	IDetour_SymNormal(const char *name, void *func_ptr) :
		m_strName(name), m_bFuncByName(false), m_pFunc(func_ptr) {}
	IDetour_SymNormal(const char *name, const char *func_name) :
		m_strName(name), m_bFuncByName(true), m_strFuncName(func_name) {}
	
	virtual bool DoLoad() override;
	virtual void DoUnload() override;
	
private:
	std::string m_strName;
	
	bool m_bFuncByName = false;
	std::string m_strFuncName;
	void *m_pFunc = nullptr;
};


class IDetour_SymRegex : public IDetour
{
public:
	virtual const char *GetName() const override;
	virtual void *GetFuncPtr() const override { return this->m_pFunc; }
	
protected:
	IDetour_SymRegex(Library lib, const char *pattern) :
		m_Library(lib), m_strPattern(pattern) {}
	
	virtual bool DoLoad() override;
	virtual void DoUnload() override;
	
private:
	Library m_Library;
	std::string m_strPattern;
	
	std::string m_strSymbol;
	std::string m_strDemangled;
	void *m_pFunc = nullptr;
};


class CDetour : public IDetour_SymNormal
{
public:
	/* by pointer */
	CDetour(const char *name, void *func_ptr, void *callback, void **inner_ptr, DetourPriority priority = NORMAL) :
		IDetour_SymNormal(name, func_ptr), m_pCallback(callback), m_pInner(inner_ptr) { SetPriority(priority); }
	/* by addr name */
	CDetour(const char *func_name, void *callback, void **inner_ptr, DetourPriority priority = NORMAL) :
		IDetour_SymNormal(func_name, func_name), m_pCallback(callback), m_pInner(inner_ptr) { SetPriority(priority); }
	
private:
	virtual bool DoLoad() override;
	virtual void DoUnload() override;
	
	virtual void DoEnable() override;
	virtual void DoDisable() override;
	
	bool EnsureUniqueInnerPtrs();
	
	void *m_pCallback;
	void **m_pInner;
	
	static inline std::list<CDetour *> s_LoadedDetours;
	static inline std::list<CDetour *> s_ActiveDetours;
	
	friend class CDetouredFunc;
};


class ITrace
{
public:
	virtual ~ITrace() {}
	
	virtual void TracePre() = 0;
	virtual void TracePost() = 0;
	
protected:
	ITrace() {}
};


class CFuncCount : public IDetour_SymNormal, public ITrace
{
public:
	CFuncCount(RefCount& rc, const char *name, void *func_ptr) :
		IDetour_SymNormal(name, func_ptr), m_RefCount(rc) {}
	CFuncCount(RefCount& rc, const char *func_name) :
		IDetour_SymNormal(func_name, func_name), m_RefCount(rc) {}
	
	virtual void TracePre() override;
	virtual void TracePost() override;
	
private:
	RefCount& m_RefCount;
};


class CFuncCallback : public IDetour_SymNormal, public ITrace
{
public:
	using Callback_t = void (*)();
	
	CFuncCallback(Callback_t pre, Callback_t post, const char *name, void *func_ptr) :
		IDetour_SymNormal(name, func_ptr), m_pCBPre(pre), m_pCBPost(post) {}
	CFuncCallback(Callback_t pre, Callback_t post, const char *func_name) :
		IDetour_SymNormal(func_name, func_name), m_pCBPre(pre), m_pCBPost(post) {}
	
	virtual void TracePre() override;
	virtual void TracePost() override;
	
private:
	Callback_t m_pCBPre;
	Callback_t m_pCBPost;
};


class CFuncTrace : public IDetour_SymRegex, public ITrace
{
public:
	CFuncTrace(Library lib, const char *pattern) :
		IDetour_SymRegex(lib, pattern) {}
	
	virtual void TracePre() override;
	virtual void TracePost() override;
};


class CFuncBacktrace : public IDetour_SymRegex, public ITrace
{
public:
	CFuncBacktrace(Library lib, const char *pattern) :
		IDetour_SymRegex(lib, pattern) {}
	
	virtual void TracePre() override;
	virtual void TracePost() override;
};


class CFuncVProf : public IDetour_SymRegex, public ITrace
{
public:
	CFuncVProf(Library lib, const char *pattern, const char *vprof_name, const char *vprof_group) :
		IDetour_SymRegex(lib, pattern), m_strVProfName(vprof_name), m_strVProfGroup(vprof_group) {}
	
	virtual void TracePre() override;
	virtual void TracePost() override;
	
private:
	std::string m_strVProfName;
	std::string m_strVProfGroup;
};


class CDetouredFunc
{
public:
	CDetouredFunc(void *func_ptr);
	~CDetouredFunc();
	
	static CDetouredFunc& Find(void *func_ptr);
	static CDetouredFunc *FindOptional(void *func_ptr);
	
	static void CleanUp();
	
	void AddDetour(CDetour *detour);
	void RemoveDetour(CDetour *detour);
	
	void AddTrace(ITrace *trace);
	void RemoveTrace(ITrace *trace);

	void TemporaryDisable();
	void TemporaryEnable();
	
private:
	void RemoveAllDetours();
	
	void CreateWrapper();
	void DestroyWrapper();
	
	void CreateTrampoline();
	void DestroyTrampoline();
	
	void Reconfigure();
	
	void InstallJump(void *target);
	void UninstallJump();
	
	/* validation: ensuring that nothing else (e.g. SourceHook) has meddled with our bytes while we weren't looking */
	bool Validate(const uint8_t *ptr, const std::vector<uint8_t>& vec, const char *caller);
	bool ValidateOriginalPrologue() { return this->Validate(this->m_pFunc,       this->m_OriginalPrologue, "ValidateOriginalPrologue"); }
	bool ValidateCurrentPrologue()  { return this->Validate(this->m_pFunc,       this->m_CurrentPrologue,  "ValidateCurrentPrologue" ); }
	bool ValidateWrapper()          { return this->Validate(this->m_pWrapper,    this->m_WrapperCheck,     "ValidateWrapper"         ); }
	bool ValidateTrampoline()       { return this->Validate(this->m_pTrampoline, this->m_TrampolineCheck,  "ValidateTrampoline"      ); }
	
#if !defined _WINDOWS
	void FuncPre();
	void FuncPost();
#endif
	
	uint8_t *m_pFunc;
	
	std::vector<CDetour *> m_Detours;
	std::vector<ITrace *> m_Traces;
	
	bool m_bJumpInstalled = false;
	bool m_bJumpIsRelative = false;
	size_t m_zJumpSize = 0;
	
	std::vector<uint8_t> m_OriginalPrologue;     // backup of the original unmodified function prologue
	std::vector<uint8_t> m_TrueOriginalPrologue; // FOR VALIDATION: copy of the original unmodified function prologue, done at extension start only
	std::vector<uint8_t> m_CurrentPrologue;  // FOR VALIDATION: copy of what we believe the current hooked prologue should be
	std::vector<uint8_t> m_WrapperCheck;     // FOR VALIDATION: copy of what we believe the wrapper contents should be
	std::vector<uint8_t> m_TrampolineCheck;  // FOR VALIDATION: copy of what we believe the trampoline contents should be
	
	uint8_t *m_pWrapper    = nullptr;
	uint8_t *m_pTrampoline = nullptr;

	bool m_bModifiedByPatch = false;

	CVirtualHookBase m_VirtualHookOptional;
	void *m_pVirtualHookInner;
	std::set<std::pair<void **, void *>> m_FoundFuncPtrAndVTablePtr;
	
#if !defined _WINDOWS && defined TRACE_DETOUR_ENABLED
	void *m_pWrapperPre   = reinterpret_cast<void *>(&WrapperPre);
	void *m_pWrapperPost  = reinterpret_cast<void *>(&WrapperPost);
	void *m_pWrapperInner = nullptr;

	/* we use this to store the actual-func's caller's return address */
	static inline thread_local std::stack<uint32_t> s_WrapperCallerRetAddrs;
	__fastcall static void WrapperPre(void *func_ptr, const uint32_t *retaddr_save);
	__fastcall static void WrapperPost(void *func_ptr, uint32_t *retaddr_restore);
#endif
	
	static inline std::map<void *, CDetouredFunc> s_FuncMap;
};

// x64 srcds uses an optimization which allows callee registers to be reused after function call if the compiler can detect they are not being used. This makes many detours potentially unsafe without wrapping callee registers first
class RegWrapper {
public:
	RegWrapper() {
        asm("push %rdi\n"
            "push %rsi\n"
            "push %rdx\n"
            "push %rcx\n"
            "push %r8\n"
            "push %r9\n");
    }
    ~RegWrapper() {
        asm("pop %r9\n"
            "pop %r8\n"
            "pop %rcx\n"
            "pop %rdx\n"
            "pop %rsi\n"
            "pop %rdi\n");
    }
};
class RegWrapper_rsi {
public:
	RegWrapper_rsi() {
        asm("push %rsi\n");
    }
    ~RegWrapper_rsi() {
        asm("pop %rsi\n");
    }
};

#ifdef PLATFORM_64BITS
#define REG_WRAPPER(reg) RegWrapper_##reg _regwrapper_##reg;
#define REG_WRAPPER_ALL RegWrapper _regwrapper;
#else
#define REG_WRAPPER(reg)
#define REG_WRAPPER_ALL
#endif

#if !defined(_WINDOWS) || defined(PLATFORM_64BITS)
#define DETOUR_MEMBER_CALL(...) (Actual)(this, ## __VA_ARGS__)
#else
#define DETOUR_MEMBER_CALL(...) (this->*Actual)(__VA_ARGS__)
#endif
#define DETOUR_STATIC_CALL(...) (Actual)(__VA_ARGS__)

#define __DETOUR_DECL_STATIC(prefix, name, ...) \
namespace detour_ns_##name {\
	prefix Detour_##name(__VA_ARGS__); \
	prefix (*Actual)(__VA_ARGS__) = nullptr; \
}\
	CDetour *detour_##name= nullptr; \
	prefix detour_ns_##name::Detour_##name(__VA_ARGS__)

#define DETOUR_DECL_STATIC(ret, name, ...) \
	__DETOUR_DECL_STATIC(static ret, name, ##__VA_ARGS__)
#define DETOUR_DECL_STATIC_CALL_CONVENTION(cc, ret, name, ...) \
	__DETOUR_DECL_STATIC(cc static ret, name, ##__VA_ARGS__)

#if !defined(_WINDOWS) || defined(PLATFORM_64BITS)
// Call original function as a static function with this argument, avoids fat pointer conversion
#define DETOUR_DECL_MEMBER(ret, name, ...) \
	class Detour_##name \
	{ \
	public: \
		ret callback(__VA_ARGS__); \
		static ret (*Actual)(Detour_##name *, ## __VA_ARGS__); \
	}; \
	static CDetour *detour_##name = nullptr; \
	ret (* Detour_##name::Actual)(Detour_##name *, ## __VA_ARGS__) = nullptr; \
	ret Detour_##name::callback(__VA_ARGS__)


#define DETOUR_DECL_MEMBER_CALL_CONVENTION(cc ,ret, name, ...) \
	class Detour_##name \
	{ \
	public: \
		cc ret callback(__VA_ARGS__); \
		cc static ret (*Actual)(Detour_##name *, ## __VA_ARGS__); \
	}; \
	static CDetour *detour_##name = nullptr; \
	cc ret (* Detour_##name::Actual)(Detour_##name *, ## __VA_ARGS__) = nullptr; \
	cc ret Detour_##name::callback(__VA_ARGS__)
#else
// Call original function as a member function
#define DETOUR_DECL_MEMBER(ret, name, ...) \
	class Detour_##name \
	{ \
	public: \
		ret callback(__VA_ARGS__); \
		static ret (Detour_##name::* Actual)(__VA_ARGS__); \
	}; \
	static CDetour *detour_##name = nullptr; \
	ret (Detour_##name::* Detour_##name::Actual)(__VA_ARGS__) = nullptr; \
	ret Detour_##name::callback(__VA_ARGS__)


#define DETOUR_DECL_MEMBER_CALL_CONVENTION(cc ,ret, name, ...) \
	class Detour_##name \
	{ \
	public: \
		cc ret callback(__VA_ARGS__); \
		cc static ret (Detour_##name::* Actual)(__VA_ARGS__); \
	}; \
	static CDetour *detour_##name = nullptr; \
	cc ret (Detour_##name::* Detour_##name::Actual)(__VA_ARGS__) = nullptr; \
	cc ret Detour_##name::callback(__VA_ARGS__)
#endif

#define GET_MEMBER_CALLBACK(name) GetAddrOfMemberFunc(&Detour_##name::callback)
#define GET_MEMBER_INNERPTR(name) reinterpret_cast<void **>(&Detour_##name::Actual)

#define GET_STATIC_CALLBACK(name) reinterpret_cast<void *>(&detour_ns_##name::Detour_##name)
#define GET_STATIC_INNERPTR(name) reinterpret_cast<void **>(&detour_ns_##name::Actual)


// TODO: have an "exclusive" version of CDetour, which is identical, but when it's enabled/disabled,
// make it ensure that it's the only active detour for that function
// (also check whenever trying to enable/disable other detours at that func)


// PHASE 3:
// implement the whole set of pseudo-detour fixtures


// there will be one "master detour" that is actually called directly
// it will call into the chain of regular detours
// before and after that, it will do lightweight pre- and post- tasks


// IDetour: anything that is detour-ish, which a mod can enable/disable

// CDetour: regular detour
// lowest priority

// RefCount thing:
// increments/decrements RefCount when the master detour is active

// pre-notify:
// read-only detour variant that just informs you of the parameters before the call

// post-notify:
// read-only detour variant that just informs you of the parameters/retval after the call

// pre-hook:
// just a callback that happens before the call, no info given

// post-hook:
// just a callback that happens after the call, no info given


#endif
