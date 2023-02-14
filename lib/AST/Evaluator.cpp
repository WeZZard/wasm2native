/// This file implements the \c Evaluator class that evaluates and caches
/// requests.

#include <llvm/ADT/StringExtras.h>
#include <vector>
#include <w2n/AST/DeclContext.h>
#include <w2n/AST/DiagnosticEngine.h>
#include <w2n/AST/Evaluator.h>
#include <w2n/Basic/LanguageOptions.h>
#include <w2n/Basic/Range.h>
#include <w2n/Basic/SourceManager.h>

using namespace w2n;

AbstractRequestFunction * Evaluator::getAbstractRequestFunction(
  uint8_t ZoneId, uint8_t RequestId
) const {
  for (const auto& Zone : RequestFunctionsByZone) {
    if (Zone.first == ZoneId) {
      if (RequestId < Zone.second.size()) {
        return Zone.second[RequestId];
      }

      return nullptr;
    }
  }

  return nullptr;
}

void Evaluator::registerRequestFunctions(
  Zone Zone, ArrayRef<AbstractRequestFunction *> Functions
) {
  uint8_t ZoneId = static_cast<uint8_t>(Zone);
#ifndef NDEBUG
  for (const auto& EachZone : RequestFunctionsByZone) {
    assert(EachZone.first != ZoneId);
  }
#endif

  RequestFunctionsByZone.push_back({ZoneId, Functions});
}

Evaluator::Evaluator(
  DiagnosticEngine& Diags, const LanguageOptions& Opts
) :
  Diags(Diags),
  DebugDumpCycles(Opts.DebugDumpCycles),
  Recorder(Opts.RecordRequestReferences) {
}

bool Evaluator::checkDependency(const ActiveRequest& Req) {
  // Record this as an active request.
  if (ActiveRequests.insert(Req)) {
    return false;
  }

  // Diagnose cycle.
  diagnoseCycle(Req);
  return true;
}

void Evaluator::diagnoseCycle(const ActiveRequest& Req) {
  if (DebugDumpCycles) {
    const auto PrintIndent = [](llvm::raw_ostream& OS, unsigned Indent) {
      OS.indent(Indent);
      OS << "`--";
    };

    unsigned Indent = 1;
    auto& OS = llvm::errs();

    OS << "===CYCLE DETECTED===\n";
    for (const auto& Step : ActiveRequests) {
      PrintIndent(OS, Indent);
      if (Step == Req) {
        OS.changeColor(llvm::raw_ostream::GREEN);
        simple_display(OS, Step);
        OS.resetColor();
      } else {
        simple_display(OS, Step);
      }
      OS << "\n";
      Indent += 4;
    }

    PrintIndent(OS, Indent);
    OS.changeColor(llvm::raw_ostream::GREEN);
    simple_display(OS, Req);

    OS.changeColor(llvm::raw_ostream::RED);
    OS << " (cyclic dependency)";
    OS.resetColor();

    OS << "\n";
  }

  Req.diagnoseCycle(Diags);
  for (const auto& Step : llvm::reverse(ActiveRequests)) {
    if (Step == Req) {
      return;
    }

    Step.noteCycleStep(Diags);
  }

  llvm_unreachable(
    "Diagnosed a cycle but it wasn't represented in the stack"
  );
}

void evaluator::DependencyRecorder::recordDependency(
  const DependencyCollector::Reference& Ref
) {
  if (activeRequestReferences.empty()) {
    return;
  }

  activeRequestReferences.back().insert(Ref);
}

evaluator::DependencyCollector::DependencyCollector(
  evaluator::DependencyRecorder& Parent
) :
  Parent(Parent) {
#ifndef NDEBUG
  assert(
    !Parent.isRecording
    && "Probably not a good idea to allow nested recording"
  );
  Parent.isRecording = true;
#endif
}

evaluator::DependencyCollector::~DependencyCollector() {
#ifndef NDEBUG
  assert(
    Parent.isRecording && "Should have been recording this whole time"
  );
  Parent.isRecording = false;
#endif
}

void evaluator::DependencyCollector::addUsedMember(
  DeclContext * Subject, DeclBaseName Name
) {
  // FIXME: assert(subject->isTypeContext());
  return Parent.recordDependency(Reference::usedMember(Subject, Name));
}

void evaluator::DependencyCollector::addPotentialMember(
  DeclContext * Subject
) {
  // FIXME: assert(subject->isTypeContext());
  return Parent.recordDependency(Reference::potentialMember(Subject));
}

void evaluator::DependencyCollector::addTopLevelName(DeclBaseName Name) {
  return Parent.recordDependency(Reference::topLevel(Name));
}

void evaluator::DependencyCollector::addDynamicLookupName(
  DeclBaseName Name
) {
  return Parent.recordDependency(Reference::dynamic(Name));
}

void evaluator::DependencyRecorder::enumerateReferencesInFile(
  const SourceFile * SF, ReferenceEnumerator F
) const {
  auto Entry = fileReferences.find(SF);
  if (Entry == fileReferences.end()) {
    return;
  }

  for (const auto& Ref : Entry->getSecond()) {
    switch (Ref.RefKind) {
    case DependencyCollector::Reference::Kind::Empty:
    case DependencyCollector::Reference::Kind::Tombstone:
      llvm_unreachable("Cannot enumerate dead reference!");
    case DependencyCollector::Reference::Kind::UsedMember:
    case DependencyCollector::Reference::Kind::PotentialMember:
    case DependencyCollector::Reference::Kind::TopLevel:
    case DependencyCollector::Reference::Kind::Dynamic: F(Ref);
    }
  }
}
