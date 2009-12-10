/*  This file is part of mhmake.
 *
 *  Copyright (C) 2001-2009 Marc Haesen
 *
 *  Mhmake is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mhmake is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Mhmake.  If not, see <http://www.gnu.org/licenses/>.
*/

/* $Rev$ */

#ifndef __MHMAKEFILE_PARSER__
#define __MHMAKEFILE_PARSER__

#include "fileinfo.h"
#include "util.h"

class rule;

class mhmakelexer;

struct TOKENVALUE
{
  string theString;
  int ival;
};

class mhmakefileparser;
typedef string (mhmakefileparser::*function_f)(const string &) const;

struct funcdef
{
  const char *szFuncName;
  function_f   pFunc;
};

class fileinfoarray : public refbase,public vector<refptr<fileinfo> >
{
};

typedef set< refptr<fileinfo> > deps_t;
class mhmakefileparser : public refbase
{

private:
  mhmakelexer *m_ptheLexer;
  int          m_yyloc;
  refptr<fileinfo> m_RuleThatIsBuild;
  vector<string> m_ToBeIncludeAfterBuild;
  vector<string> m_MakefilesToLoad;
  refptr<fileinfo> m_AutoDepFileLoaded;
  int m_InExpandExpression;
  mh_time_t m_Date;
  uint32 m_EnvMd5_32;   /* Cached Md5_32 value of the userd environment variables */
#ifdef _DEBUG
  int m_ImplicitSearch;
#endif
  map<string,string> m_CommandCache;

protected:
  map<string,string> m_Variables;
  map<string,string> m_CommandLineVars;
  TOKENVALUE       m_theTokenValue;
  refptr<fileinfo> m_MakeDir;
  refptr<rule>     m_pCurrentRule;
  refptr<fileinfoarray> m_pCurrentItems;
  refptr<fileinfoarray> m_pCurrentDeps;
  refptr<fileinfo> m_FirstTarget;
  fileinfoarray m_IncludedMakefiles;
  refptr< fileinfoarray > m_pIncludeDirs;
  string m_IncludeDirs;
  map< refptr<fileinfo>, set< refptr<fileinfo> > > m_AutoDeps;
  set< const fileinfo* , less_fileinfo > m_Targets; // List of targets that are build by this makefile
  bool m_DoubleColonRule;
  bool m_AutoDepsDirty;
  bool m_ForceAutoDepRescan;
  set<string> m_SkipHeaders;        // Headers to skip
  vector<string> m_PercentHeaders;  // Percent specification of headers to skip
  bool m_SkipHeadersInitialized;    // true when previous 2 variables are initialized

  bool m_RebuildAll; /* true when to rebuild all targets of this makefile */

  vector<string>     m_Exports;         // Array of variables to export.
  map<string,string> m_SavedExports;    // Original values of the environment are saved here.
  set<string>        m_UsedEnvVars;     // Array containing a list of variables that are taken from the environment (This is used for rebuild checking)
  static const mhmakefileparser *m_spCurEnv; // Identifies which makefiles exports are currently set

  static mh_time_t m_sBuildTime;

public:
#ifdef _DEBUG
  deque< refptr<fileinfo> > m_TargetStack;   /* Used to detect circular dependencies */
#endif

  mhmakefileparser(const map<string,string> &CommandLineVars)
    :  m_CommandLineVars(CommandLineVars)
      ,m_InExpandExpression(0)
      ,m_AutoDepsDirty(false)
      ,m_ForceAutoDepRescan(false)
      ,m_SkipHeadersInitialized(false)
      ,m_RebuildAll(false)
      ,m_EnvMd5_32(0)
      #ifdef _DEBUG
      ,m_ImplicitSearch(false)
      #endif
  {
    if (!m_FunctionsInitialised) InitFuncs();
    SetVariable("MAKE_VERSION",MHMAKEVER);
    SetVariable(OBJEXTVAR,OBJEXT);
    SetVariable(EXEEXTVAR,EXEEXT);
  }

  bool CompareEnv() const;
  uint32 CreateEnvMd5_32() const;
  string GetFromEnv(const string &Var,bool Cache=true) const;
  void CreateUSED_ENVVARS();

  void SaveEnv();
  void RestoreEnv() const;
  void CheckEnv(void);

  void SetRebuildAll()
  {
    m_RebuildAll=true;
    m_AutoDepsDirty=true;  /* This is to be sure that all new calculated md5 strings are saved. */
  }

