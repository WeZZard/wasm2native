// Include the common option parsing interfaces.
include "llvm/Option/OptParser.td"

/////////
// Flags

// The option should be accepted by w2n frontend.
def FrontendOption : OptionFlag;

// The option's argument is a file-system path that may be affected by the
// current working directory.
def ArgumentIsPath : OptionFlag;

// Some options should not be available depending on whether this is the
// interactive driver 'w2n', or the batch compiler 'w2nc'.
def NoInteractiveOption : OptionFlag;

def NoBatchOption : OptionFlag;

// The option should not be accepted by the driver.
def NoDriverOption : OptionFlag;

/////////
// Options

// The internal option ID must be a valid C++ identifier and results in a
// w2n::driver::options::OPT_XX enum constant for XX.
//
// We want to unambiguously be able to refer to options from the driver source
// code, for this reason the option name is mangled into an ID. This mangling
// isn't guaranteed to have an inverse, but for practical purposes it does.
//
// The mangling scheme is to ignore the leading '-', and perform the following
// substitutions:
//   _ => __
//   - => _
//   / => _SLASH
//   # => _HASH
//   ? => _QUESTION
//   , => _COMMA
//   = => _EQ
//   C++ => CXX
//   . => _

def driver_mode : Joined<["--"], "driver-mode=">, Flags<[HelpHidden]>,
  HelpText<"Set the driver mode to either 'w2n' or 'w2nc'">;

def help : Flag<["-", "--"], "help">,
  Flags<[FrontendOption]>,
  HelpText<"Display available options">;
def h : Flag<["-"], "h">, Alias<help>;
def help_hidden : Flag<["-", "--"], "help-hidden">,
  Flags<[HelpHidden, FrontendOption]>,
  HelpText<"Display available options, including hidden options">;

def v : Flag<["-"], "v">,
  Flags<[FrontendOption]>,
  HelpText<"Show commands to run and use verbose output">;
def version : Flag<["-", "--"], "version">, Flags<[FrontendOption]>,
  HelpText<"Print version information and exit">;

// Standard Options
def _DASH_DASH : Option<["--"], "", KIND_REMAINING_ARGS>,
  Flags<[FrontendOption]>;

def o : JoinedOrSeparate<["-"], "o">,
  Flags<[FrontendOption]>,
  HelpText<"Write output to <file>">, MetaVarName<"<file>">;

def j : JoinedOrSeparate<["-"], "j">, Flags<[]>,
  HelpText<"Number of commands to execute in parallel">, MetaVarName<"<n>">;

def sdk : Separate<["-"], "sdk">,
  Flags<[FrontendOption]>,
  HelpText<"Compile against <sdk>">, MetaVarName<"<sdk>">;

def wasm_version : Separate<["-"], "wasm-version">,
  Flags<[FrontendOption]>,
  HelpText<"Interpret input according to a specific Web Assembly version number">,
  MetaVarName<"<vers>">;

def F : JoinedOrSeparate<["-"], "F">,
  Flags<[FrontendOption, ArgumentIsPath]>,
  HelpText<"Add directory to framework search path">;
def F_EQ : Joined<["-"], "F=">, Flags<[FrontendOption, ArgumentIsPath]>,
  Alias<F>;

def Fsystem : Separate<["-"], "Fsystem">,
  Flags<[FrontendOption, ArgumentIsPath]>,
  HelpText<"Add directory to system framework search path">;

def I : JoinedOrSeparate<["-"], "I">,
  Flags<[FrontendOption, ArgumentIsPath]>,
  HelpText<"Add directory to the import search path">;
def I_EQ : Joined<["-"], "I=">, Flags<[FrontendOption, ArgumentIsPath]>,
  Alias<I>;

def entry_point : Flag<["-"], "entry-point">,
  Flags<[FrontendOption]>,
  HelpText<"Entry point function name. Use \"main\" if unspecified.">;

def static : Flag<["-"], "static">,
  Flags<[FrontendOption]>,
  HelpText<"Make this module statically linkable and make the output of -emit-library a static library.">;

