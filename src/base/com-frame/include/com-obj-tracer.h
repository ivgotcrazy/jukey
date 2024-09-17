#pragma once

#include <sstream>

#include "com-factory.h"

namespace jukey::base
{

//==============================================================================
//
//==============================================================================
class ComObjTracer
{
public:
	ComObjTracer(IComFactory* factory, const std::string& cid,
		const std::string& owner)
		: m_tracer_factory(factory)
	{
		std::stringstream ss;
		ss << this;
		m_oid = ss.str();

		ComObj co;
		co.cid = cid;
		co.owner = owner;
		co.oid = m_oid;

		m_tracer_factory->AddComObj(co);
	}

	~ComObjTracer()
	{
		std::stringstream ss;
		ss << this;

		m_tracer_factory->RemoveComObj(m_oid);
	}

private:
	std::string m_oid;
	IComFactory* m_tracer_factory = nullptr;
};

}