  void SetVariable(string Var,string Val)
  {
    m_Variables[Var]=Val;
  }
  void EnableAutoDepRescan(void)
  {
    m_ForceAutoDepRescan=true;
    m_AutoDepsDirty=true;
  }
  bool ForceAutoDepRescan(void)
  {
    return m_ForceAutoDepRescan;
  }
  void SetRuleThatIsBuild(const refptr<fileinfo> &Target)
  {
    m_RuleThatIsBuild=Target;
  }
  void ClearRuleThatIsBuild()
  {
    m_RuleThatIsBuild=NULL;
  }
  void UpdateAutomaticDependencies(const refptr<fileinfo> &Target);
  const refptr<fileinfoarray> GetIncludeDirs() const;
  void GetAutoDeps(const refptr<fileinfo> &FirstDep,set< refptr<fileinfo> > &Autodeps);
  void SaveAutoDepsFile();
  void LoadAutoDepsFile(refptr<fileinfo> &DepFile);
  bool SkipHeaderFile(const string &FileName);
  void InitEnv() const;

  virtual ~mhmakefileparser()
  {
    SaveAutoDepsFile();
  }
  virtual int yylex(void);
  virtual void yyerror(const char *m);
  virtual int yyparse()=0;

  int ParseFile(const refptr<fileinfo> &FileInfo,bool SetMakeDir=false);

  /* Functions to handle variables */
  bool IsDefined(const string &Var) const;
  bool IsEqual(const string &EqualExpr) const;
  string ExpandExpression(const string &Expr) const;
  string ExpandMacro(const string &Expr) const;
  string ExpandVar(const string &Var) const;

  void PrintVariables(bool Expand=false) const;

  /* Functions for macro functions */
  static funcdef m_FunctionsDef[];
  static map<string,function_f> m_Functions;
  static bool m_FunctionsInitialised;
  static void InitFuncs(void);
  string f_filter(const string &) const;
  string f_call(const string &) const;
  string f_if(const string &) const;
  string f_findstring(const string &) const;
  string f_firstword(const string &) const;
  string f_wildcard(const string &) const;
  string f_subst(const string &) const;
  string f_patsubst(const string &) const;
  string f_concat(const string &) const;
  string f_basename(const string & Arg) const;
  string f_notdir(const string & Arg) const;
  string f_dir(const string & Arg) const;
  string f_shell(const string & Arg) const;
  string f_relpath(const string & Arg) const;
  string f_toupper(const string & Arg) const;
  string f_tolower(const string & Arg) const;
  string f_exist(const string & Arg) const;
  string f_filesindirs(const string & Arg) const;
  string f_fullname(const string & Arg) const;
  string f_addprefix(const string & Arg) const;
  string f_addsuffix(const string & Arg) const;
  string f_filterout(const string & Arg) const;
  string f_word(const string & Arg) const;
  string f_words(const string & Arg) const;
  string f_strip(const string & Arg) const;
  string f_which(const string & Arg) const;

  const refptr<fileinfo> GetFirstTarget() const
  {
    return m_FirstTarget;
  }

  const refptr<fileinfo> GetMakeDir() const
  {
    return m_MakeDir;
  }

  mh_time_t GetDate(void) const
  {
    return m_Date;
  }

  void UpdateDate(mh_time_t Date)
  {
    if (Date.IsNewer(m_Date))
      m_Date=Date;
  }

  void AddTarget(const fileinfo* pTarget)
  {
    m_Targets.insert(pTarget);
  }
  mh_time_t BuildTarget(const refptr<fileinfo> &Target, bool bCheckTargetDir=true);  /* returns the date of the target after build, especially important for phony rules, since this will be the youngest date of all dependencies */
  void BuildDependencies(const refptr<rule> &pRule, const refptr<fileinfo> &Target, mh_time_t TargetDate, mh_time_t &YoungestDate, bool &MakeTarget);

  void BuildIncludedMakefiles();

  void AddIncludedMakefile(refptr<fileinfo> &MakeInfo)
  {
    UpdateDate(MakeInfo->GetDate());
    m_IncludedMakefiles.push_back(MakeInfo);
  }
  fileinfoarray &GetIncludedMakefiles()
  {
    return m_IncludedMakefiles;
  }

  void IncludeAfterBuild(const string &IncludeFile)
  {
    m_ToBeIncludeAfterBuild.push_back(IncludeFile);
  }
  void ParseBuildedIncludeFiles();

  void AddMakefileToMakefilesToLoad(const string &Makefile)
  {
    m_MakefilesToLoad.push_back(Makefile);
  }
  vector<string>& GetMakefilesToLoad()
  {
    return m_MakefilesToLoad;
  }
  void AddRule();

  bool ExecuteCommand(string Command,string *pOutput=NULL);
  string GetFullCommand(string Command);
  void CreatePythonExe(const string &FullCommand);

  static void InitBuildTime();
};

void SplitToItems(const string &String,vector< refptr<fileinfo> > &Items,refptr<fileinfo> Dir=curdir::GetCurDir());

#endif

