/*! @file callee.cpp  Code for instrumenting function calls (callee context). */
/*
 * Copyright (c) 2012 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "callee.h"

#include "Instrumentation.h"
#include "Manifest.h"

#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/IRBuilder.h"
#include "llvm/Module.h"

#include "llvm/Support/raw_ostream.h"

using namespace llvm;

using std::string;
using std::vector;

namespace tesla {


// ==== CalleeInstrumentation implementation ===================================
bool CalleeInstrumentation::InstrumentEntry(Function &Fn) {
  if (&Fn != this->Fn) return false;
  if (EntryEvent == NULL) return false;

  // Instrumenting function entry is easy: just add a new call to
  // instrumentation at the beginning of the function's entry block.
  BasicBlock& Entry = Fn.getEntryBlock();
  CallInst::Create(EntryEvent, Args)->insertBefore(Entry.getFirstNonPHI());

  return true;
}

bool CalleeInstrumentation::InstrumentReturn(Function &Fn) {
  if (&Fn != this->Fn) return false;
  if (ReturnEvent == NULL) return false;

  // First, build up the set of blocks that return from the function.
  vector<BasicBlock*> ReturnBlocks;

  // We explicitly iterate over BasicBlocks (rather than using an InstIterator)
  // because we need the blocks themselves (later, we'll split them).
  for (auto &Block : Fn) {
    auto *Return = dyn_cast<ReturnInst>(Block.getTerminator());
    if (Return) ReturnBlocks.push_back(&Block);
  }

  // Finally, instrument the returns.
  for (auto *Block : ReturnBlocks) {
    auto *Return = cast<ReturnInst>(Block->getTerminator());
    Value *RetVal = Return->getReturnValue();

    vector<Value*> InstrumentationArgs;
    if (RetVal) InstrumentationArgs.push_back(RetVal);
    InstrumentationArgs.insert(InstrumentationArgs.end(), Args.begin(), Args.end());

    CallInst::Create(ReturnEvent, InstrumentationArgs)->insertBefore(Return);
  }

  return false;
}

CalleeInstrumentation* CalleeInstrumentation::Build(
  LLVMContext &Context, Module &M, StringRef FnName,
  FunctionEvent::Direction Dir)
{
  Function *Fn = M.getFunction(FnName);
  if (Fn == NULL) return NULL;

  Function *Entry = NULL;
  Function *Return = NULL;

  if (Fn) {
    // Instrumentation functions do not return.
    Type *VoidTy = Type::getVoidTy(Context);

    // Get the argument types of the function to be instrumented.
    vector<Type*> ArgTypes;
    for (auto &Arg : Fn->getArgumentList()) ArgTypes.push_back(Arg.getType());

    // Declare or retrieve instrumentation functions.
    if (Dir & FunctionEvent::Entry) {
      string Name = ("__tesla_instrumentation_callee_enter_" + FnName).str();
      auto InstrType = FunctionType::get(VoidTy, ArgTypes, Fn->isVarArg());
      Entry = cast<Function>(M.getOrInsertFunction(Name, InstrType));
      assert(Entry != NULL);
    }

    if (Dir & FunctionEvent::Exit) {
      // Instrumentation of returns must include the returned value...
      vector<Type*> RetTypes(ArgTypes);
      if (!Fn->getReturnType()->isVoidTy())
        RetTypes.insert(RetTypes.begin(), Fn->getReturnType());

      string Name = ("__tesla_instrumentation_callee_return_" + FnName).str();
      auto InstrType = FunctionType::get(VoidTy, RetTypes, Fn->isVarArg());
      Return = cast<Function>(M.getOrInsertFunction(Name, InstrType));
      assert(Return != NULL);
    }
  }

  return new CalleeInstrumentation(Fn, Entry, Return);
}

CalleeInstrumentation::CalleeInstrumentation(
  Function *Fn, Function *Entry, Function *Return)
  : Fn(Fn), EntryEvent(Entry), ReturnEvent(Return) {

  // Record the arguments passed to the instrumented function.
  //
  // LLVM's SSA magic will keep these around for us until we need them, even if
  // C code overwrites its parameters.
  for (auto &Arg : Fn->getArgumentList()) Args.push_back(&Arg);
}


// ==== CalleeInstrumenter implementation ======================================
char tesla::TeslaCalleeInstrumenter::ID = 0;

TeslaCalleeInstrumenter::~TeslaCalleeInstrumenter() {
  google::protobuf::ShutdownProtobufLibrary();
}

bool TeslaCalleeInstrumenter::doInitialization(Module &M) {
  OwningPtr<Manifest> Manifest(Manifest::load(llvm::errs()));
  if (!Manifest) return false;

  for (auto& Fn : Manifest->FunctionsToInstrument()) {
    if (!Fn.context() & FunctionEvent::Callee) continue;

    assert(Fn.has_function());
    auto Name = Fn.function().name();

    FunctionsToInstrument[Name] =
      CalleeInstrumentation::Build(M.getContext(), M, Name, Fn.direction());
  }

  return false;
}

bool TeslaCalleeInstrumenter::runOnFunction(Function &F) {
  auto I = FunctionsToInstrument.find(F.getName());
  if (I == FunctionsToInstrument.end()) return false;

  auto *FnInstrumenter = I->second;
  assert(FnInstrumenter != NULL);

  bool modifiedIR = false;
  modifiedIR |= FnInstrumenter->InstrumentEntry(F);
  modifiedIR |= FnInstrumenter->InstrumentReturn(F);

  return modifiedIR;
}

} /* namespace tesla */

static RegisterPass<tesla::TeslaCalleeInstrumenter> Callee("tesla-callee",
  "TESLA instrumentation: callee context");
