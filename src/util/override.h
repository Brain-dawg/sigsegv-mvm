#ifndef _INCLUDE_SIGSEGV_UTIL_OVERRIDE_H_
#define _INCLUDE_SIGSEGV_UTIL_OVERRIDE_H_


class IOverride
{
public:
	virtual ~IOverride() = default;
	
	bool IsEnabled() const { return this->m_bEnabled; }
	
	void Enable()  { this->DoEnable();  this->m_bEnabled = true;  }
	void Disable() { this->DoDisable(); this->m_bEnabled = false; }
    bool Toggle() {
        this->m_bEnabled ? this->Disable() : this->Enable();
        return this->m_bEnabled;
    }
	
protected:
	IOverride() = default;
	
	virtual void DoEnable()  = 0;
	virtual void DoDisable() = 0;
	
private:
	bool m_bEnabled = false;
};


template<typename T, typename = std::enable_if_t<std::is_base_of_v<IOverride, T>>>
class COverrideRAII : public T
{
public:
	template<typename... ARGS>
	COverrideRAII(ARGS&&... args) : T(std::forward<ARGS>(args)...)
	{
		this->Enable();
	}
	virtual ~COverrideRAII()
	{
		this->Disable();
	}
};


class IConVarOverride : public IOverride
{
protected:
	IConVarOverride(const char *name) : m_pszConVarName(name),    m_pIConVar(nullptr) {}
	IConVarOverride(IConVar *p_icvar) : m_pszConVarName(nullptr), m_pIConVar(p_icvar) {}
public:	
	bool IsValid() const
	{
		this->InitOnDemand();
		return (this->m_pConVarRef != nullptr && this->m_pConVarRef->IsValid());
	}
protected:	
	ConVarRef& CVRef()
	{
		this->InitOnDemand();
		return *(this->m_pConVarRef);
	}
	
private:
	void InitOnDemand() const
	{
		if (this->m_pConVarRef == nullptr) {
			/* ensure that only one or the other is set */
			assert(this->m_pszConVarName != nullptr || this->m_pIConVar != nullptr);
			assert(this->m_pszConVarName == nullptr || this->m_pIConVar == nullptr);
			
			if (this->m_pIConVar != nullptr) {
				this->m_pConVarRef = std::make_unique<ConVarRef>(this->m_pIConVar);
			} else {
				this->m_pConVarRef = std::make_unique<ConVarRef>(this->m_pszConVarName);
			}
		}
	}
public:
	const char *const m_pszConVarName;
private:
	IConVar *const m_pIConVar;
	
	mutable std::unique_ptr<ConVarRef> m_pConVarRef;
};


class CConVarOverride_Flags : public IConVarOverride
{
public:
	CConVarOverride_Flags(const char *name, int flags_add, int flags_del)
		: IConVarOverride(name), m_nFlagsAdd(flags_add), m_nFlagsDel(flags_del) {}
	CConVarOverride_Flags(IConVar *p_icvar, int flags_add, int flags_del)
		: IConVarOverride(p_icvar), m_nFlagsAdd(flags_add), m_nFlagsDel(flags_del) {}
	
private:
	virtual void DoEnable() override
	{
		if (!this->IsValid()) return;
		
		this->m_nFlagsRestore = CVRef().Ref_Flags();
		
		CVRef().Ref_Flags() |=  this->m_nFlagsAdd;
		CVRef().Ref_Flags() &= ~this->m_nFlagsDel;
	}
	
	virtual void DoDisable() override
	{
		if (!this->IsValid()) return;
		
		CVRef().Ref_Flags() = this->m_nFlagsRestore;
	}
	
	int m_nFlagsAdd;
	int m_nFlagsDel;
	int m_nFlagsRestore;
};


class CConVarOverride_FloatVal : public IConVarOverride
{
public:
	CConVarOverride_FloatVal(const char *name, float value)
		: IConVarOverride(name), m_flValue(value) {}
	CConVarOverride_FloatVal(IConVar *p_icvar, float value)
		: IConVarOverride(p_icvar), m_flValue(value) {}
	
private:
	virtual void DoEnable() override
	{
		if (!this->IsValid()) return;
		
		this->m_flValueRestore = CVRef().Ref_FloatVal();
		
		CVRef().Ref_FloatVal() = this->m_flValue;
	}
	
	virtual void DoDisable() override
	{
		if (!this->IsValid()) return;
		
		CVRef().Ref_FloatVal() = this->m_flValueRestore;
	}
	
	float m_flValue;
	float m_flValueRestore;
};


class CConVarOverride_IntVal : public IConVarOverride
{
public:
	CConVarOverride_IntVal(const char *name, int value)
		: IConVarOverride(name), m_nValue(value) {}
	CConVarOverride_IntVal(IConVar *p_icvar, int value)
		: IConVarOverride(p_icvar), m_nValue(value) {}
	
private:
	virtual void DoEnable() override
	{
		if (!this->IsValid()) return;
		
		this->m_nValueRestore = CVRef().Ref_IntVal();
		
		CVRef().Ref_IntVal() = this->m_nValue;
	}
	
	virtual void DoDisable() override
	{
		if (!this->IsValid()) return;
		
		CVRef().Ref_IntVal() = this->m_nValueRestore;
	}
	
	int m_nValue;
	int m_nValueRestore;
};


#endif
