###################### FULL BUILD TARGETS ############################

FINALTARGETS=kernel
FINALVERYCLEANS=$(FINALTARGETS)
FINALCLEANS=$(FINALTARGETS:%=%.gz) $(FINALTARGETS:%=%.strip)

$(patsubst %,$(BUILDDIR)/%.o,$(FINALTARGETS)) \
	: $(BUILDDIR)/%.o \
	: \
		$(KERNEL_BOOT_HEAD) \
		$(410KDIR)/partial_%.o \
		$(STUKDIR)/partial_%.o \
		$(410KLIBS)
	mkdir -p $(BUILDDIR)
	$(LD) -r $(KLDFLAGS) -o $@ $(filter-out %.a,$^) \
		--start-group $(410KLIBS) --end-group

$(FINALTARGETS) : %: $(BUILDDIR)/%.o $(BUILDDIR)/user_apps.o
	$(LD) $(KLDFLAGS) -o $@ $^
#	$(LD) -T $(410KDIR)/kernel.lds $(KLDFLAGS) -o $@ $^


$(patsubst %,%.strip,$(FINALTARGETS)) : %.strip : %
	$(OBJCOPY) -g $< $@

$(patsubst %,%.gz,$(FINALTARGETS)) : %.gz : %.strip
	gzip -c $< > $@

### Boy was that a bad idea, especially for this semester.
### Too clever by half.  Sorry.
### .INTERMEDIATE: $(FINALTARGETS:%=%.strip)
.INTERMEDIATE: $(FINALTARGETS:%=%.gz)

$(410KDIR)/bootfd.img.gz:
	@echo file $(410KDIR)/bootfd.img.gz missing
	@false

$(410KDIR)/menu.lst:
	@echo file $(410KDIR)/menu.lst missing
	@false

ifeq (,$(INFRASTRUCTURE_OVERRIDE_BOOTFDIMG))
bootfd.img: $(FINALTARGETS:%=%.gz) $(410KDIR)/bootfd.img.gz $(410KDIR)/menu.lst
	gzip -cd $(410KDIR)/bootfd.img.gz > $(PROJROOT)/bootfd.img
	mcopy -o -i "$(PROJROOT)/bootfd.img" $(FINALTARGETS:%=%.gz) $(410KDIR)/menu.lst ::/boot/
endif
