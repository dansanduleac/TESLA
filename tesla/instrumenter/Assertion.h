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

#include "llvm/Pass.h"

#include <set>


namespace llvm {
  class CallInst;
}

namespace tesla {

class Automaton;
class Manifest;
class NowTransition;
class Location;

/// Converts calls to TESLA pseudo-assertions into instrumentation sites.
class TeslaAssertionSiteInstrumenter : public llvm::ModulePass {
public:
  static char ID;
  TeslaAssertionSiteInstrumenter() : ModulePass(ID) {}
  virtual ~TeslaAssertionSiteInstrumenter();

  virtual bool runOnModule(llvm::Module &M);

private:
  //! Convert assertion declarations into instrumentation calls.
  bool ConvertAssertions(std::set<llvm::CallInst*>&, Manifest&, llvm::Module&);

  //! Add instrumentation for all automata in a @ref Manifest.
  bool AddInstrumentation(const Manifest&, llvm::Module& M);

  //! Add instrumentation to an @ref Automaton's event handler.
  bool AddInstrumentation(const Automaton&, llvm::Module& M);

  //! Add instrumentation for a single @ref NowTransition.
  bool AddInstrumentation(const NowTransition&, const Automaton&,
                          llvm::Module& M);

  /**
   * Parse a @ref Location out of a @ref CallInst to the TESLA assertion
   * pseudo-call.
   */
  static void ParseAssertionLocation(Location *Loc, llvm::CallInst*);

  /**
   * Find (or create) the instrumentation function for an @ref Assertion's
   * 'NOW' event.
   */
  static llvm::Function* InstrumentationFn(const Automaton&, llvm::Module&);

  //! The TESLA pseudo-function used to declare assertions.
  llvm::Function *AssertFn = NULL;
};

}

