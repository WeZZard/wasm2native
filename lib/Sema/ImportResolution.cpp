#include <w2n/AST/ASTContext.h>
#include <w2n/AST/SourceFile.h>
#include <w2n/Basic/Defer.h>
#include <w2n/Basic/Statistic.h>
#include <w2n/Sema/TypeCheck.h>

void w2n::performImportResolution(SourceFile& SF) {
  // If we've already performed import resolution, bail.
  if (SF.getASTStage() == SourceFile::ASTStage::ImportsResolved)
    return;

  FrontendStatsTracer tracer(
    SF.getASTContext().Stats, "Import resolution"
  );

  // If we're silencing parsing warnings, then also silence import
  // warnings. This is necessary for secondary files as they can be parsed
  // and have their imports resolved multiple times.
  auto& diags = SF.getASTContext().Diags;
  auto didSuppressWarnings = diags.getSuppressWarnings();
  if (auto * WF = dyn_cast<WatFile>(&SF)) {
    auto shouldSuppress = WF->getParsingOptions().contains(
      WatFile::ParsingFlags::SuppressWarnings
    );
    diags.setSuppressWarnings(didSuppressWarnings || shouldSuppress);
  }
  W2N_DEFER {
    diags.setSuppressWarnings(didSuppressWarnings);
  };

  // ImportResolver resolver(SF);

  // Resolve each import declaration.
  for (auto * D : SF.getTopLevelDecls())
    __unused auto * _ = D; // resolver.visit(D);

  // FIXME: SF.setImports(resolver.getFinishedImports());

  SF.Stage = SourceFile::ASTStage::ImportsResolved;
  // FIXME: verify(SF);
}