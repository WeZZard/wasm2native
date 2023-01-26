
// This file defines the options accepted by w2n -frontend (aka
// w2n-frontend).

let Flags = [FrontendOption, NoDriverOption] in {

def use_malloc : Flag<["-"], "use-malloc">,
  HelpText<"Allocate internal data structures using malloc "
           "(for memory debugging)">;

}

def enable_stack_protector :
  Flag<["-"], "enable-stack-protector">,
  HelpText<"Enable the stack protector">;
def disable_stack_protector :
  Flag<["-"], "disable-stack-protector">,
  HelpText<"Disable the stack-protector">;
def enable_move_inout_stack_protector :
  Flag<["-"], "enable-move-inout-stack-protector">,
  HelpText<"Enable the stack protector by moving values to temporaries">;
