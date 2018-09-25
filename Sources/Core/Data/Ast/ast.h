/**
 * @file Core/Data/Ast/ast.h
 * Contains the definitions and include statements of all types in the Data::Ast
 * namespace.
 *
 * @copyright Copyright (C) 2018 Sarmad Khalid Abdullah
 *
 * @license This file is released under Alusus Public License, Version 1.0.
 * For details on usage and copying conditions read the full license in the
 * accompanying license file or at <http://alusus.net/alusus_license_1_0>.
 */
//==============================================================================

#ifndef CORE_DATA_AST_AST_H
#define CORE_DATA_AST_AST_H

namespace Core::Data::Ast
{

/**
 * @defgroup core_data_ast AST Classes
 * @ingroup core_data
 * @brief Abstract Syntax Tree classes.
 */


//==============================================================================
// Macros

#define _IMPLEMENT_AST_CLONABLE(type, copyStmt) \
  public: virtual SharedPtr<TiObject> clone() const \
  { \
    SharedPtr<type> newObj = std::make_shared<type>(); \
    for (Word i = 0; i < this->getMemberCount(); ++i) { \
      newObj->setMember(this->getMemberKey(i).c_str(), this->getMember(i)); \
    } \
    for (Int i = 0; i < this->getElementCount(); ++i) { \
      copyStmt; \
    } \
    return newObj; \
  }

#define IMPLEMENT_AST_CLONABLE(type) \
  _IMPLEMENT_AST_CLONABLE(type, newObj->setElement(i, this->getElement(i)))
#define IMPLEMENT_AST_LIST_CLONABLE(type) \
  _IMPLEMENT_AST_CLONABLE(type, newObj->addElement(this->getElement(i)))
#define IMPLEMENT_AST_MAP_CLONABLE(type) \
  _IMPLEMENT_AST_CLONABLE(type, newObj->setElement(this->getElementKey(i).c_str(), this->getElement(i)))

#define _PRINT_AST_TYPE_NAME1(type) stream << S(#type)
#define _PRINT_AST_TYPE_NAME2(type, extra) stream << S(#type " ") extra
#define _PRINT_AST_TYPE_NAME(...) \
  SELECT_MACRO(__VA_ARGS__, _, _, _, _, _, _, _, _, _PRINT_AST_TYPE_NAME2, _PRINT_AST_TYPE_NAME1)(__VA_ARGS__)

#define IMPLEMENT_AST_MAP_PRINTABLE(...) \
  public: virtual void print(OutStream &stream, Int indents=0) const \
  { \
    _PRINT_AST_TYPE_NAME(__VA_ARGS__); \
    Word id = this->getProdId(); \
    if (id != UNKNOWN_ID) { \
      stream << S(" [") << ID_GENERATOR->getDesc(id) << S("]"); \
    } \
    for (Word i = 0; i < this->getElementCount(); ++i) { \
      stream << S("\n"); \
      printIndents(stream, indents+1); \
      stream << this->getElementKey(i).c_str() << S(": "); \
      Core::Data::dumpData(stream, this->getElement(i), indents+1); \
    } \
  }

#define IMPLEMENT_AST_LIST_PRINTABLE(...) \
  public: virtual void print(OutStream &stream, Int indents=0) const \
  { \
    _PRINT_AST_TYPE_NAME(__VA_ARGS__); \
    Word id = this->getProdId(); \
    if (id != UNKNOWN_ID) { \
      stream << S(" [") << ID_GENERATOR->getDesc(id) << S("]"); \
    } \
    for (Word i = 0; i < this->getElementCount(); ++i) { \
      stream << S("\n"); \
      printIndents(stream, indents+1); \
      Core::Data::dumpData(stream, this->getElement(i), indents+1); \
    } \
  }


//==============================================================================
// Data Types

ti_s_enum(BracketType, TiInt, "Core.Data.Ast", "Core", "alusus.net", ROUND, SQUARE);


//==============================================================================
// Forward Defnitions

class Definition;


//==============================================================================
// Global Functions

SharedPtr<SourceLocation> const& findSourceLocation(TiObject *obj);

Bool mergeDefinition(Definition *def, ListContaining<TiObject> *target, Notices::Store *noticeStore);
Bool addPossiblyMergeableElement(TiObject *src, ListContaining<TiObject> *target, Notices::Store *noticeStore);
Bool addPossiblyMergeableElements(
  Containing<TiObject> *src, ListContaining<TiObject> *target, Notices::Store *noticeStore
);

} // namespace

#include "MetaHaving.h"
#include "Mergeable.h"

#include "List.h"
#include "Map.h"

#include "Token.h"
#include "Route.h"

#include "Scope.h"
#include "ParamPass.h"
#include "InfixOperator.h"
#include "OutfixOperator.h"
#include "Text.h"
#include "Bracket.h"
#include "Definition.h"
#include "GenericCommand.h"
#include "Alias.h"

namespace Core::Data::Ast
{

DEFINE_AST_OUTFIX_OPERATOR(PrefixOperator);
DEFINE_AST_OUTFIX_OPERATOR(PostfixOperator);

DEFINE_AST_INFIX_OPERATOR(AssignmentOperator);
DEFINE_AST_INFIX_OPERATOR(ComparisonOperator);
DEFINE_AST_INFIX_OPERATOR(AdditionOperator);
DEFINE_AST_INFIX_OPERATOR(MultiplicationOperator);
DEFINE_AST_INFIX_OPERATOR(BitwiseOperator);
DEFINE_AST_INFIX_OPERATOR(LogOperator);
DEFINE_AST_INFIX_OPERATOR(LinkOperator);
DEFINE_AST_INFIX_OPERATOR(ConditionalOperator);

DEFINE_AST_TEXT_ELEMENT(Identifier);
DEFINE_AST_TEXT_ELEMENT(IntegerLiteral);
DEFINE_AST_TEXT_ELEMENT(FloatLiteral);
DEFINE_AST_TEXT_ELEMENT(CharLiteral);
DEFINE_AST_TEXT_ELEMENT(StringLiteral);

} // namespace

#endif