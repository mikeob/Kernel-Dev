# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(setuid) begin
(setuid) uid: 0
(setuid) uid: 1
(setuid) end
setuid: exit(0)
EOF
pass;
