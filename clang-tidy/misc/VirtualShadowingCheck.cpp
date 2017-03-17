//===--- VirtualShadowingCheck.cpp - clang-tidy----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "VirtualShadowingCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {

namespace
{
} // namespace


void VirtualShadowingCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(functionDecl(hasBody(compoundStmt())).bind("method"), this);
}

void VirtualShadowingCheck::check(const MatchFinder::MatchResult &Result) {
  const auto method = Result.Nodes.getNodeAs<FunctionDecl>("method");

   if (const auto *MethodDecl = dyn_cast<CXXMethodDecl>(method))
     if (!MethodDecl->isConst())
       return;

  if (!method->getReturnType().getTypePtr()->isVoidType())
    return;

  if (method->getNumParams() == 0)
    return;

  if (!method->getParamDecl(0)->getType()->isPointerType()) 
    return;

  auto varName = method->getParamDecl(0)->getDefinition()->getName();

  auto *body = dyn_cast<CompoundStmt>(method->getBody());
  
  if (!body)
    return;

  const auto *ifAtStart = body->size() ? dyn_cast<IfStmt>(body->body_begin()[0]) : nullptr;

  if (!ifAtStart)
    return;

  const auto *unaryAtIfCondition = dyn_cast<UnaryOperator>(ifAtStart->getCond()); 

  if (!unaryAtIfCondition || unaryAtIfCondition->getOpcode() != UO_LNot || !unaryAtIfCondition->getSubExpr())
    return;

  const auto *implicitCastExpr1 = dyn_cast<ImplicitCastExpr>(unaryAtIfCondition->getSubExpr()); 
  if (!implicitCastExpr1)
    return;

  const auto *implicitCastExpr2 = dyn_cast<ImplicitCastExpr>(implicitCastExpr1->getSubExpr()); 
  if (!implicitCastExpr2)
    return;

  const auto *var = dyn_cast<DeclRefExpr>(implicitCastExpr2->getSubExpr()); 
  if (!var)
    return;

  if (var->getDecl()->getDeclName().getAsString() != varName) return;

  if (!dyn_cast<ReturnStmt>(ifAtStart->getThen())) return; 
   
  diag(method->getLocStart(),
       "method could accept reference as parameter thus delegating null check to caller, or use a smart pointer");
}

} // namespace misc
} // namespace tidy
} // namespace clang
