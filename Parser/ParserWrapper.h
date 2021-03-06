/*
 * Copyright 2017 MapD Technologies, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file    ParserWrapper.h
 * @author  michael
 * @brief   Classes used to wrap parser calls for calcite redirection
 *
 * Copyright (c) 2016 MapD Technologies, Inc.  All rights reserved.
 **/
#ifndef PARSER_WRAPPER_H_
#define PARSER_WRAPPER_H_

#include <string>
#include <vector>
#include <boost/regex.hpp>

#include "Shared/ConfigResolve.h"

class ParserWrapper {
 public:
  // HACK:  This needs to go away as calcite takes over parsing
  enum class DMLType : int { Insert = 0, Delete, Update, Upsert, NotDML };

  ParserWrapper(std::string query_string);
  std::string process(std::string user,
                      std::string passwd,
                      std::string catalog,
                      std::string sql_string,
                      const bool legacy_syntax);
  virtual ~ParserWrapper();
  bool is_select_explain = false;
  bool is_select_calcite_explain = false;
  bool is_other_explain = false;
  bool is_ddl = false;
  bool is_update_dml = false;
  bool is_copy = false;
  bool is_copy_to = false;
  std::string actual_query;

  DMLType getDMLType() const { return dml_type; }

 private:
  DMLType dml_type = DMLType::NotDML;

  static const std::vector<std::string> ddl_cmd;
  static const std::vector<std::string> update_dml_cmd;
  static const std::string explain_str;
  static const std::string calcite_explain_str;
};


enum class CalciteDMLPathSelection : int {
    Unsupported = 0,
    OnlyUpdates = 1,
    OnlyDeletes = 2,
    UpdatesAndDeletes = 3
};

inline CalciteDMLPathSelection yield_dml_path_selector() {
   int selector = 0;
   if( std::is_same< CalciteDeletePathSelector, PreprocessorTrue >::value ) selector |= 0x02;
   if( std::is_same< CalciteUpdatePathSelector, PreprocessorTrue >::value ) selector |= 0x01;
   return static_cast< CalciteDMLPathSelection >( selector );
}

inline bool is_calcite_permissable_dml( ParserWrapper const& pw ) {
    switch( yield_dml_path_selector() ) {
        case CalciteDMLPathSelection::OnlyUpdates:
            return !pw.is_update_dml || (pw.getDMLType() == ParserWrapper::DMLType::Update);
        case CalciteDMLPathSelection::OnlyDeletes:
            return !pw.is_update_dml || (pw.getDMLType() == ParserWrapper::DMLType::Delete);
        case CalciteDMLPathSelection::UpdatesAndDeletes:
            return !pw.is_update_dml || (pw.getDMLType() == ParserWrapper::DMLType::Delete) || (pw.getDMLType() == ParserWrapper::DMLType::Update);
        case CalciteDMLPathSelection::Unsupported:
        default:
            return false;
    }
}

inline bool is_calcite_path_permissable(ParserWrapper const& pw) {
  return (!pw.is_ddl && is_calcite_permissable_dml(pw) && !pw.is_other_explain);
}

#endif  // PARSERWRAPPER_H_
