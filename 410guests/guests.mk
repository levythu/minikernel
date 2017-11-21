# Add hooks to push {very,}clean down into the guests
GUEST_CLEAN=for g in $(STUDENTGUESTS); do make -C $(STUGDIR)/$$g clean; done
GUEST_VERYCLEAN=for g in $(STUDENTGUESTS); do make -C $(STUGDIR)/$$g veryclean; done
GUEST_UPDATE=for g in $(STUDENTGUESTS); do make -C $(STUGDIR)/$$g update; done

# Add guest images into the "ramdisk"
FILES+=$(STUDENTGUESTS)

# Having done the above hook, we'll get asked to make targets for
# $(BUILDDIR)/${GUEST}; here's how we do that, using (ick)
# recursive make.  We tell this make that these are .PHONY targets
# so that they always get rebuilt, because we don't have visibility
# into the dependency structure internally.  Probably, that will
# go quickly, and it's not like it takes us a long time to rebuild
# the ramdisk image in light of spurious changes.
.PHONY: $(STUDENTGUESTS:%=$(BUILDDIR)/%)
$(STUDENTGUESTS:%=$(BUILDDIR)/%) : $(BUILDDIR)/% :
	make -C $(STUGDIR)/$* kernel.strip
	mkdir -p $(BUILDDIR)
	cp -p $(STUGDIR)/$*/kernel.strip $@

# Add pre-compiled guest binaries into the "ramdisk"
FILES+=$(410GUESTBINS)

$(410GUESTBINS:%=$(BUILDDIR)/%): \
$(BUILDDIR)/%: $(410GDIR)/%.bin
	mkdir -p $(BUILDDIR)
	strip -o $@ $<