def lto : Joined<["-"], "lto=">,
  Flags<[FrontendOption]>,
  HelpText<"Specify the LTO type to either 'llvm-thin' or 'llvm-full'">;

def lto_library : Separate<["-"], "lto-library">,
  Flags<[FrontendOption, ArgumentIsPath]>,
  HelpText<"Perform LTO with <lto-library>">, MetaVarName<"<lto-library>">;

// Linker options

def linker_option_Group : OptionGroup<"<linker-specific options>">;

def l : JoinedOrSeparate<["-"], "l">, Group<linker_option_Group>,
    Flags<[FrontendOption]>,
    HelpText<"Specifies a library which should be linked against">;
def framework : Separate<["-"], "framework">, Group<linker_option_Group>,
  Flags<[FrontendOption]>,
  HelpText<"Specifies a framework which should be linked against">;

def L : JoinedOrSeparate<["-"], "L">, Group<linker_option_Group>,
  Flags<[FrontendOption, ArgumentIsPath]>,
  HelpText<"Add directory to library link search path">;
def L_EQ : Joined<["-"], "L=">, Group<linker_option_Group>,
  Flags<[FrontendOption, ArgumentIsPath]>,
  Alias<L>;

def use_ld : Joined<["-"], "use-ld=">,
  Flags<[]>,
  HelpText<"Specifies the linker to be used">;

def Xlinker : Separate<["-"], "Xlinker">,
  Flags<[]>,
  HelpText<"Specifies an option which should be passed to the linker">;

// Optimization levels

def O_Group : OptionGroup<"<optimization level options>">;

def Onone : Flag<["-"], "Onone">, Group<O_Group>,
  Flags<[FrontendOption]>,
  HelpText<"Compile without any optimization">;
def O : Flag<["-"], "O">, Group<O_Group>,
  Flags<[FrontendOption]>,
  HelpText<"Compile with optimizations">;
def Osize : Flag<["-"], "Osize">, Group<O_Group>,
  Flags<[FrontendOption]>,
  HelpText<"Compile with optimizations and target small code size">;

def modes_Group : OptionGroup<"<mode options>">, HelpText<"MODES">;

class ModeOpt : Group<modes_Group>;

// Output Modes
def emit_executable : Flag<["-"], "emit-executable">,
  HelpText<"Emit a linked executable">, ModeOpt,
  Flags<[]>;
def emit_library : Flag<["-"], "emit-library">,
  HelpText<"Emit a linked library">, ModeOpt,
  Flags<[]>;
def emit_object : Flag<["-"], "emit-object">,
  HelpText<"Emit object file(s) (-c)">, ModeOpt,
  Flags<[FrontendOption]>;
def emit_assembly : Flag<["-"], "emit-assembly">,
  HelpText<"Emit assembly file(s) (-S)">, ModeOpt,
  Flags<[FrontendOption]>;
def emit_bc : Flag<["-"], "emit-bc">,
  HelpText<"Emit LLVM BC file(s)">, ModeOpt,
  Flags<[FrontendOption]>;
def emit_irgen : Flag<["-"], "emit-irgen">,
  HelpText<"Emit LLVM IR file(s) before LLVM optimizations">, ModeOpt,
  Flags<[FrontendOption]>;
def emit_ir : Flag<["-"], "emit-ir">,
  HelpText<"Emit LLVM IR file(s) after LLVM optimizations">, ModeOpt,
  Flags<[FrontendOption]>;

// Other Modes
def repl : Flag<["-"], "repl">,
  HelpText<"REPL mode (the default if there is no input file)">,
  Flags<[FrontendOption, NoBatchOption, HelpHidden]>, ModeOpt;
def lldb_repl : Flag<["-"], "lldb-repl">, HelpText<"LLDB-enhanced REPL mode">,
  Flags<[NoBatchOption, HelpHidden]>, ModeOpt;

def target : Separate<["-"], "target">,
  Flags<[FrontendOption]>,
  HelpText<"Generate code for the given target <triple>, such as aarch64-apple-macos13.0">, MetaVarName<"<triple>">;
def target_legacy_spelling : Joined<["--"], "target=">,
  Flags<[FrontendOption]>, Alias<target>;
