/*
 * Copyright 2021 Anthony Sorge II
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
 * documentation files (the “Software”), to deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to 
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the 
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE 
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR 
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "sop_objectmerge.h"
#include "ams_utils.h"
#include <SYS/SYS_Version.h>
#include <GU/GU_Detail.h>
#include <OP/OP_Director.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_SpareData.h>
#include <PRM/PRM_Parm.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_DirUtil.h>
#include <VOP/VOP_Node.h>
#include <UT/UT_WorkArgs.h>
#include <map>
#include <regex>

//#if SYS_VERSION_MAJOR_INT >= 19
//#endif

using std::map;

// Define the new sop operator...
void newSopOperator(OP_OperatorTable * table) {
  table->addOperator(new OP_Operator(
    "ams::objectmerge::1.0",
    "AMS Object Merge",
    ams::SOP_ObjectMerge::myConstructor,
    ams::SOP_ObjectMerge::myTemplateList,
    0, 0, 0, OP_FLAG_GENERATOR));
}


namespace ams {

PRM_Template PRM_TemplateWithHelp(PRM_Type thetype = PRM_LIST_TERMINATOR,
int thevectorsize = 1, PRM_Name* thenameptr = 0, PRM_Default* thedefaults = 0,
const char* thehelptext = 0) {
  return PRM_Template(thetype, thevectorsize, thenameptr, thedefaults,
  0, 0, 0, 0, 0,
  thehelptext);
}

// NOTE: Houdini will crash if a map is used with const char* keys. std::string keys must be used.
static std::map<std::string, PRM_Name> parmNames = {
  {"numobj",                PRM_Name("numobj", "Number of Objects")},
  {"objpath",               PRM_Name("objpath#", "Object #")},
  {"xformpath",             PRM_Name("xformpath", "Transform Object")},
  {"enable",                PRM_Name("enable#", "Enable Merge #")},
  {"resolve_mats",          PRM_Name("resolve_mats", "Resolve Material Paths")},
  {"matnet_hint_path",      PRM_Name("matnet_hint_path", "Matnet Hint Path")},
  {"enable_pathattrib",     PRM_Name("enable_pathattrib", "Enable Path")},
  {"pathattrib_name",       PRM_Name("pathattrib_name", "Path Attribute")},
  {"resolve_subnets",       PRM_Name("resolve_subnets", "Resolve Subnets")},
  {"enable_nodepathattrib", PRM_Name("enable_nodepathattrib", "Enable Node Path")},
  {"nodepathattrib_name",   PRM_Name("nodepathattrib_name", "Node Path Attribute")},
  {"None",                  PRM_Name(0)}
};

static auto pathattrib_name_prmdefault = PRM_Default(0.0f, "path", CH_STRING_LITERAL);
static auto nodepathattrib_name_prmdefault = PRM_Default(0.0f, "nodepath", CH_STRING_LITERAL);

static PRM_Template theObjectTemplates[] = {
  PRM_Template(PRM_TOGGLE, 1, &parmNames["enable"], PRMoneDefaults),
  PRM_Template(PRM_STRING, PRM_TYPE_DYNAMIC_PATH, 1, &parmNames["objpath"], 0, 0, 0, 0, &PRM_SpareData::sopPath),
  PRM_Template()
};


PRM_Template SOP_ObjectMerge
::myTemplateList[] = {
  PRM_TemplateWithHelp(PRM_TOGGLE, 1, &parmNames["resolve_mats"], PRMoneDefaults,
                       "If a geometry's shop_materialpath contains an invalid material, attempt to find the material."),
  PRM_TemplateWithHelp(PRM_STRING, 1, &parmNames["matnet_hint_path"], PRMoneDefaults,
                       "A path leading to a matnet node to assist in finding materials."),
  PRM_TemplateWithHelp(PRM_TOGGLE, 1, &parmNames["enable_pathattrib"], PRMoneDefaults,
                       "Creates a path attribute which describes the transform hierarchy of the object being merged. This is not to be confused with a node path."),
  PRM_TemplateWithHelp(PRM_STRING, 1, &parmNames["pathattrib_name"], &pathattrib_name_prmdefault,
                       "The name of the path attribute to create."),
  PRM_TemplateWithHelp(PRM_TOGGLE, 1, &parmNames["resolve_subnets"], PRMoneDefaults,
                       "When this option is enabled, node paths will be resolved for subnets."),
  PRM_TemplateWithHelp(PRM_TOGGLE, 1, &parmNames["enable_nodepathattrib"], PRMoneDefaults,
                       "Creates a node path attribute. This is a path to the object node being merged."),
  PRM_TemplateWithHelp(PRM_STRING, 1, &parmNames["nodepathattrib_name"], &nodepathattrib_name_prmdefault,
                       "The name of the node path attribute to create."),
  PRM_Template(PRM_STRING, PRM_TYPE_DYNAMIC_PATH, 1, &parmNames["xformpath"],
               0, 0, 0, 0, &PRM_SpareData::objPath),
  PRM_Template(PRM_MULTITYPE_LIST, theObjectTemplates, 2, &parmNames["numobj"], PRMoneDefaults),
  PRM_Template()
};


OP_Node* SOP_ObjectMerge
::myConstructor(OP_Network* net, const char* name, OP_Operator* entry) {
  return new SOP_ObjectMerge(net, name, entry);
}


SOP_ObjectMerge
::SOP_ObjectMerge(OP_Network* net, const char* name, OP_Operator* entry)
  : SOP_Node(net, name, entry) {
  // NOTE: GEO_Detail::copy(), when GEO_COPY_ONCE is used, will copy all
  //       data IDs from the source, and GEO_Detail::transform() will
  //       bump data IDs of any transformed attributes, as well as the
  //       primitive list data ID if there are any transforming primitives
  //       that were transformed.
  mySopFlags.setManagesDataIDs(true);
}


SOP_ObjectMerge::~SOP_ObjectMerge() {}


bool SOP_ObjectMerge
::updateParmsFlags() {
  bool changed = false;
  int n = NUMOBJ();
  for (int i = 1; i <= n; i++) {
    changed |= enableParmInst(parmNames["objpath"].getToken(), &i, ENABLEMERGE(i));
  }
  return changed;
}


int SOP_ObjectMerge
::getDandROpsEqual() {
  // don't do anything if we're locked
  if (flags().getHardLocked())
    return 1;
  int numobj = NUMOBJ();
  // Determine if any of our SOPs are evil.
  for (int objindex = 1; objindex <= numobj; objindex++) {
    if (!ENABLEMERGE(objindex)) // Ignore disabled ones.
      continue;
    UT_String objname;
    SOPPATH(objname, objindex, 0.0f);
    OP_Network * objptr = (OP_Network*) findNode((const char*) objname);
    if (!objptr)
      continue;
    // Self-referential nodes are assumed to have equal DandR ops,
    // as it doesn't matter as they will be ignored and flagged
    // as errors during cook anyways.
    if (objptr == this) {
      continue;
    }
    if (!objptr->getDandROpsEqual()) {
      return 0;
    }
  }
  // None of our interests was unequal, thus we are equal!
  return 1;
}


std::vector<UT_String> SOP_ObjectMerge
::parsePathString(UT_String& str) {
  std::vector<UT_String> ret;
  UT_WorkArgs paths;
  // split spaces between paths.
  str.tokenize(paths, ' ');
  for (auto* path : paths) {
    if (findNode(path)) {
      ret.emplace_back(path);
    }
  }
  return ret;
}


void formatDirPath(UT_String& path) {
  if (path.length() > 0 && path[path.length()-1] != '/') {
    path.append('/');
  }
}

// TODO: figure out why Resolve Mats modifies shop_materialpath with strange material mappings.
OP_ERROR SOP_ObjectMerge
::cookMySop(OP_Context& context) {
  fpreal t = context.getTime();
  
  #pragma region Get Params
  updateHiddenParms();
  // Get our xform object, if any.
  UT_String objname;
  XFORMPATH(objname, t);
  OP_Network * xformobjptr = (OP_Network*) findNode(objname);
  if (xformobjptr && xformobjptr->getOpTypeID() == SOP_OPTYPE_ID) {
    // The user pointed to the SOP.  We silently promote it to the
    // containing object.  This allows the intuitive "." to be used
    // for the path to transform relative to our own op (rather than
    // having to track up an arbitrary number of paths)
    xformobjptr = xformobjptr->getCreator();
  }
  // We must explicitly cast down as OBJ_Node * is unknown here.
  // We also do the cast here instead of in findNode as we want
  // to allow people to reference SOPs.
  xformobjptr = (OP_Network*) CAST_OBJNODE(xformobjptr);
  if (!xformobjptr && objname.isstring()) {
    // We didn't get an xform object, but they had something typed,
    // badmerge.
    addError(SOP_BAD_SOP_MERGED, objname);
  }
  int numobj = NUMOBJ();

  //
  //
  // PATH ATTRIB
  bool enablepathattrib = ENABLEPATHATTRIB();
  UT_String pathattribname;
  GA_Attribute* pathattrib = nullptr;
  if (enablepathattrib) {
    PATHATTRIBNAME(pathattribname);
    if (pathattribname.length() > 0) {
      pathattrib = gdp->createStringAttribute(GA_ATTRIB_PRIMITIVE, GA_SCOPE_PUBLIC, pathattribname);
    } else {
      addWarning(SOP_ErrorCodes::SOP_ATTRIBUTE_INVALID);
      enablepathattrib = false;
    }
  }
  
  //
  //
  // NODE PATH ATTRIB
  bool enable_nodepathattrib = ENABLENODEPATHATTRIB();
  UT_String nodepathattribname;
  GA_Attribute* nodepathattrib = nullptr;
  if (enable_nodepathattrib) {
    NODEPATHATTRIBNAME(nodepathattribname);
    if (nodepathattribname.length() > 0) {
      nodepathattrib = gdp->createStringAttribute(GA_ATTRIB_PRIMITIVE, GA_SCOPE_PUBLIC, nodepathattribname);
    } else {
      addWarning(SOP_ErrorCodes::SOP_ATTRIBUTE_INVALID);
      enable_nodepathattrib = false;
    }
  }
  
  // RESOLVE MATS
  bool resolve_mats = RESOLVEMATS();;
  UT_String hintpath;
  HINTPATH(hintpath);
  formatDirPath(hintpath);
  #pragma endregion Get Params

  #pragma region Main Loop
  // MAIN LOOP
  // Peek whether we are a render cook or not.
  int cookrender = getCreator()->isCookingRender();
  bool copiedfirst = false;
  bool copiedlast = false;
  // this is an iterated value that increments only when geometry is merged.
  int cooked_objindx = 1;
  for (int objindex = 1; objindex <= numobj; objindex++) {
    if (!ENABLEMERGE(objindex))             // Ignore disabled ones.
      continue;
    UT_String soppathstr;
    SOPPATH(soppathstr, objindex, t);
    if (!soppathstr.isstring())
      continue;                           // Blank means ignore.
    for (auto soppath : parsePathString(soppathstr)) {
      SOP_Node* sopptr = getSOPNode(soppath, 1); // We want extra inputs.
      if (sopptr == this) {
        // Self-reference.  Special brand of evil.
        addWarning(SOP_ERR_SELFMERGE);
        continue;
      }
      if (!sopptr) {
        // Illegal merge.  Just warn so we don't abort everything.
        addWarning(SOP_BAD_SOP_MERGED, soppath);
        continue;
      }
      // Get the creator, which is our objptr.
      OP_Network * objptr = sopptr->getCreator();
      // Change over so any subnet evaluation will properly track...
      int savecookrender = objptr->isCookingRender();
      objptr->setCookingRender(cookrender);
      // Actually cook...
      const GU_Detail* cookedgdp = sopptr->getCookedGeo(context);
      // Restore the cooking render state.
      objptr->setCookingRender(savecookrender);
      if (!cookedgdp) {
        // Something went wrong with the cooking. Warn the hapless user.
        addWarning(SOP_BAD_SOP_MERGED, soppath);
        continue;
      }
      // Now add the extra inputs...
      addExtraInput(objptr, OP_INTEREST_DATA);
      // The sop extra inputs were set by the getSOPNode
      bool firstmerge = !copiedfirst;
      // Choose the best copy method we can
      GEO_CopyMethod copymethod = GEO_COPY_ADD;
      if (!copiedfirst) {
        copymethod = GEO_COPY_START;
        copiedfirst = true;
        if (objindex == numobj) {
          copymethod = GEO_COPY_ONCE;
          copiedlast = true;
        }
      } else if (objindex == numobj) {
        copymethod = GEO_COPY_END;
        copiedlast = true;
      }
      // Mark where the new prims and points start
      GA_IndexMap::Marker pointmarker(gdp->getPointMap());
      GA_IndexMap::Marker primmarker(gdp->getPrimitiveMap());

      // Don't copy internal groups!
      // Accumulation of internal groups may ensue.
      gdp->copy(*cookedgdp, copymethod, true, false, GA_DATA_ID_CLONE);
      // Apply the transform.
      if (xformobjptr) {
        // GEO_Detail::transform supports double-precision,
        // so we might as well use double-precision transforms.
        UT_Matrix4D xform, xform2;
        if (!objptr->getWorldTransform(xform, context))
          addTransformError(*objptr, "world");
        if (!xformobjptr->getIWorldTransform(xform2, context))
          addTransformError(*xformobjptr, "inverse world");
        xform *= xform2;
        if (firstmerge) {
          // The first object we merge we can just do a full gdp transform
          // rather than building the subgroup.
          gdp->transform(xform);
        } else {
          gdp->transform(xform, primmarker.getRange(), pointmarker.getRange(), false);
        }
      }

      if (enablepathattrib) {
        gdp->addAttribute(pathattribname, nullptr, nullptr, "string", GA_ATTRIB_PRIMITIVE);
        // Create a path from the hierarchy of transforms. This is not to be confused with a node "directory" path.
        UT_String path(resolvePath(*objptr, RESOLVESUBNETS()));
        GA_Iterator it;
        if (cooked_objindx == 1) {
          // for loop doesn't evaluate primmarker on first run. Get global prim iterator instead.
          it = gdp->getPrimitiveRange().begin();
        } else {
          it = primmarker.getRange().begin();
        }
        GA_RWHandleS handle(gdp, GA_ATTRIB_PRIMITIVE, pathattribname);
        if (handle.isValid())
          for (; !it.atEnd(); ++it) {
            auto offset = *it;
            handle->setString(offset, path);
          }
      }

      if (enable_nodepathattrib) {
        gdp->addAttribute(nodepathattribname, nullptr, nullptr, "string", GA_ATTRIB_PRIMITIVE);
        GA_Iterator it;
        if (cooked_objindx == 1) {
          // for loop doesn't evaluate primmarker on first run. Get global prim iterator instead.
          it = gdp->getPrimitiveRange().begin();
        } else {
          it = primmarker.getRange().begin();
        }
        GA_RWHandleS handle(gdp, GA_ATTRIB_PRIMITIVE, nodepathattribname);
        if (handle.isValid())
          for (; !it.atEnd(); ++it) {
            auto offset = *it;
            handle->setString(offset, objptr->getFullPath());
          }
      }

      if (resolve_mats) {
        resolveMaterials(primmarker, objptr, cooked_objindx);
      }
    }
    cooked_objindx++;
  }
  #pragma endregion Main Loop
  
  if (xformobjptr) {
    addExtraInput(xformobjptr, OP_INTEREST_DATA);
  }
  // Finish & clean up the copy procedure, if not already done.
  if (!copiedfirst) {
    gdp->clearAndDestroy();
  } else if (!copiedlast) {
    GU_Detail blank_gdp;
    gdp->copy(blank_gdp, GEO_COPY_END, true, false, GA_DATA_ID_CLONE);
  }
  // Set the node selection for this primitive. This will highlight all
  // the primitives of the node, but only if the highlight flag for this node
  // is on and the node is selected.
  if (error() < UT_ERROR_ABORT)
    select(GA_GROUP_PRIMITIVE);
  return error();
}


void SOP_ObjectMerge
::updateHiddenParms() {
  bool enablePathattrib = ENABLEPATHATTRIB();
  bool resolveMats = RESOLVEMATS();
  bool enableNodePathattrib = ENABLENODEPATHATTRIB();
  
  this->getParm(parmNames["matnet_hint_path"].getToken()).setVisibleState(resolveMats);
  this->getParm(parmNames["pathattrib_name"].getToken()).setVisibleState(enablePathattrib);
  this->getParm(parmNames["resolve_subnets"].getToken()).setVisibleState(enablePathattrib);
  this->getParm(parmNames["nodepathattrib_name"].getToken()).setVisibleState(enableNodePathattrib);
}


UT_String SOP_ObjectMerge
::resolvePath(const OP_Node& node, bool resolve_subnets) {
  UT_String path;
  auto dir = OPgetDirector();
  node.getFullPath(path);
  auto ancestors = ams::getInputAncestors(node);

  UT_String parentHier = "";
  for (auto* n : ancestors) {
    auto name = n->getName();
    name.prepend("/");
    parentHier.prepend(name);
  }

  if (resolve_subnets) {
    const OP_Node* rootAtLevel;
    if (ancestors.empty()) {
      rootAtLevel = &node;
    } else {
      rootAtLevel = ancestors[ancestors.size() - 1];
    }
    auto type = rootAtLevel->getOpType();
    auto* subnParent = rootAtLevel->getParent();

    if (subnParent && subnParent->getOpType() == type) {
      auto upstream = resolvePath(*subnParent, resolve_subnets);
      parentHier.prepend(upstream);
    }
  }

  parentHier.append("/");
  parentHier.append(node.getName());
  return parentHier;
}


void SOP_ObjectMerge
::resolveMaterials(GA_IndexMap::Marker primmarker, OP_Network* objptr, int objindex) {
  UT_String hintpath;
  HINTPATH(hintpath);
  auto matnet = findNode(hintpath);
  if (matnet)
    hintpath = UT_String(matnet->getFullPath());
  formatDirPath(hintpath);
  
  // check if material leads to a valid node.
  // first we will check at the sop level. If resolution fails, check the object material path.
  GA_Iterator it;
  if (objindex == 1) {
    // for loop doesn't evaluate primmarker on first run. Get global prim iterator instead.
    it = gdp->getPrimitiveRange().begin();
  } else {
    it = primmarker.getRange().begin();
  }
  auto handle = GA_RWHandleS(gdp, GA_ATTRIB_PRIMITIVE, "shop_materialpath");
  if (!handle.isValid()) {
    auto attr = gdp->addStringTuple(GA_ATTRIB_PRIMITIVE, "shop_materialpath", 1);
    attr->fill(gdp->getPrimitiveRange(), *it);
    handle = GA_RWHandleS(gdp, GA_ATTRIB_PRIMITIVE, "shop_materialpath");
  }
  if (handle.isValid()) {
    UT_String matpath;
    VOP_Node *matnode;
    auto matnodes = map<UT_String, VOP_Node*>(); // this map guards against redundant searches.
    
    UT_String objshoppath{};
    objptr->getParm("shop_materialpath").getValue(0.0f, objshoppath, 0, true, 0);
    auto* objmatnode = objptr->findNode(objshoppath);
    if (objmatnode)
      objshoppath = objmatnode->getFullPath();
    
    for (; !it.atEnd(); ++it) {
      auto offset = *it;
      auto shpath = handle->getString(offset, 0);
      matpath = shpath;
      auto mapname = UT_String(shpath);
      if (matnodes.count(mapname) > 0) {
        // mapname has previously been searched. Skip search, even if value is null.
        if (matnodes[mapname]) {
          matnode = matnodes[mapname];
          handle->setString(offset, matnode->getFullPath());
        }
        continue;
      }
      if (matpath.length() == 0 && objshoppath.length() > 0) {
        // uninitialized material path. Use object material path.
        matpath = objshoppath;
        if (matpath.length() == 0) continue;
      }
      matnode = OPgetDirector()->findVOPNode(matpath);
      if (matnode) {
        // matnode has been discovered. Add to map and continue.
        matnodes[mapname] = matnode;
        handle->setString(offset, matnode->getFullPath());
        continue;
      }

      matpath.prepend(hintpath);
      matnode = OPgetDirector()->findVOPNode(matpath);

      if (matnode) {
        // matnode has been discovered. Add to map and continue.
        matnodes[mapname] = matnode;
        handle->setString(offset, matnode->getFullPath());
        continue;
      }
      matnodes[mapname] = nullptr; // signal that we have searched for mapname but nothing could be found.
    }
  }
}

}