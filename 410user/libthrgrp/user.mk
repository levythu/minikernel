410ULIB_THRGRP_OBJS := thrgrp.o
410ULIB_THRGRP_OBJS := $(410ULIB_THRGRP_OBJS:%=$(410UDIR)/libthrgrp/%)

ALL_410UOBJS += $(410ULIB_THRGRP_OBJS)
410UCLEANS += $(410UDIR)/libthrgrp.a

$(410UDIR)/libthrgrp.a: $(410ULIB_THRGRP_OBJS)
