#pragma once
#include "object.h"

namespace X {
	namespace Data {
		class XClassObject :
			public virtual Object
		{
		protected:
			AST::XClass* m_obj = nullptr;
			AST::StackFrame* m_stackFrame = nullptr;
		public:
			XClassObject(AST::XClass* p)
			{
				m_t = ObjType::XClassObject;
				m_obj = p;
				m_stackFrame = new AST::StackFrame((AST::Scope*)this);
				m_stackFrame->SetVarCount(p->GetVarNum());
				auto* pClassStack = p->GetClassStack();
				if (pClassStack)
				{
					m_stackFrame->Copy(pClassStack);
				}
			}
			~XClassObject()
			{
				if (m_stackFrame)
				{
					delete m_stackFrame;
				}
			}
			virtual long long Size() override
			{
				return m_obj ? m_obj->GetVarNum() : 0;
			}
			inline virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override
			{
				Object::GetBaseScopes(bases);
				bases.push_back(GetClassObj());
			}
			virtual List* FlatPack(XlangRuntime* rt, XObj* pContext,
				std::vector<std::string>& IdList, int id_offset,
				long long startIndex, long long count) override;
			virtual bool CalcCallables(XlangRuntime* rt, XObj* pContext,
				std::vector<AST::Scope*>& callables) override
			{
				return m_obj ? m_obj->CalcCallables(rt, pContext, callables) : false;
			}
			virtual const char* ToString(bool WithFormat = false)
			{
				char v[1000];
				snprintf(v, sizeof(v), "Class:%s@0x%llx",
					m_obj->GetNameString().c_str(), (unsigned long long)this);
				std::string retStr(v);
				return GetABIString(retStr);
			}
			inline AST::StackFrame* GetStack()
			{
				return m_stackFrame;
			}
			inline AST::XClass* GetClassObj() { return m_obj; }
			virtual bool Call(XRuntime* rt, XObj* pContext, ARGS& params,
				KWARGS& kwParams,
				X::Value& retValue)
			{
				return m_obj->Call((XlangRuntime*)rt, this,
					params, kwParams, retValue);
			}
		};
	}
}