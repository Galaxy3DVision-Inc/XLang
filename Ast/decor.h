#pragma once
#include "op.h"

namespace X
{
	class XlangRuntime;
	namespace AST
	{
		class Expression;
		class Decorator :
			virtual public UnaryOp
		{
			Expression* m_client = nullptr;
			bool GetParamList(XlangRuntime* rt, Expression* e, ARGS& params, KWARGS& kwParams);
			bool RunExp(XlangRuntime* rt,Value& v, LValue* lValue);
		public:
			Decorator() :
				Operator(),
				UnaryOp()
			{
				m_type = ObType::Decor;
			}
			Decorator(short op) :
				Operator(op),
				UnaryOp(op)
			{
				m_type = ObType::Decor;
			}
			~Decorator()
			{

			}
			inline Expression* Client() { return m_client; }
			inline void SetClient(Expression* e) { m_client = e; }
			virtual bool Exec(XlangRuntime* rt,ExecAction& action, XObj* pContext, Value& v, LValue* lValue = nullptr) override;
		};
	}
}