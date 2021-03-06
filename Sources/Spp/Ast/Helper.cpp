/**
 * @file Spp/Ast/Helper.cpp
 * Contains the implementation of class Spp::Ast::Helper.
 *
 * @copyright Copyright (C) 2020 Sarmad Khalid Abdullah
 *
 * @license This file is released under Alusus Public License, Version 1.0.
 * For details on usage and copying conditions read the full license in the
 * accompanying license file or at <https://alusus.org/license.html>.
 */
//==============================================================================

#include "spp.h"

namespace Spp { namespace Ast
{

//==============================================================================
// Initialization Functions

void Helper::initBindingCaches()
{
  Basic::initBindingCaches(this, {
    &this->isAstReference,
    &this->lookupCalleeInScope,
    &this->lookupCalleeOnObject,
    &this->lookupCustomCaster,
    &this->lookupReferenceTarget,
    &this->traceType,
    &this->isVoid,
    &this->isImplicitlyCastableTo,
    &this->isExplicitlyCastableTo,
    &this->matchTargetType,
    &this->isReferenceTypeFor,
    &this->getReferenceTypeFor,
    &this->getPointerTypeFor,
    &this->getArrayTypeFor,
    &this->getValueTypeFor,
    &this->getNullType,
    &this->getBoolType,
    &this->getCharType,
    &this->getCharArrayType,
    &this->getArchIntType,
    &this->getIntType,
    &this->getWord64Type,
    &this->getWordType,
    &this->getFloatType,
    &this->getVoidType,
    &this->getTiObjectType,
    &this->resolveNodePath,
    &this->getFunctionName,
    &this->getNeededIntSize,
    &this->getNeededWordSize,
    &this->getVariableDomain,
    &this->getFunctionDomain,
    &this->validateUseStatement
  });
}


void Helper::initBindings()
{
  this->isAstReference = &Helper::_isAstReference;
  this->lookupCalleeInScope = &Helper::_lookupCalleeInScope;
  this->lookupCalleeOnObject = &Helper::_lookupCalleeOnObject;
  this->lookupCustomCaster = &Helper::_lookupCustomCaster;
  this->lookupReferenceTarget = &Helper::_lookupReferenceTarget;
  this->traceType = &Helper::_traceType;
  this->isVoid = &Helper::_isVoid;
  this->isImplicitlyCastableTo = &Helper::_isImplicitlyCastableTo;
  this->isExplicitlyCastableTo = &Helper::_isExplicitlyCastableTo;
  this->matchTargetType = &Helper::_matchTargetType;
  this->isReferenceTypeFor = &Helper::_isReferenceTypeFor;
  this->getReferenceTypeFor = &Helper::_getReferenceTypeFor;
  this->getPointerTypeFor = &Helper::_getPointerTypeFor;
  this->getArrayTypeFor = &Helper::_getArrayTypeFor;
  this->getValueTypeFor = &Helper::_getValueTypeFor;
  this->getNullType = &Helper::_getNullType;
  this->getBoolType = &Helper::_getBoolType;
  this->getCharType = &Helper::_getCharType;
  this->getCharArrayType = &Helper::_getCharArrayType;
  this->getArchIntType = &Helper::_getArchIntType;
  this->getIntType = &Helper::_getIntType;
  this->getWord64Type = &Helper::_getWord64Type;
  this->getWordType = &Helper::_getWordType;
  this->getFloatType = &Helper::_getFloatType;
  this->getVoidType = &Helper::_getVoidType;
  this->getTiObjectType = &Helper::_getTiObjectType;
  this->resolveNodePath = &Helper::_resolveNodePath;
  this->getFunctionName = &Helper::_getFunctionName;
  this->getNeededIntSize = &Helper::_getNeededIntSize;
  this->getNeededWordSize = &Helper::_getNeededWordSize;
  this->getVariableDomain = &Helper::_getVariableDomain;
  this->getFunctionDomain = &Helper::_getFunctionDomain;
  this->validateUseStatement = &Helper::_validateUseStatement;
}


//==============================================================================
// Main Functions

Bool Helper::_isAstReference(TiObject *self, TiObject *obj)
{
  return
    obj->isDerivedFrom<Core::Data::Ast::ParamPass>() ||
    obj->isDerivedFrom<Core::Data::Ast::LinkOperator>() ||
    obj->isDerivedFrom<Core::Data::Ast::Identifier>() ||
    obj->isDerivedFrom<Spp::Ast::ThisTypeRef>() ||
    obj->isDerivedFrom<Spp::Ast::TypeOp>();
}


Bool Helper::lookupCalleeInScopeByName(
  Char const *name, SharedPtr<Core::Data::SourceLocation> const &sl, Core::Data::Node *astNode, Bool searchOwners,
  TiObject *thisType, Containing<TiObject> *types, ExecutionContext const *ec, CalleeLookupResult &result
) {
  Core::Data::Ast::Identifier identifier;
  identifier.setValue(name);
  identifier.setSourceLocation(sl);
  return this->lookupCalleeInScope(&identifier, astNode, searchOwners, thisType, types, ec, result);
}


Bool Helper::_lookupCalleeInScope(
  TiObject *self, TiObject *ref, Core::Data::Node *astNode, Bool searchOwners,
  TiObject *thisType, Containing<TiObject> *types, ExecutionContext const *ec, CalleeLookupResult &result
) {
  PREPARE_SELF(helper, Helper);

  Word currentStackSize = result.stack.getCount();
  Core::Data::Node *prevNode = 0;
  Bool retVal = false;
  helper->getSeeker()->foreach(ref, astNode,
    [=, &result, &prevNode, &retVal]
      (TiObject *obj, Core::Notices::Notice *ntc)->Core::Data::Seeker::Verb
      {
        if (ntc != 0) {
          if (result.notice == 0 && result.stack.getCount() == currentStackSize) result.notice = getSharedPtr(ntc);
          return Core::Data::Seeker::Verb::MOVE;
        }
        if (obj == 0) return Core::Data::Seeker::Verb::MOVE;

        // Unbox if we have a box.
        auto box = ti_cast<TioWeakBox>(obj);
        if (box != 0) obj = box->get().get();

        if (
          result.stack.getCount() > 0 && result.stack.getElement(result.stack.getCount() - 1) == obj &&
          result.thisIndex < 0
        ) {
          return Core::Data::Seeker::Verb::MOVE;
        }

        // There is no need to continue searching if we already have a match and we are jumping to another level.
        auto node = ti_cast<Core::Data::Node>(obj);
        if (node != 0 && prevNode != 0) {
          // If we encounter the same node again it means we moved up and encountered a use statement that took us back
          // into the same module. In this case we can stop looking since we have moved up and we already have a match.
          if (node == prevNode) return Core::Data::Seeker::Verb::STOP;

          // If it's not the same node, then let's check the owners of the two nodes.
          Core::Data::Node *owner = Core::Data::findOwner<Core::Data::Ast::Scope>(node);
          // It's possible that the previous match was for a function arg, so we have to account for that since a
          // function arg would have the same owner scope as a function sitting at the same module.
          Core::Data::Node *prevOwner = Core::Data::findOwner<Spp::Ast::FunctionType>(prevNode);
          if (prevOwner == 0) prevOwner = Core::Data::findOwner<Core::Data::Ast::Scope>(prevNode);
          if (owner != prevOwner) return Core::Data::Seeker::Verb::STOP;
        }

        if (helper->lookupCalleeOnObject(obj, thisType, types, ec, currentStackSize, result)) {
          retVal = true;
          prevNode = node;
        } else {
          if (result.notice != 0 && result.notice->isDerivedFrom<Spp::Notices::MultipleCalleeMatchNotice>()) {
            retVal = false;
            if (result.matchStatus == TypeMatchStatus::EXACT) {
              // There is no need to continue searching if we found multiple exact matches.
              return Core::Data::Seeker::Verb::STOP;
            }
          }
        }

        return Core::Data::Seeker::Verb::MOVE;
      },
      searchOwners ? 0 : SeekerExtension::Flags::SKIP_OWNERS_AND_USES
  );

  // Try the injections if we don't have an exact match yet.
  if (result.matchStatus < TypeMatchStatus::EXACT) {
    auto dataType = ti_cast<DataType>(astNode);
    if (dataType != 0) {
      auto block = dataType->getBody().get();
      if (block != 0) {
        Word newStackSize = result.stack.getCount();
        for (Int i = 0; i < block->getInjectionCount(); ++i) {
          auto def = ti_cast<Core::Data::Ast::Definition>(block->getInjection(i));
          if (def == 0 || def->getTarget() == 0) continue;
          if (!helper->isAstReference(def->getTarget().get())) continue;
          Bool isMember = helper->getVariableDomain(def) == DefinitionDomain::OBJECT;
          if ((isMember && thisType == 0) || (!isMember && thisType != 0)) continue;
          auto objType = helper->traceType(def->getTarget().get());
          if (objType == 0) continue;
          auto refType = helper->getReferenceTypeFor(objType, Ast::ReferenceMode::IMPLICIT);
          objType = helper->tryGetDeepReferenceContentType(objType);
          result.stack.add(def->getTarget().get());
          Bool noBindDef = helper->isNoBindDef(def);
          if (helper->lookupCalleeInScope(ref, objType, false, noBindDef ? thisType : refType, types, ec, result)) {
            retVal = true;
            if (noBindDef) result.thisIndex = currentStackSize - 1;
            if (result.matchStatus == TypeMatchStatus::EXACT) break;
          } else {
            while (result.stack.getCount() > newStackSize) result.stack.remove(newStackSize);
          }
        }
      }
    }
  }

  // Did we have a matched callee?
  if (!retVal) {
    // Remove any extra items we added to the stack.
    while (result.stack.getCount() > currentStackSize) result.stack.remove(currentStackSize);

    // If we have no notice it means the symbol was not found in the first place.
    if (result.notice == 0 && result.matchStatus == TypeMatchStatus::NONE) {
      result.notice = newSrdObj<Spp::Notices::UnknownSymbolNotice>();
    }

    if (result.notice != 0) result.notice->setSourceLocation(Core::Data::Ast::findSourceLocation(ref));
  }

  return retVal;
}


Bool Helper::_lookupCalleeOnObject(
  TiObject *self, TiObject *obj, TiObject *thisType, Containing<TiObject> *types, ExecutionContext const *ec,
  Word currentStackSize, CalleeLookupResult &result
) {
  PREPARE_SELF(helper, Helper);

  ContainerExtender<TiObject, 1, 0> extTypes(types);
  extTypes.setPreItem(0, thisType);

  if (obj != 0 && obj->isDerivedFrom<Ast::Function>()) {
    // Match functions.
    auto f = static_cast<Ast::Function*>(obj);
    TypeMatchStatus ms;

    Bool useThis = !f->getType()->isBindDisabled() && thisType != 0;

    ms = f->getType()->matchCall(useThis ? &extTypes : types, helper, ec);
    if (ms < TypeMatchStatus::CUSTOM_CASTER && result.matchStatus == TypeMatchStatus::NONE) {
      result.notice = newSrdObj<Spp::Notices::ArgsMismatchNotice>();
      return false;
    } else if (ms >= TypeMatchStatus::CUSTOM_CASTER && ms > result.matchStatus) {
      result.matchStatus = ms;
      result.notice.reset();
      result.thisIndex = currentStackSize - 1;
      if (result.stack.getCount() == currentStackSize) result.stack.add(f);
      else result.stack.set(currentStackSize, f);
      result.type = f->getType().get();
      return true;
    } else if (ms >= TypeMatchStatus::CUSTOM_CASTER && ms == result.matchStatus) {
      result.notice = newSrdObj<Spp::Notices::MultipleCalleeMatchNotice>();
      return false;
    } else {
      return false;
    }
  } else if (obj != 0 && obj->isDerivedFrom<ArrayType>()) {
    if (
      types != 0 && types->getElementCount() == 1 &&
      helper->isImplicitlyCastableTo(types->getElement(0), helper->getArchIntType(), ec)
    ) {
      if (result.matchStatus < TypeMatchStatus::EXACT) {
        result.matchStatus = TypeMatchStatus::EXACT;
        result.type = static_cast<ArrayType*>(obj);
        result.notice.reset();
        return true;
      } else {
        result.notice = newSrdObj<Spp::Notices::MultipleCalleeMatchNotice>();
        return false;
      }
    } else {
      if (result.matchStatus == TypeMatchStatus::NONE) {
        result.notice = newSrdObj<Spp::Notices::ArgsMismatchNotice>();
      }
      return false;
    }
  } else if (obj != 0 && obj->isDerivedFrom<Type>()) {
    auto funcType = helper->tryGetPointerContentType<FunctionType>(obj);
    if (funcType != 0) {
      // We have a function pointer.
      Bool useThis = !funcType->isBindDisabled() && thisType != 0;
      auto ms = funcType->matchCall(useThis ? &extTypes : types, helper, ec);
      if (ms < TypeMatchStatus::CUSTOM_CASTER && result.matchStatus == TypeMatchStatus::NONE) {
        result.notice = newSrdObj<Spp::Notices::ArgsMismatchNotice>();
        return false;
      } else if (ms >= TypeMatchStatus::CUSTOM_CASTER && ms > result.matchStatus) {
        result.matchStatus = ms;
        result.notice.reset();
        result.thisIndex = useThis ? currentStackSize - 1 : -2;
        result.type = funcType;
        return true;
      } else if (ms >= TypeMatchStatus::CUSTOM_CASTER && ms == result.matchStatus) {
        result.notice = newSrdObj<Spp::Notices::MultipleCalleeMatchNotice>();
        return false;
      } else {
        return false;
      }
    } else {
      auto objType = static_cast<Type*>(obj);
      auto objRefType = helper->getReferenceTypeFor(obj, Ast::ReferenceMode::IMPLICIT);
      Function *customCaster;
      if (result.stack.getCount() == currentStackSize && thisType == 0) {
        // This is a constructor.
        static Core::Data::Ast::Identifier ref({ {S("value"), TiStr(S("~init"))} });
        if (helper->lookupCalleeInScope(
          &ref, static_cast<Core::Data::Node*>(obj), false, objRefType, types, ec, result
        )) {
          // A constructor match was found.
          result.stack.set(currentStackSize, objType);
          result.thisIndex = -2;
          result.type = objType;
          return true;
        } else if (
          types->getElementCount() == 0 && (objType->getInitializationMethod(helper, ec) & TypeInitMethod::USER) == 0
        ) {
          // No user-defined constructor is defined and no params is provided, so we can skip the matching.
          result.matchStatus = TypeMatchStatus::EXACT;
          result.notice.reset();
          result.stack.add(objType);
          result.thisIndex = -2;
          result.type = objType;
          return true;
        } else if (
          (objType->getInitializationMethod(helper, ec) & TypeInitMethod::USER) == 0 &&
          types->getElementCount() == 1 &&
          helper->matchTargetType(types->getElement(0), objType, ec, customCaster) >= TypeMatchStatus::CUSTOM_CASTER
        ) {
          // A type with no custom init but can be initialized from the given arg.
          result.matchStatus = TypeMatchStatus::EXACT;
          result.notice.reset();
          result.stack.add(objType);
          result.thisIndex = -2;
          result.type = objType;
          return true;
        } else {
          // If the ref symbol we provided here was not found we'll provide a better error message to the user since
          // the user didn't manually provide this symbol.
          if (result.notice != 0 && result.notice->isA<Spp::Notices::UnknownSymbolNotice>()) {
            result.notice = newSrdObj<Spp::Notices::InvalidOperationNotice>();
          }
          return false;
        }
      } else {
        // Lookup custom parens operators.
        static Core::Data::Ast::Identifier ref({ {S("value"), TiStr(S("()"))} });
        if (helper->lookupCalleeInScope(
          &ref, static_cast<Core::Data::Node*>(obj), false, objRefType, types, ec, result
        )) {
          return true;
        } else {
          // If the ref symbol we provided here was not found we'll provide a better error message to the user since
          // the user didn't manually provide this symbol.
          if (result.notice != 0 && result.notice->isA<Spp::Notices::UnknownSymbolNotice>()) {
            result.notice = newSrdObj<Spp::Notices::InvalidOperationNotice>();
          }
          return false;
        }
      }
    }
  } else if (obj != 0 && helper->isAstReference(obj)) {
    // Match variables
    auto objType = ti_cast<Type>(helper->traceType(obj));
    if (objType == 0) return false;
    objType = helper->tryGetDeepReferenceContentType(objType);
    if (objType == 0) {
      if (result.matchStatus == TypeMatchStatus::NONE) {
        result.notice = newSrdObj<Spp::Notices::InvalidTypeNotice>(Core::Data::Ast::findSourceLocation(obj));
      }
      return false;
    } else {
      TiObject *prevVal = 0;
      if (result.stack.getCount() == currentStackSize) result.stack.add(obj);
      else {
        prevVal = result.stack.get(currentStackSize);
        result.stack.set(currentStackSize, obj);
      }
      if (helper->lookupCalleeOnObject(objType, thisType, types, ec, currentStackSize, result)) {
        return true;
      } else {
        if (result.stack.getCount() > currentStackSize) result.stack.remove(currentStackSize);
        else result.stack.set(currentStackSize, prevVal);
        return false;
      }
    }
  } else if (obj != 0 && obj->isDerivedFrom<Template>()) {
    // TODO: Look-up template functions as well.
    auto objType = helper->traceType(obj);
    if (objType == 0) {
      if (result.matchStatus == TypeMatchStatus::NONE) {
        result.notice = newSrdObj<Spp::Notices::InvalidOperationNotice>();
      }
      return false;
    }
    return helper->lookupCalleeOnObject(objType, thisType, types, ec, currentStackSize, result);
  } else {
    // Invalid
    if (result.matchStatus == TypeMatchStatus::NONE) {
      result.notice = newSrdObj<Spp::Notices::InvalidOperationNotice>();
    }
    return false;
  }
}


TypeMatchStatus Helper::_lookupCustomCaster(
  TiObject *self, Type *srcType, Type *targetType, ExecutionContext const *ec, Function *&caster
) {
  PREPARE_SELF(helper, Helper);

  auto srcRefType = ti_cast<ReferenceType>(srcType);
  if (srcRefType != 0) {
    auto srcContentType = ti_cast<DataType>(srcRefType->getContentType(helper));
    if (srcContentType != 0 && srcContentType->getBody() != 0) {
      PlainList<TiObject> argTypes({ srcRefType });
      static Core::Data::Ast::Identifier ref({{S("value"), TiStr(S("~cast"))}});
      TypeMatchStatus retMatch;
      helper->getSeeker()->foreach(&ref, srcContentType->getBody().get(),
        [=, &retMatch, &caster, &argTypes] (TiObject *obj, Core::Notices::Notice *ntc)->Core::Data::Seeker::Verb
        {
          auto func = ti_cast<Function>(obj);
          if (func != 0) {
            auto funcType = func->getType().get();
            auto match = funcType->matchCall(&argTypes, helper, ec);
            if (match == TypeMatchStatus::EXACT) {
              auto retType = funcType->traceRetType(helper);
              auto rmatch = retType->matchTargetType(targetType, helper, ec);
              if ((rmatch>=TypeMatchStatus::IMPLICIT_CAST || rmatch==TypeMatchStatus::AGGREGATION) && rmatch>retMatch) {
                retMatch = rmatch;
                caster = func;
                if (rmatch == TypeMatchStatus::EXACT) return Core::Data::Seeker::Verb::STOP;
              }
            }
          }
          return Core::Data::Seeker::Verb::MOVE;
        },
        SeekerExtension::Flags::SKIP_OWNERS_AND_USES
      );
      return retMatch;
    }
  }
  return TypeMatchStatus::NONE;
}


Bool Helper::_lookupReferenceTarget(
  TiObject *self, TiObject *astNode, Core::Data::Ast::Identifier *ref, Bool searchOwners, PlainList<TiObject> &stack
) {
  PREPARE_SELF(helper, Helper);

  Word currentStackSize = stack.getCount();
  Bool symbolFound = false;
  helper->getSeeker()->foreach(ref, astNode,
    [=, &symbolFound, &stack] (TiObject *obj, Core::Notices::Notice*)->Core::Data::Seeker::Verb
    {
      symbolFound = true;
      stack.add(obj);
      return Core::Data::Seeker::Verb::STOP;
    },
    searchOwners ? 0 : SeekerExtension::Flags::SKIP_OWNERS_AND_USES
  );

  if (!symbolFound) {
    Block *block = 0;
    auto dataType = ti_cast<DataType>(astNode);
    if (dataType != 0) block = dataType->getBody().get();
    else block = Core::Data::findOwner<Block>(ti_cast<Core::Data::Node>(astNode));

    if (block != 0) {
      for (Int i = 0; i < block->getInjectionCount(); ++i) {
        auto def = ti_cast<Core::Data::Ast::Definition>(block->getInjection(i));
        if (def == 0 || def->getTarget() == 0) continue;
        if (!helper->isAstReference(def->getTarget().get())) continue;
        auto objType = ti_cast<Type>(helper->getSeeker()->tryGet(def->getTarget().get(), block));
        if (objType == 0) continue;
        objType = helper->tryGetDeepReferenceContentType(objType);
        stack.add(def->getTarget().get());
        if (helper->lookupReferenceTarget(objType, ref, false, stack)) {
          symbolFound = true;
          break;
        } else {
          while (stack.getCount() > currentStackSize) stack.remove(currentStackSize);
        }
      }
    }
  }

  return symbolFound;
}


Type* Helper::_traceType(TiObject *self, TiObject *ref)
{
  PREPARE_SELF(helper, Helper);

  SharedPtr<Core::Notices::Notice> notice;
  TiObject *foundObj = 0;
  Spp::Ast::Type *type = 0;
  if (ref->isDerivedFrom<Spp::Ast::TypeOp>()) {
    auto operand = static_cast<Spp::Ast::TypeOp*>(ref)->getOperand().ti_cast_get<Core::Data::Node>();
    auto typeRef = helper->getSeeker()->tryGet(operand, operand->getOwner());
    if (typeRef != 0) {
      type = helper->traceType(typeRef);
    }
  }
  else if (ref->isDerivedFrom<Spp::Ast::Type>()) {
    type = static_cast<Spp::Ast::Type*>(ref);
  } else if (ref->isDerivedFrom<Spp::Ast::Template>()) {
    auto tpl = static_cast<Spp::Ast::Template*>(ref);
    Core::Data::Ast::List list;
    TioSharedPtr result;
    if (tpl->matchInstance(&list, helper, result)) {
      type = result.ti_cast_get<Spp::Ast::Type>();
    } else {
      if (result != 0 && result->isDerivedFrom<Core::Notices::Notice>()) notice = result.s_cast<Core::Notices::Notice>();
    }
  } else if (helper->isAstReference(ref)) {
    auto typeRef = static_cast<Core::Data::Node*>(ref);
    auto owner = typeRef->getOwner();
    auto paramPass = ti_cast<Core::Data::Ast::ParamPass>(ref);
    if (paramPass != 0 && paramPass->getType() == Core::Data::Ast::BracketType::ROUND) {
      typeRef = paramPass->getOperand().ti_cast_get<Core::Data::Node>();
    }
    if (typeRef == 0) {
      throw EXCEPTION(GenericException, S("Invalid type reference."));
    }
    helper->getSeeker()->foreach(typeRef, owner,
      [=, &foundObj, &type, &notice](TiObject *obj, Core::Notices::Notice *ntc)->Core::Data::Seeker::Verb
      {
        if (ntc != 0) {
          notice = getSharedPtr(ntc);
          return Core::Data::Seeker::Verb::MOVE;
        }

        // Unbox if we have a box.
        auto box = ti_cast<TioWeakBox>(obj);
        if (box != 0) foundObj = box->get().get();
        else foundObj = obj;

        // Do we have a type?
        type = ti_cast<Spp::Ast::Type>(foundObj);
        if (type != 0) {
          return Core::Data::Seeker::Verb::STOP;
        }

        // If we have a template then try to get a default instance.
        auto tpl = ti_cast<Spp::Ast::Template>(foundObj);
        if (tpl != 0) {
          Core::Data::Ast::List list;
          TioSharedPtr result;
          if (tpl->matchInstance(&list, helper, result)) {
            type = result.ti_cast_get<Spp::Ast::Type>();
            if (type != 0) {
              return Core::Data::Seeker::Verb::STOP;
            }
          } else {
            if (result != 0 && result->isDerivedFrom<Core::Notices::Notice>()) notice = result.s_cast<Core::Notices::Notice>();
          }
        }

        return Core::Data::Seeker::Verb::MOVE;
      }, 0
    );
  }

  if (type == 0 && helper->noticeStore != 0) {
    if (notice != 0) helper->noticeStore->add(notice);
    if (foundObj == 0) {
      helper->noticeStore->add(
        newSrdObj<Spp::Notices::InvalidTypeNotice>(Core::Data::Ast::findSourceLocation(ref))
      );
    } else {
      helper->noticeStore->add(
        newSrdObj<Spp::Notices::IdentifierIsNotTypeNotice>(Core::Data::Ast::findSourceLocation(ref))
      );
    }
  }

  return type;
}


Bool Helper::_isVoid(TiObject *self, TiObject const *ref)
{
  PREPARE_SELF(helper, Helper);

  if (ref == 0) return true;
  auto type = helper->traceType(const_cast<TiObject*>(ref));
  if (type == 0) return false;
  return type->isA<VoidType>();
}


Bool Helper::_isImplicitlyCastableTo(
  TiObject *self, TiObject *srcTypeRef, TiObject *targetTypeRef, ExecutionContext const *ec
) {
  PREPARE_SELF(helper, Helper);
  Function *caster;
  return helper->matchTargetType(srcTypeRef, targetTypeRef, ec, caster) >= TypeMatchStatus::CUSTOM_CASTER;
}


Bool Helper::_isExplicitlyCastableTo(
  TiObject *self, TiObject *srcTypeRef, TiObject *targetTypeRef, ExecutionContext const *ec
) {
  PREPARE_SELF(helper, Helper);
  Function *caster;
  return helper->matchTargetType(srcTypeRef, targetTypeRef, ec, caster) >= TypeMatchStatus::EXPLICIT_CAST;
}


TypeMatchStatus Helper::_matchTargetType(
  TiObject *self, TiObject *srcTypeRef, TiObject *targetTypeRef, ExecutionContext const *ec, Function *&caster
) {
  PREPARE_SELF(helper, Helper);

  auto srcType = ti_cast<Type>(srcTypeRef);
  if (srcType == 0) {
    srcType = helper->traceType(srcTypeRef);
    if (srcType == 0) return TypeMatchStatus::NONE;
  }

  auto targetType = ti_cast<Type>(targetTypeRef);
  if (targetType == 0) {
    targetType = helper->traceType(targetTypeRef);
    if (targetType == 0) return TypeMatchStatus::NONE;
  }

  // If the target type is a temp_ref then we'll cast to the content type instead of the ref type, and then update
  // derefs accordingly if we found a match. This is because even if we have a value rather than a reference the
  // casting is still possible since this is a temp ref and we can create a temp var to convert values into
  // references.
  Bool negativeDeref = false;
  if (
    targetType->isDerivedFrom<ReferenceType>() &&
    static_cast<ReferenceType*>(targetType)->getMode() == Ast::ReferenceMode::TEMP_EXPLICIT
  ) {
    negativeDeref = true;
    targetType = static_cast<ReferenceType*>(targetType)->getContentType(helper);
  }
  auto matchType = srcType->matchTargetType(targetType, helper, ec);
  if (negativeDeref && (matchType == TypeMatchStatus::AGGREGATION || matchType >= TypeMatchStatus::REF_AGGREGATION)) {
    --matchType.derefs;
    if (matchType == TypeMatchStatus::AGGREGATION) matchType.value = TypeMatchStatus::REF_AGGREGATION;
    return matchType;
  } else if (!negativeDeref && matchType >= TypeMatchStatus::EXPLICIT_CAST) {
    return matchType;
  } else {
    Int deref = 0;
    while (srcType->isDerivedFrom<ReferenceType>()) {
      auto customMatchType = helper->lookupCustomCaster(srcType, targetType, ec, caster);
      if (
        (negativeDeref && (
          customMatchType==TypeMatchStatus::AGGREGATION || customMatchType>=TypeMatchStatus::REF_AGGREGATION
        )) ||
        (!negativeDeref && customMatchType>=TypeMatchStatus::IMPLICIT_CAST)
      ) {
        return TypeMatchStatus(TypeMatchStatus::CUSTOM_CASTER, deref);
      }
      ++deref;
      srcType = static_cast<ReferenceType*>(srcType)->getContentType(helper);
    }
    if (negativeDeref) return TypeMatchStatus::NONE;
    else return matchType;
  }
}


Bool Helper::_isReferenceTypeFor(TiObject *self, Type *refType, Type *contentType, ExecutionContext const *ec)
{
  PREPARE_SELF(helper, Helper);

  auto referenceType = ti_cast<ReferenceType>(refType);
  if (referenceType == 0) return false;
  return referenceType->getContentType(helper)->isEqual(contentType, helper, ec);
}


ReferenceType* Helper::_getReferenceTypeFor(TiObject *self, TiObject *type, ReferenceMode const &mode)
{
  PREPARE_SELF(helper, Helper);

  auto tpl = helper->getReferenceTemplate(mode);

  TioSharedPtr result;
  if (tpl->matchInstance(type, helper, result)) {
    auto refType = result.ti_cast_get<ReferenceType>();
    if (refType == 0) {
      throw EXCEPTION(GenericException, S("Template for reference type is invalid."));
    }
    return refType;
  } else {
    auto notice = result.ti_cast<Core::Notices::Notice>();
    if (notice != 0) {
      helper->noticeStore->add(notice);
    }
    return 0;
  }
}


ReferenceType* Helper::getReferenceTypeForPointerType(PointerType *type, ReferenceMode const &mode)
{
  if (type == 0) {
    throw EXCEPTION(InvalidArgumentException, S("type"), S("Cannot be null."));
  }

  return this->getReferenceTypeFor(type->getContentType(this), mode);
}


PointerType* Helper::_getPointerTypeFor(TiObject *self, TiObject *type)
{
  PREPARE_SELF(helper, Helper);

  auto tpl = helper->getPointerTemplate();

  TioSharedPtr result;
  if (tpl->matchInstance(type, helper, result)) {
    auto refType = result.ti_cast_get<PointerType>();
    if (refType == 0) {
      throw EXCEPTION(GenericException, S("Template for pointer type is invalid."));
    }
    return refType;
  } else {
    auto notice = result.ti_cast<Core::Notices::Notice>();
    if (notice != 0) {
      helper->noticeStore->add(notice);
    }
    return 0;
  }
}


ArrayType* Helper::_getArrayTypeFor(TiObject *self, TiObject *type)
{
  PREPARE_SELF(helper, Helper);

  auto tpl = helper->getArrayTemplate();

  TioSharedPtr result;
  if (tpl->matchInstance(type, helper, result)) {
    auto refType = result.ti_cast_get<ArrayType>();
    if (refType == 0) {
      throw EXCEPTION(GenericException, S("Template for array type is invalid."));
    }
    return refType;
  } else {
    auto notice = result.ti_cast<Core::Notices::Notice>();
    if (notice != 0) {
      helper->noticeStore->add(notice);
    }
    return 0;
  }
}


Type* Helper::swichInnerReferenceTypeWithPointerType(ReferenceType *type)
{
  if (type == 0) {
    throw EXCEPTION(InvalidArgumentException, S("type"), S("Cannot be null."));
  }

  auto contentType = type->getContentType(this);
  auto innerRefType = ti_cast<ReferenceType>(contentType);
  if (innerRefType != 0 && innerRefType->isAutoDeref()) {
    return this->getReferenceTypeFor(this->swichInnerReferenceTypeWithPointerType(innerRefType), type->getMode());
  } else {
    return this->getPointerTypeFor(contentType);
  }
}


Type* Helper::swichOuterPointerTypeWithReferenceType(Type *type, ReferenceMode const &mode)
{
  if (type == 0) {
    throw EXCEPTION(InvalidArgumentException, S("type"), S("Cannot be null."));
  }

  if (type->isDerivedFrom<PointerType>()) {
    auto contentType = static_cast<PointerType*>(type)->getContentType(this);
    return this->getReferenceTypeFor(contentType, mode);
  } else if (type->isDerivedFrom<ReferenceType>()) {
    auto refType = static_cast<ReferenceType*>(type);
    auto contentType = refType->getContentType(this);
    auto switchedContentType = this->swichOuterPointerTypeWithReferenceType(contentType, mode);
    if (switchedContentType == 0) return 0;
    return this->getReferenceTypeFor(switchedContentType, refType->getMode());
  } else {
    return 0;
  }
}


Type* Helper::_getValueTypeFor(TiObject *self, TiObject *typeRef)
{
  PREPARE_SELF(helper, Helper);

  Type *type = helper->traceType(typeRef);
  if (type->isDerivedFrom<ReferenceType>()) {
    return static_cast<ReferenceType*>(type)->getContentType(helper);
  } else {
    return type;
  }
}


IntegerType* Helper::_getNullType(TiObject *self)
{
  PREPARE_SELF(helper, Helper);
  // Prepare the reference.
  if (helper->nullType == 0) {
    // Create a new reference.
    auto typeRef = helper->rootManager->parseExpression(S("Null")).s_cast<Core::Data::Ast::Identifier>();
    typeRef->setOwner(helper->rootManager->getRootScope().get());

    helper->nullType = ti_cast<Ast::IntegerType>(
      helper->getSeeker()->doGet(typeRef.get(), helper->rootManager->getRootScope().get())
    );
    if (helper->nullType == 0) {
      throw EXCEPTION(GenericException, S("Failed to get null AST type."));
    }
  }
  return helper->nullType;
}


IntegerType* Helper::_getBoolType(TiObject *self)
{
  PREPARE_SELF(helper, Helper);
  if (helper->boolType == 0) {
    helper->boolType = helper->getWordType(1);
  }
  return helper->boolType;
}


IntegerType* Helper::_getCharType(TiObject *self)
{
  PREPARE_SELF(helper, Helper);
  if (helper->charType == 0) {
    helper->charType = helper->getWordType(8);
  }
  return helper->charType;
}


ArrayType* Helper::_getCharArrayType(TiObject *self, Word size)
{
  PREPARE_SELF(helper, Helper);

  // Prepare the reference.
  if (helper->charArrayTypeRef == 0) {
    // Create a new reference.
    StrStream stream;
    stream << S("array[Word[8],") << size << S("]");
    helper->charArrayTypeRef = helper->rootManager->parseExpression(stream.str().c_str())
      .s_cast<Core::Data::Ast::ParamPass>();
    helper->charArrayTypeRef->setOwner(helper->rootManager->getRootScope().get());
  } else {
    // Recycle the existing reference.
    auto intLiteral = helper->charArrayTypeRef
      ->getParam().ti_cast_get<Core::Data::Ast::List>()
      ->get(1).ti_cast_get<Core::Data::Ast::IntegerLiteral>();
    if (!intLiteral) {
      throw EXCEPTION(GenericException, S("Unexpected internal error."));
    }
    intLiteral->setValue(std::to_string(size).c_str());
  }
  // Seek the requested type.
  auto astType = ti_cast<Ast::ArrayType>(
    helper->getSeeker()->doGet(helper->charArrayTypeRef.get(), helper->rootManager->getRootScope().get())
  );
  if (astType == 0) {
    throw EXCEPTION(GenericException, S("Failed to get char array AST type."));
  }
  return astType;
}


IntegerType* Helper::_getArchIntType(TiObject *self)
{
  PREPARE_SELF(helper, Helper);
  if (helper->archIntType == 0) {
    helper->archIntType = helper->getIntType(0);
  }
  return helper->archIntType;
}


IntegerType* Helper::_getIntType(TiObject *self, Word size)
{
  PREPARE_SELF(helper, Helper);
  // Prepare the reference.
  if (helper->integerTypeRef == 0) {
    // Create a new reference.
    StrStream stream;
    stream << S("Int[") << size << S("]");
    helper->integerTypeRef = helper->rootManager->parseExpression(stream.str().c_str())
      .s_cast<Core::Data::Ast::ParamPass>();
    helper->integerTypeRef->setOwner(helper->rootManager->getRootScope().get());
  } else {
    // Recycle the existing reference.
    auto intLiteral = helper->integerTypeRef->getParam().ti_cast_get<Core::Data::Ast::IntegerLiteral>();
    if (!intLiteral) {
      throw EXCEPTION(GenericException, S("Unexpected internal error."));
    }
    intLiteral->setValue(std::to_string(size).c_str());
  }
  // Generate the llvm type.
  auto astType = ti_cast<Ast::IntegerType>(
    helper->getSeeker()->doGet(helper->integerTypeRef.get(), helper->rootManager->getRootScope().get())
  );
  if (astType == 0) {
    throw EXCEPTION(GenericException, S("Failed to get integer AST type."));
  }
  return astType;
}


IntegerType* Helper::_getWord64Type(TiObject *self)
{
  PREPARE_SELF(helper, Helper);
  if (helper->word64Type == 0) {
    helper->word64Type = helper->getWordType(64);
  }
  return helper->word64Type;
}


IntegerType* Helper::_getWordType(TiObject *self, Word size)
{
  PREPARE_SELF(helper, Helper);
  // Prepare the reference.
  if (helper->wordTypeRef == 0) {
    // Create a new reference.
    StrStream stream;
    stream << S("Word[") << size << S("]");
    helper->wordTypeRef = helper->rootManager->parseExpression(stream.str().c_str())
      .s_cast<Core::Data::Ast::ParamPass>();
    helper->wordTypeRef->setOwner(helper->rootManager->getRootScope().get());
  } else {
    // Recycle the existing reference.
    auto intLiteral = helper->wordTypeRef->getParam().ti_cast_get<Core::Data::Ast::IntegerLiteral>();
    if (!intLiteral) {
      throw EXCEPTION(GenericException, S("Unexpected internal error."));
    }
    intLiteral->setValue(std::to_string(size).c_str());
  }
  // Generate the llvm type.
  auto astType = ti_cast<Ast::IntegerType>(
    helper->getSeeker()->doGet(helper->wordTypeRef.get(), helper->rootManager->getRootScope().get())
  );
  if (astType == 0) {
    throw EXCEPTION(GenericException, S("Failed to get integer AST type."));
  }
  return astType;
}


FloatType* Helper::_getFloatType(TiObject *self, Word size)
{
  PREPARE_SELF(helper, Helper);
  // Prepare the reference.
  if (helper->floatTypeRef == 0) {
    // Create a new reference.
    StrStream stream;
    stream << S("Float[") << size << S("]");
    helper->floatTypeRef = helper->rootManager->parseExpression(stream.str().c_str())
      .s_cast<Core::Data::Ast::ParamPass>();
    helper->floatTypeRef->setOwner(helper->rootManager->getRootScope().get());
  } else {
    // Recycle the existing reference.
    auto intLiteral = helper->floatTypeRef->getParam().ti_cast_get<Core::Data::Ast::IntegerLiteral>();
    if (!intLiteral) {
      throw EXCEPTION(GenericException, S("Unexpected internal error."));
    }
    intLiteral->setValue(std::to_string(size).c_str());
  }
  // Generate the llvm type.
  auto astType = ti_cast<Ast::FloatType>(
    helper->getSeeker()->doGet(helper->floatTypeRef.get(), helper->rootManager->getRootScope().get())
  );
  if (astType == 0) {
    throw EXCEPTION(GenericException, S("Failed to get float AST type."));
  }
  return astType;
}


VoidType* Helper::_getVoidType(TiObject *self)
{
  PREPARE_SELF(helper, Helper);
  if (helper->voidType == 0) {
    auto typeRef = helper->rootManager->parseExpression(S("Void")).s_cast<Core::Data::Ast::Identifier>();
    typeRef->setOwner(helper->rootManager->getRootScope().get());

    helper->voidType = ti_cast<VoidType>(helper->getSeeker()->doGet(
      typeRef.get(), helper->rootManager->getRootScope().get()
    ));
    if (helper->voidType == 0) {
      throw EXCEPTION(GenericException, S("Invalid object found for void type."));
    }
  }
  return helper->voidType;
}


UserType* Helper::_getTiObjectType(TiObject *self)
{
  PREPARE_SELF(helper, Helper);
  if (helper->tiObjectType == 0) {
    auto typeRef = helper->rootManager->parseExpression(S("Core.Basic.TiObject")).s_cast<Core::Data::Node>();
    typeRef->setOwner(helper->rootManager->getRootScope().get());

    helper->tiObjectType = ti_cast<UserType>(helper->getSeeker()->tryGet(
      typeRef.get(), helper->rootManager->getRootScope().get()
    ));
  }
  return helper->tiObjectType;
}


Str Helper::_resolveNodePath(TiObject *self, Core::Data::Node const *node)
{
  PREPARE_SELF(helper, Helper);
  return helper->nodePathResolver->doResolve(node, helper);
}


Str const& Helper::_getFunctionName(TiObject *self, Function *astFunc)
{
  if (astFunc->getName().getStr() == S("")) {
    PREPARE_SELF(helper, Helper);
    Str name = helper->nodePathResolver->doResolve(astFunc, helper);
    astFunc->setName(name);
  }
  return astFunc->getName().getStr();
}


Word Helper::_getNeededIntSize(TiObject *self, LongInt value)
{
  if (value == 0 || value == -1) return 1;
  else {
    Word bitCount = 8;
    for (Int i = 0; i < 7; ++i) {
      value >>= 7;
      if (value == 0 || value == -1) break;
      value >>= 1;
      bitCount += 8;
    }
    if (bitCount <= 16) return bitCount;
    else if (bitCount <= 32) return 32;
    else if (bitCount <= 64) return 64;
    else return 128;
  }
}


Word Helper::_getNeededWordSize(TiObject *self, LongWord value)
{
  if (value == 0 || value == 1) return 1;
  else {
    Word bitCount = 8;
    for (Int i = 0; i < 7; ++i) {
      value >>= 8;
      if (value == 0) break;
      bitCount += 8;
    }
    if (bitCount <= 16) return bitCount;
    else if (bitCount <= 32) return 32;
    else if (bitCount <= 64) return 64;
    else return 128;
  }
}


DefinitionDomain Helper::_getVariableDomain(TiObject *self, TiObject const *obj)
{
  // Find the definition.
  auto def = ti_cast<Core::Data::Ast::Definition const>(obj);
  if (def == 0) {
    auto node = ti_cast<Core::Data::Node const>(obj);
    if (node == 0) {
      throw EXCEPTION(InvalidArgumentException, S("obj"), S("Object is null or of invalid type."), obj);
    }
    def = ti_cast<Core::Data::Ast::Definition const>(node->getOwner());
    if (def == 0) {
      // This could be a function arg.
      if (
        node->getOwner() != 0 && node->getOwner()->getOwner() != 0 &&
        node->getOwner()->getOwner()->isDerivedFrom<FunctionType>()
      ) {
        return DefinitionDomain::FUNCTION;
      } else {
        throw EXCEPTION(InvalidArgumentException, S("obj"), S("Object is not a definition."), obj);
      }
    }
  }

  auto owner = def->getOwner();
  while (owner != 0) {
    if (owner->isDerivedFrom<Module>()) {
      return DefinitionDomain::GLOBAL;
    } else if (owner->isDerivedFrom<Type>()) {
      PREPARE_SELF(helper, Helper);
      return helper->isSharedDef(def) ? DefinitionDomain::GLOBAL : DefinitionDomain::OBJECT;
    } else if (owner->isDerivedFrom<Function>()) {
      PREPARE_SELF(helper, Helper);
      return helper->isSharedDef(def) ? DefinitionDomain::GLOBAL : DefinitionDomain::FUNCTION;
    } else if (owner->isDerivedFrom<EvalStatement>()) {
      return DefinitionDomain::FUNCTION;
    } else if (owner->isDerivedFrom<WhileStatement>()) {
      return DefinitionDomain::FUNCTION;
    } else if (owner->isDerivedFrom<ForStatement>()) {
      return DefinitionDomain::FUNCTION;
    } else if (owner->isDerivedFrom<IfStatement>()) {
      return DefinitionDomain::FUNCTION;
    }
    owner = owner->getOwner();
  }
  return DefinitionDomain::GLOBAL;
}


DefinitionDomain Helper::_getFunctionDomain(TiObject *self, TiObject const *obj)
{
  // Find the function type.
  Core::Data::Node const *owner;
  FunctionType const *funcType;
  if (obj->isDerivedFrom<Core::Data::Ast::Definition>()) {
    auto definition = static_cast<Core::Data::Ast::Definition const*>(obj);
    auto function = definition->getTarget().ti_cast_get<Function const>();
    funcType = function->getType().get();
    owner = definition->getOwner();
  } else if (obj->isDerivedFrom<Function>()) {
    auto function = static_cast<Function const*>(obj);
    funcType = function->getType().get();
    owner = function->getOwner();
  } else if (obj->isDerivedFrom<FunctionType>()) {
    funcType = static_cast<FunctionType const*>(obj);
    owner = funcType->getOwner();
  } else {
    throw EXCEPTION(InvalidArgumentException, S("obj"), S("Invalid type."));
  }

  while (owner != 0) {
    if (owner->isDerivedFrom<Module>()) {
      return DefinitionDomain::GLOBAL;
    } else if (owner->isDerivedFrom<Type>()) {
      return funcType->isShared() ? DefinitionDomain::GLOBAL : DefinitionDomain::OBJECT;
    } else if (owner->isDerivedFrom<Function>()) {
      return DefinitionDomain::GLOBAL;
    } else if (owner->isDerivedFrom<EvalStatement>()) {
      return DefinitionDomain::GLOBAL;
    } else if (owner->isDerivedFrom<WhileStatement>()) {
      return DefinitionDomain::GLOBAL;
    } else if (owner->isDerivedFrom<ForStatement>()) {
      return DefinitionDomain::GLOBAL;
    } else if (owner->isDerivedFrom<IfStatement>()) {
      return DefinitionDomain::GLOBAL;
    }
    owner = owner->getOwner();
  }
  return DefinitionDomain::GLOBAL;
}


Bool Helper::doesModifierExistOnDef(Core::Data::Ast::Definition const *def, Char const *name)
{
  auto modifiers = def->getModifiers().get();
  if (modifiers != 0) {
    for (Int i = 0; i < modifiers->getElementCount(); ++i) {
      auto identifier = ti_cast<Core::Data::Ast::Identifier>(modifiers->getElement(i));
      if (identifier != 0 && identifier->getValue() == name) return true;
    }
  }
  return false;
}


Bool Helper::_validateUseStatement(TiObject *self, Core::Data::Ast::Bridge *bridge)
{
  PREPARE_SELF(helper, Helper);
  VALIDATE_NOT_NULL(bridge);
  if (bridge->getTarget() == 0) {
    throw EXCEPTION(InvalidArgumentException, S("bridge"), S("Use statement has a null target."));
  }
  Bool found = false;
  helper->getSeeker()->foreach(bridge->getTarget().get(), bridge->getOwner(),
    [=, &found] (TiObject *obj, Core::Notices::Notice*)->Core::Data::Seeker::Verb
    {
      if (ti_cast<Ast::Module>(obj) != 0) {
        found = true;
        return Core::Data::Seeker::Verb::STOP;
      } else {
        return Core::Data::Seeker::Verb::MOVE;
      }
    }, 0
  );
  if (!found) {
    helper->noticeStore->add(
      newSrdObj<Spp::Notices::InvalidUseStatementNotice>(bridge->findSourceLocation())
    );
  }
  return found;
}


//==============================================================================
// Main Functions

Template* Helper::getReferenceTemplate(ReferenceMode const &mode)
{
  if (mode == ReferenceMode::EXPLICIT && this->refTemplate != 0) return this->refTemplate;
  else if (mode == ReferenceMode::TEMP_EXPLICIT && this->trefTemplate != 0) return this->trefTemplate;
  else if (mode == ReferenceMode::IMPLICIT && this->irefTemplate != 0) return this->irefTemplate;
  else if (mode == ReferenceMode::NO_DEREF && this->ndrefTemplate != 0) return this->ndrefTemplate;

  Core::Data::Ast::Identifier identifier;
  identifier.setValue(
    mode == ReferenceMode::EXPLICIT ? S("ref") : (
      mode == ReferenceMode::TEMP_EXPLICIT ? S("temp_ref") : (mode == ReferenceMode::IMPLICIT ? S("iref") : S("ndref"))
    )
  );
  auto tpl = ti_cast<Template>(rootManager->getSeeker()->doGet(
    &identifier, this->rootManager->getRootScope().get())
  );
  if (tpl == 0) {
    throw EXCEPTION(GenericException, S("Invalid object found for ref template."));
  }
  if (mode == ReferenceMode::EXPLICIT) this->refTemplate = tpl;
  else if (mode == ReferenceMode::TEMP_EXPLICIT) this->trefTemplate = tpl;
  else if (mode == ReferenceMode::IMPLICIT) this->irefTemplate = tpl;
  else this->ndrefTemplate = tpl;
  return tpl;
}


Template* Helper::getPointerTemplate()
{
  if (this->ptrTemplate != 0) return this->ptrTemplate;

  Core::Data::Ast::Identifier identifier;
  identifier.setValue(S("ptr"));
  this->ptrTemplate = ti_cast<Template>(rootManager->getSeeker()->doGet(
    &identifier, this->rootManager->getRootScope().get())
  );
  if (this->ptrTemplate == 0) {
    throw EXCEPTION(GenericException, S("Invalid object found for ptr template."));
  }
  return this->ptrTemplate;
}


Template* Helper::getArrayTemplate()
{
  if (this->arrayTemplate != 0) return this->arrayTemplate;

  Core::Data::Ast::Identifier identifier;
  identifier.setValue(S("array"));
  this->arrayTemplate = ti_cast<Template>(rootManager->getSeeker()->doGet(
    &identifier, this->rootManager->getRootScope().get())
  );
  if (this->arrayTemplate == 0) {
    throw EXCEPTION(GenericException, S("Invalid object found for array template."));
  }
  return this->arrayTemplate;
}

} } // namespace
