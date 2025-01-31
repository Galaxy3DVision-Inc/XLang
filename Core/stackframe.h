#pragma once
#include "def.h"
#include "value.h"
#include "lvalue.h"
#include <iostream>
#include "Locker.h"

namespace X {namespace AST {
class Scope;
class Expression;
class StackFrame
{
	Locker m_lock;
protected:
	StackFrame* m_prev = nil;
	StackFrame* m_next = nil;
	Scope* m_pScope = nil;
	int m_varCnt = 0;
	X::Value* m_Values = nil;
	X::Value m_retVal;
	int m_lineStart = -1;
	int m_charPos = 0;
#if XLANG_ENG_DBG
	void ObjDbgSet(XObj* pObj);
	void ObjDbgRemove(XObj* pObj);
#endif
public:
	StackFrame()
	{
	}
	StackFrame(Scope* s)
	{
		m_pScope = s;
	}
	~StackFrame()
	{
		AutoLock lock(m_lock);
		if (m_Values)
		{
#if XLANG_ENG_DBG
			for (int i = 0; i < m_varCnt; i++)
			{
				auto& v = m_Values[i];
				if (v.IsObject())
				{
					ObjDbgRemove(v.GetObj());
				}
			}
#endif
			delete[] m_Values;
		}
	}
	bool AddVar(XlangRuntime* rt,std::string& name, X::Value& val);
	inline virtual int GetStartLine() { return m_lineStart; }
	inline void SetLine(int l) { m_lineStart = l; }
	inline void SetCharPos(int c) { m_charPos = c; }
	inline virtual int GetCharPos() { return m_charPos; }
	inline Scope* GetScope() { return m_pScope; }
	inline void SetNext(StackFrame* n) { m_next = n; if(n) n->m_prev = this; }
	inline void SetPrev(StackFrame* p) { m_prev = p; if(p) p->m_next = this; }
	inline StackFrame* Next() { return m_next; }
	inline StackFrame* Prev() { return m_prev; }
	inline bool belongTo(Scope* s) { return s == m_pScope; }
	void Copy(StackFrame* pFrom)
	{
		AutoLock lock(m_lock);
		for (int i = 0; i < m_varCnt; i++)
		{
			m_Values[i] = pFrom->m_Values[i];
		}
		m_retVal = pFrom->m_retVal;
	}
	inline int GetVarCount() { return m_varCnt; }
	inline bool SetVarCount(int cnt)
	{//can be called multiple times,
	//so need to check if m_Values is created
	//if created, copy data into new array
		AutoLock lock(m_lock);
		if (cnt == m_varCnt)
		{
			return true;
		}
		if (cnt > 0)
		{
			X::Value* newList = new X::Value[cnt];
			if (m_Values)
			{
				for (int i = 0; i < cnt && i < m_varCnt; i++)
				{
					newList[i] = m_Values[i];
				}
				delete[] m_Values;
			}
			m_Values = newList;
			m_varCnt = cnt;
		}
		return true;
	}
	inline void Set(int idx, X::Value& v)
	{
		AutoLock lock(m_lock);
		if (idx < 0 && idx >= m_varCnt)
		{
			std::cout << "StackFrame,Overflow,Var=" << m_varCnt << "Index="<<idx << std::endl;
		}
		m_Values[idx] = v;
#if XLANG_ENG_DBG
		if (v.IsObject())
		{
			ObjDbgSet(v.GetObj());
		}
#endif
	}
	inline void SetReturn(X::Value& v)
	{
		AutoLock lock(m_lock);
		m_retVal = v;
	}
	inline void Get(int idx, X::Value& v, X::LValue* lValue = nullptr)
	{
		AutoLock lock(m_lock);
		if (idx < 0 && idx >= m_varCnt)
		{
			std::cout << "StackFrame,Overflow,Var=" << m_varCnt << "Index="<<idx << std::endl;
		}
		X::Value& v0 = m_Values[idx];
		v = v0;
		if (lValue) *lValue = &v0;
	}
	inline X::Value& GetReturnValue()
	{
		AutoLock lock(m_lock);
		return m_retVal;
	}
};
}
}