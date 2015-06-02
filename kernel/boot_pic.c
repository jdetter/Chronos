#include "console.h"

static char* boot_picture = 
"                                             ````` `-``.````-/. -`+-/+.````/.`` "
"                                                 ````-` ````-/-.. /++/--..:`    "
"                                                    `.-   . `/-.  /+/.`.-:-```  "
"                                                      .-  ```::`  //.`.--.`   ``"
"              Chronos is loading...                    ..  `../  `+:`.-.`     `."
"                                                        ..  `-/` -+-..  ``  ``.+"
"                                                         -.  -/- :/-`   ` ```.+/"
"                                                         `:`  :/`//.       .-/:."
"                                                          .:` -+:/:`  `` .-.:.  "
"                                                          `:- -+/+.  ````:--`   "
"                                                         ` ::`.+/+`  ```:/-     "
"                                                     ```   `+-.+++` `  -+.      "
"                                                            -/`:++    :+`       "
"                                                             -//o+  ./:`        "
"                                                              -ooo.:+.          "
"                                                               +ooo:`           "
"                                                               -oo+             "
"                                                               .oo+             "
"                                                               `oso             "
"                                                               `oso             "
"                            ``                                 `oss     `   ``  "
"                       ``` ``` ````.-.`..-.`.```..``.-....------ooo:::::::/:/::/"
"+++++/+//:://:--/:.    ..-.--------:++++++++o++oo+++ooo+o++osoooossyyyhhhyhhhyhh"
"++++++++++/////////:...::::::::::://///+++ossossssyyyyysysyyyyysyyyhhhhhhyyyhhyy"
"mmmddddddhmmmmmmmddmmdddddddhhhhhhhhddddddddddddddddddmdddddhhhhhhhhdddhhdhhhhhh";

void display_boot_pic()
{
	cprintf("%s", boot_picture);
}
