#pragma once

#include "if-unknown.h"

// TODO: interface!!!
#include <string>
#include <vector>

namespace jukey::base
{

//==============================================================================
// Component object
//==============================================================================
struct ComObj
{
	std::string owner;
	std::string oid; // object ID
	std::string cid; // class ID
};

//==============================================================================
// Component factory
//==============================================================================
class IComFactory
{
public:
	//
	// Load components from apointed directory
	// @param com_dir the path of components
	//
	virtual bool Init(const std::string& com_dir) = 0;

	//
	// Create component with CID(Class ID)
	// @param cid component ID
	// @return component interface
	//
	virtual IUnknown* CreateComponent(const std::string& cid,
		const std::string& owner) = 0;

	//
	// Create component first, then query interface
	// @param cid component ID
	// @param iid interface ID
	// @return interface
	//
	virtual void* QueryInterface(const std::string& cid, const std::string& iid,
		const std::string& owner) = 0;

	//
	// Find all components by iid
	// @param iid  interface ID
	// @param cids cids of components that implemented the interface
	// @return found component count
	//
	virtual void GetComponents(const std::string& iid,
		std::vector<std::string>& cids) = 0;

	//
	// Add component object
	//
	virtual void AddComObj(const ComObj& co) = 0;

	//
	// Remove component object by object ID
	//
	virtual void RemoveComObj(const std::string& oid) = 0;

	//
	// Find componet objects by CID, empty cid means find all component objects
	//
	virtual std::vector<ComObj> GetComObjList(const std::string& cid) = 0;
};

} // namespace

// 
// Global entry to get component factory
// 
jukey::base::IComFactory* GetComFactory(void);



