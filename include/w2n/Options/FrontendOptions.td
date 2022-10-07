
// This file defines the options accepted by w2n -frontend (aka
// w2n-frontend).

let Flags = [FrontendOption, NoDriverOption] in {

def use_malloc : Flag<["-"], "use-malloc">,
  HelpText<"Allocate internal data structures using malloc "
           "(for memory debugging)">;

}