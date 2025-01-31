#include "package.h"
#include "list.h"
#include "dict.h"
#include "prop.h"
#include "event.h"
#include "port.h"
#include "utility.h"
#include "Hosting.h"

namespace X
{
	namespace AST
	{
		std::string GetMemberType(PackageMemberType type)
		{
			std::string valType;
			switch (type)
			{
			case PackageMemberType::Func:
			case PackageMemberType::FuncEx:
				valType = "Function";
				break;
			case PackageMemberType::Class:
			case PackageMemberType::ClassInstance:
				valType = "Class";
				break;
			case PackageMemberType::Prop:
				valType = "Prop";
				break;
			case PackageMemberType::ObjectEvent:
				valType = "Event";
				break;
			default:
				break;
			}
			return valType;
		}
		X::Value Package::UpdateItemValue(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdList, int id_offset,
			std::string itemName, X::Value& val)
		{
			X::Value retVal = val;
			AutoLock autoLock(m_lock);
			if (id_offset < IdList.size())
			{
				auto& strId = IdList[id_offset++];
				X::Value item;
				int idx = Scope::AddOrGet(strId, true);
				if (idx >= 0)
				{
					GetIndexValue(idx, item);
					if (item.IsObject())
					{
						Object* pChildObj = dynamic_cast<Object*>(item.GetObj());
						if (pChildObj)
						{
							return pChildObj->UpdateItemValue(rt, this,
								IdList, id_offset, itemName, val);
						}
					}
				}
				//all elses, return value passed in
				return val;
			}
			int index = AddOrGet(itemName, true);
			if (index > 0)
			{
				X::Value valMember;
				GetIndexValue(index, valMember);
				//only change Prop's value
				if (valMember.IsObject() &&
					valMember.GetObj()->GetType() == X::ObjType::Prop)
				{
					auto* pPropObj = dynamic_cast<Data::PropObject*>(valMember.GetObj());
					if (pPropObj)
					{
						if (!pPropObj->SetPropValue(rt, this, val))
						{
							//Failed, return the original value
							pPropObj->GetPropValue(rt, this, retVal);
						}
					}
				}
			}
			return retVal;
		}
		void Package::UnloadAddedModules()
		{
			for (auto* pModule : m_loadedModules)
			{
				X::Hosting::I().Unload(pModule);
			}
			m_loadedModules.clear();
		}
		bool Package::RunCodeWithThisScope(const char* code)
		{
			auto* pTopModule = X::Hosting::I().LoadWithScope(this, code,strlen(code));
			m_loadedModules.push_back(pTopModule);
			//change StackFrame VarCount
			m_stackFrame->SetVarCount(GetVarNum());
			X::Value retVal;
			std::vector<X::Value> passInParams;
			bool bOK = X::Hosting::I().Run(pTopModule, retVal, passInParams);
			return bOK;
		}
		X::Data::List* Package::FlatPack(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdList, int id_offset,
			long long startIndex, long long count)
		{
			AutoLock autoLock(m_lock);
			if (id_offset < IdList.size())
			{
				auto& key = IdList[id_offset++];
				X::Value item;
				int idx = Scope::AddOrGet(key, true);
				if (idx >= 0)
				{
					GetIndexValue(idx, item);
					if (item.IsObject())
					{
						Object* pChildObj = dynamic_cast<Object*>(item.GetObj());
						if (pChildObj)
						{
							return pChildObj->FlatPack(rt, pContext, IdList, id_offset, startIndex, count);
						}
					}
				}
				//all elses, return an empty list
				X::Data::List* pOutList = new X::Data::List();
				pOutList->IncRef();
				return pOutList;
			}
			X::Data::List* pOutList = new X::Data::List();
			pOutList->IncRef();

			auto& memberInfo = GetMemberInfo();
			for (auto& it : memberInfo)
			{
				X::Data::Dict* dict = new X::Data::Dict();
				Data::Str* pStrName = new Data::Str(it.name);
				dict->Set("Name", X::Value(pStrName));
				//make object_ids
				std::string myId = it.name;
				auto valType = GetMemberType(it.type);
				int idx = AddOrGet(it.name, true);
				X::Value val;
				GetIndexValue(idx, val);
				if (val.IsObject() && val.GetObj()->GetType()
					== X::ObjType::Prop)
				{
					auto* pPropObj = dynamic_cast<X::Data::PropObject*>(val.GetObj());
					if (pPropObj)
					{
						//if this Prop is an Object,can't hold this object
						//because no chance to release it
						//so keep as PropObject which has Refcount hold by its owner
						//but for others, replace with this value
						X::Value v0;
						pPropObj->GetPropValue(rt, this, v0);
						if (!v0.IsObject() || v0.GetObj()->GetType() == X::ObjType::Str)
						{
							val = v0;
							valType = val.GetValueType();
						}
					}
				}
				auto objIds = CombinObjectIds(IdList, myId);
				dict->Set("Id", objIds);
				Data::Str* pStrType = new Data::Str(valType);
				dict->Set("Type", X::Value(pStrType));
				if (!val.IsObject() || (val.IsObject()
					&& dynamic_cast<Object*>(val.GetObj())->IsStr()))
				{
					dict->Set("Value", val);
				}
				else if (it.type == PackageMemberType::Func ||
					it.type == PackageMemberType::FuncEx)
				{
					dict->Set("Value", it.doc);
				}
				else if (val.IsObject())
				{
					X::Value objId((unsigned long long)val.GetObj());
					dict->Set("Value", objId);
					long long objSize = 0;
					if (val.GetObj()->GetType() == X::ObjType::Prop)
					{
						auto* pPropObj = dynamic_cast<X::Data::PropObject*>(val.GetObj());
						X::Value v0;
						pPropObj->GetPropValue(rt, this, v0);
						objSize = v0.Size();
					}
					else
					{
						objSize = val.Size();
					}
					X::Value valSize(objSize);
					dict->Set("Size", valSize);
				}
				X::Value valDict(dict);
				pOutList->Add(rt, valDict);
			}
			return pOutList;
		}
		X::Value PackageProxy::UpdateItemValue(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdList, int id_offset,
			std::string itemName, X::Value& val)
		{
			X::Value retVal = val;
			AutoLock autoLock(Object::m_lock);
			if (id_offset < IdList.size())
			{
				auto& strId = IdList[id_offset++];
				X::Value item;
				int idx = AddOrGet(strId, true);
				if (idx >= 0)
				{
					GetIndexValue(idx, item);
					if (item.IsObject())
					{
						Object* pChildObj = dynamic_cast<Object*>(item.GetObj());
						if (pChildObj)
						{
							return pChildObj->UpdateItemValue(rt, this,
								IdList, id_offset, itemName, val);
						}
					}
				}
				//all elses, return value passed in
				return val;
			}
			int index = AddOrGet(itemName, true);
			if (index > 0)
			{
				X::Value valMember;
				GetIndexValue(index, valMember);
				//only change Prop's value
				if (valMember.IsObject() &&
					valMember.GetObj()->GetType() == X::ObjType::Prop)
				{
					auto* pPropObj = dynamic_cast<Data::PropObject*>(valMember.GetObj());
					if (pPropObj)
					{
						if (!pPropObj->SetPropValue(rt, this, val))
						{
							//Failed, return the original value
							pPropObj->GetPropValue(rt, this, retVal);
						}
					}
				}
			}
			return retVal;
		}
		X::Data::List* PackageProxy::FlatPack(XlangRuntime* rt, XObj* pContext,
			std::vector<std::string>& IdList, int id_offset,
			long long startIndex, long long count)
		{
			AutoLock autoLock(Object::m_lock);
			if (id_offset < IdList.size())
			{
				auto& strId = IdList[id_offset++];
				X::Value item;
				int idx = AddOrGet(strId, true);
				if (idx >= 0)
				{
					GetIndexValue(idx, item);
					if (item.IsObject())
					{
						Object* pChildObj = dynamic_cast<Object*>(item.GetObj());
						if (pChildObj)
						{
							return pChildObj->FlatPack(rt, this, IdList, id_offset, startIndex, count);
						}
					}
				}
				//all elses, return an empty list
				X::Data::List* pOutList = new X::Data::List();
				pOutList->IncRef();
				return pOutList;
			}
			X::Data::List* pOutList = new X::Data::List();
			pOutList->IncRef();
			if (m_pPackage == nullptr)
			{
				return pOutList;
			}
			auto& memberInfo = m_pPackage->GetMemberInfo();
			for (auto& it : memberInfo)
			{
				X::Data::Dict* dict = new X::Data::Dict();
				Data::Str* pStrName = new Data::Str(it.name);
				dict->Set("Name", X::Value(pStrName));
				//make object_ids
				std::string myId = it.name;
				int idx = AddOrGet(it.name, true);
				auto valType = GetMemberType(it.type);
				X::Value val;
				GetIndexValue(idx, val);
				if (val.IsObject())
				{
					if (val.GetObj()->GetType() == X::ObjType::Prop)
					{
						auto* pPropObj = dynamic_cast<X::Data::PropObject*>(val.GetObj());
						if (pPropObj)
						{
							//if this Prop is an Object,can't hold this object
							//because no chance to release it
							//so keep as PropObject which has Refcount hold by its owner
							//but for others, replace with this value
							X::Value v0;
							pPropObj->GetPropValue(rt, this, v0);
							if (!v0.IsObject() || v0.GetObj()->GetType() == X::ObjType::Str)
							{
								val = v0;
								valType = val.GetValueType();
							}
						}
					}
					else if (val.GetObj()->GetType() == X::ObjType::ObjectEvent)
					{
						auto* pEvtObj = dynamic_cast<X::ObjectEvent*>(val.GetObj());
						if (pEvtObj)
						{
							auto str_abi = pEvtObj->ToString();
							std::string strVal = str_abi;
							X::g_pXHost->ReleaseString(str_abi);
							val = strVal;
						}
					}
				}
				auto objIds = CombinObjectIds(IdList, myId);
				dict->Set("Id", objIds);

				Data::Str* pStrType = new Data::Str(valType);
				dict->Set("Type", X::Value(pStrType));
				if (!val.IsObject() || (val.IsObject()
					&& dynamic_cast<Object*>(val.GetObj())->IsStr()))
				{
					dict->Set("Value", val);
				}
				else if (it.type == PackageMemberType::Func ||
					it.type == PackageMemberType::FuncEx)
				{
					dict->Set("Value", it.doc);
				}
				else if (val.IsObject())
				{
					X::Value objId((unsigned long long)val.GetObj());
					dict->Set("Value", objId);
					long long objSize = 0;
					if (val.GetObj()->GetType() == X::ObjType::Prop)
					{
						auto* pPropObj = dynamic_cast<X::Data::PropObject*>(val.GetObj());
						X::Value v0;
						pPropObj->GetPropValue(rt, this, v0);
						objSize = v0.Size();
					}
					else
					{
						objSize = val.Size();
					}
					X::Value valSize(objSize);
					dict->Set("Size", valSize);
				}
				X::Value valDict(dict);
				pOutList->Add(rt, valDict);
			}

			return pOutList;
		}
	}
}