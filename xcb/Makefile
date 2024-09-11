XCBPROTO=xcb-proto-1.15
LIBXCB=libxcb-1.15

$(XCBPROTO).tar.gz:
	curl -O https://xorg.freedesktop.org/archive/individual/proto/$(XCBPROTO).tar.gz

$(XCBPROTO): | $(XCBPROTO).tar.gz
	tar -xzf $(XCBPROTO).tar.gz
	cd $(XCBPROTO) && ./configure --prefix="$(shell pwd)/$(XCBPROTO)"
	make -j -C $(XCBPROTO) install

$(LIBXCB).tar.gz:
	curl -O https://xorg.freedesktop.org/archive/individual/lib/$(LIBXCB).tar.gz

$(LIBXCB): | $(XCBPROTO) $(LIBXCB).tar.gz
	tar -xzf $(LIBXCB).tar.gz
	patch -d $(LIBXCB) -p1 < libxcb.patch
	cd $(LIBXCB) && PKG_CONFIG_PATH="$(shell pwd)/$(XCBPROTO)/lib/pkgconfig" ./configure --prefix="$(shell pwd)/$(LIBXCB)" --includedir="$(shell pwd)"
	# We only care about generating src/*.c
	make -j -C $(LIBXCB) install

# also generated .aarch64/libxcb.a
libxcb.a: | $(LIBXCB)
	sh -c 'cd $(LIBXCB)/src && $(COSMOCC) -c *.c -DHAVE_CONFIG_H'
	sh -c 'cd $(LIBXCB)/src && $(COSMOAR) rcs "$(shell pwd)/$@" *.o'

.PHONY: clean
clean:
	rm -rf $(XCBPROTO) $(LIBXCB) libxcb.a .aarch64
