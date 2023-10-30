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

#ifndef ams_sop_objectmerge
#define ams_sop_objectmerge

#include <CH/CH_ExprLanguage.h>
#include <SOP/SOP_Node.h>


namespace ams {

class SOP_ObjectMerge : public SOP_Node
{
public:
  SOP_ObjectMerge(OP_Network* net, const char* name, OP_Operator* entry);

  ~SOP_ObjectMerge() override;

  bool updateParmsFlags() override;

  static OP_Node* myConstructor(OP_Network* net, const char* name, OP_Operator* entry);

  static PRM_Template myTemplateList[];
  static PRM_Template myObsoleteList[];

  // Because the object merge can reference an op chain which has a 
  // subnet with different D & R we must follow all our node's
  // d & r status
  int getDandROpsEqual() override;

  int updateDandROpsEqual(int = 1) override { return getDandROpsEqual(); }
  
  int RESOLVEMATS() { return evalInt("resolve_mats", 0, 0.0f); }
  void setRESOLVEMATS(int val) { setInt("resolve_mats", 0, 0.0f, val); }
  
  void HINTPATH(UT_String& str) { return evalString(str, "matnet_hint_path", 0, 0.0f); }
  void setHINTPATH(UT_String& str) { setString(str, CH_STRING_LITERAL, "matnet_hint_path", 0, 0.0f); }

  int ENABLEPATHATTRIB() { return evalInt("enable_pathattrib", 0, 0.0f); }
  void setENABLEPATHATTRIB(int val) { setInt("enable_pathattrib", 0, 0.0f, val); }

  void PATHATTRIBNAME(UT_String& str) { evalString(str, "pathattrib_name", 0, 0.0f); }
  void setPATHATTRIBNAME(UT_String& str) { setString(str, CH_StringMeaning::CH_STRING_LITERAL, "pathattrib_name", 0, 0.0f); }

  int RESOLVESUBNETS() { return evalInt("resolve_subnets", 0, 0.0f); }
  void setRESOLVESUBNETS(int val) { setInt("resolve_subnets", 0, 0.0f, val); }
  
  int ENABLENODEPATHATTRIB() { return evalInt("enable_nodepathattrib", 0, 0.0f); }
  void setENABLENODEPATHATTRIB(int val) { setInt("enable_nodepathattrib", 0, 0.0f, val); }

  void NODEPATHATTRIBNAME(UT_String& str) { evalString(str, "nodepathattrib_name", 0, 0.0f); }
  void setNODEPATHATTRIBNAME(UT_String& str) { setString(str, CH_StringMeaning::CH_STRING_LITERAL, "nodepathattrib_name", 0, 0.0f); }


  int NUMOBJ() { return evalInt("numobj", 0, 0.0f); }
  void setNUMOBJ(int num_obj) { setInt("numobj", 0, 0.0f, num_obj); }

  int ENABLEMERGE(int i) { return evalIntInst("enable#", &i, 0, 0.0f); }
  void setENABLEMERGE(int i, int val) { setIntInst(val, "enable#", &i, 0, 0.0f); }

  void SOPPATH(UT_String& str, int i, fpreal t) { evalStringInst("objpath#", &i, str, 0, t); }
  void setSOPPATH(UT_String& str, CH_StringMeaning meaning, int i, fpreal t) { setStringInst(str, meaning, "objpath#", &i, 0, t); }

  void XFORMPATH(UT_String& str, fpreal t) { evalString(str, "xformpath", 0, t); }

protected:
  OP_ERROR cookMySop(OP_Context& context) override;

  void updateHiddenParms();
  
  void resolveMaterials(GA_IndexMap::Marker primmarker, OP_Network* objptr, int objindex=1);
  
  std::vector<UT_String> parsePathString(UT_String& str);

  static UT_String resolvePath(const OP_Node& node, bool resolve_subnets=false);
};

}


#endif // ams_sop_objectmerge
