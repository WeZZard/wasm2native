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
  uint8_t zoneID, uint8_t requestID
) const {
  for (const auto& zone : requestFunctionsByZone) {
    if (zone.first == zoneID) {
      if (requestID < zone.second.size())
        return zone.second[requestID];

      return nullptr;
    }
  }

  return nullptr;
}

void Evaluator::registerRequestFunctions(
  Zone zone, ArrayRef<AbstractRequestFunction *> functions
) {
  uint8_t zoneID = static_cast<uint8_t>(zone);
#ifndef NDEBUG
  for (const auto& zone : requestFunctionsByZone) {
    assert(zone.first != zoneID);
  }
#endif

  requestFunctionsByZone.push_back({zoneID, functions});
}

Evaluator::Evaluator(
  DiagnosticEngine& diags, const LanguageOptions& opts
) :
  diags(diags),
  debugDumpCycles(opts.DebugDumpCycles),
  recorder(opts.RecordRequestReferences) {
}

bool Evaluator::checkDependency(const ActiveRequest& request) {
  // Record this as an active request.
  if (activeRequests.insert(request))
    return false;

  // Diagnose cycle.
  diagnoseCycle(request);
  return true;
}

void Evaluator::diagnoseCycle(const ActiveRequest& request) {
  if (debugDumpCycles) {
    const auto printIndent = [](llvm::raw_ostream& OS, unsigned indent) {
      OS.indent(indent);
      OS << "`--";
    };

    unsigned indent = 1;
    auto& OS = llvm::errs();

    OS << "===CYCLE DETECTED===\n";
    for (const auto& step : activeRequests) {
      printIndent(OS, indent);
      if (step == request) {
        OS.changeColor(llvm::raw_ostream::GREEN);
        simple_display(OS, step);
        OS.resetColor();
      } else {
        simple_display(OS, step);
      }
      OS << "\n";
      indent += 4;
    }

    printIndent(OS, indent);
    OS.changeColor(llvm::raw_ostream::GREEN);
    simple_display(OS, request);

    OS.changeColor(llvm::raw_ostream::RED);
    OS << " (cyclic dependency)";
    OS.resetColor();

    OS << "\n";
  }

  request.diagnoseCycle(diags);
  for (const auto& step : llvm::reverse(activeRequests)) {
    if (step == request)
      return;

    step.noteCycleStep(diags);
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
  const SourceFile * SF, ReferenceEnumerator f
) const {
  auto entry = fileReferences.find(SF);
  if (entry == fileReferences.end()) {
    return;
  }

  for (const auto& ref : entry->getSecond()) {
    switch (ref.RefKind) {
    case DependencyCollector::Reference::Kind::Empty:
    case DependencyCollector::Reference::Kind::Tombstone:
      llvm_unreachable("Cannot enumerate dead reference!");
    case DependencyCollector::Reference::Kind::UsedMember:
    case DependencyCollector::Reference::Kind::PotentialMember:
    case DependencyCollector::Reference::Kind::TopLevel:
    case DependencyCollector::Reference::Kind::Dynamic: f(ref);
    }
  }
}
