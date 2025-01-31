#pragma once
#include "exp.h"
#include "op.h"

namespace X
{
namespace AST
{
struct Indent
{
	int charPos = 0;
	int tab_cnt =0;
	int space_cnt =0;
	Indent(int cp,int tab,int space)
	{
		charPos = cp;
		tab_cnt = tab;
		space_cnt =space;
	}
	bool operator>=(const Indent& other)
	{
		return (charPos>=other.charPos)
			&& (tab_cnt >= other.tab_cnt) 
			&& (space_cnt >= other.space_cnt);
	}
	bool operator==(const Indent& other)
	{
		return (charPos == other.charPos)
			&& (tab_cnt == other.tab_cnt)
			&& (space_cnt == other.space_cnt);
	}
	bool operator<(const Indent& other)
	{
		return (charPos <= other.charPos) &&
			((tab_cnt <= other.tab_cnt && space_cnt < other.space_cnt)
				|| (tab_cnt < other.tab_cnt&& space_cnt <= other.space_cnt));
	}
};

class ActionOperator :
	virtual public Operator
{
public:
	ActionOperator() :Operator()
	{
		m_type = ObType::ActionOp;
	}
	ActionOperator(short op) :
		Operator(op)
	{
		m_type = ObType::ActionOp;
	}
	virtual bool OpWithOperands(
		std::stack<AST::Expression*>& operands,int LeftTokenIndex)
	{
		operands.push(this);
		return true;
	}
	virtual bool Exec(XlangRuntime* rt,ExecAction& action, XObj* pContext, 
		Value& v, LValue* lValue = nullptr)
	{
		return true;
	}
};
class Block:
	virtual public UnaryOp
{
protected:
	bool NoIndentCheck = false;//just for lambda block
	Indent IndentCount = { 0,-1,-1 };
	Indent ChildIndentCount = { 0,-1,-1 };
	bool m_bRunning = false;
	std::vector<Expression*> Body;
public:
	Block():Operator(), UnaryOp()
	{
		m_type = ObType::Block;
	}
	Block(short op) :
		Operator(op),
		UnaryOp(op)
	{
		m_type = ObType::Block;
	}
	~Block()
	{
		for (auto it : Body)
		{
			delete it;
		}
		Body.clear();
	}
	long long GetBodySize()
	{
		return Body.size();
	}
	virtual bool ToBytes(XlangRuntime* rt,XObj* pContext,X::XLangStream& stream) override
	{
		UnaryOp::ToBytes(rt,pContext,stream);
		stream << NoIndentCheck << IndentCount 
			<< ChildIndentCount << m_bRunning;
		stream << (int)Body.size();
		for (auto* exp : Body)
		{
			SaveToStream(rt, pContext,exp, stream);
		}
		return true;
	}
	virtual bool FromBytes(X::XLangStream& stream) override
	{
		UnaryOp::FromBytes(stream);
		stream >> NoIndentCheck >> IndentCount
			>> ChildIndentCount >> m_bRunning;
		int size = 0;
		stream >> size;
		for (int i = 0; i < size; i++)
		{
			auto* exp = BuildFromStream<Expression>(stream);
			Body.push_back(exp);
		}
		return true;
	}
	inline int GetStartLine()
	{
		if (Body.size() > 0)
		{
			return Body[0]->GetStartLine();
		}
		else
		{
			return -1;
		}
	}
	inline bool IsNoIndentCheck()
	{
		return NoIndentCheck;
	}
	inline void SetNoIndentCheck(bool b)
	{
		NoIndentCheck = b;
	}
	virtual void Add(Expression* item);
	inline Indent GetIndentCount() { return IndentCount; }
	inline Indent GetChildIndentCount() { return ChildIndentCount; }
	inline void SetIndentCount(Indent cnt) { IndentCount = cnt; }
	inline void SetChildIndentCount(Indent cnt) { ChildIndentCount = cnt; }
	bool RunLast(XRuntime* rt, XObj* pContext, Value& v, LValue* lValue = nullptr);
	bool RunFromLine(XRuntime* rt, XObj* pContext,long long lineNo,Value& v, LValue* lValue = nullptr);
	virtual bool Exec(XlangRuntime* rt, ExecAction& action,XObj* pContext, Value& v, LValue* lValue = nullptr) override;
};
class For :
	virtual public Block
{
public:
	For() :
		Operator(),
		Block()
	{
		m_type = ObType::For;
	}
	For(short op):
		Operator(op),
		Block(op)
	{
		m_type = ObType::For;
	}
	virtual bool Exec(XlangRuntime* rt,ExecAction& action,XObj* pContext, Value& v,LValue* lValue=nullptr) override;
};
class While :
	virtual public Block
{
public:
	While() :
		Operator(), 
		Block()
	{
		m_type = ObType::While;
	}
	While(short op) :
		Operator(op),
		Block(op)
	{
		m_type = ObType::While;
	}

	virtual bool Exec(XlangRuntime* rt,ExecAction& action,XObj* pContext, Value& v,LValue* lValue=nullptr) override;
};

class If :
	virtual public Block
{
	bool m_isIf = false;//if it is 'if', this flag is true, if it is 'elif', 'else' will be false
	If* m_next = nil;//elif  or else
public:
	If() :
		Operator(), Block()
	{
		m_type = ObType::If;
	}
	If(short op,bool needParam =true) :
		Operator(op),Block(op)
	{
		m_type = ObType::If;
		NeedParam = needParam;
	}
	void SetFlag(bool b)
	{
		m_isIf = b;
	}
	bool IsIf() { return m_isIf; }
	~If()
	{
		if (m_next) delete m_next;
	}
	virtual bool ToBytes(XlangRuntime* rt,XObj* pContext,X::XLangStream& stream) override
	{
		Block::ToBytes(rt,pContext,stream);
		SaveToStream(rt, pContext,m_next, stream);
		return true;
	}
	virtual bool FromBytes(X::XLangStream& stream) override
	{
		Block::FromBytes(stream);
		m_next = BuildFromStream<If>(stream);
		return true;
	}
	virtual bool EatMe(Expression* other) override;
	virtual bool Exec(XlangRuntime* rt,ExecAction& action,XObj* pContext, Value& v,LValue* lValue=nullptr) override;
};
}
}